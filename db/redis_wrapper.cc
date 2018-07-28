#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <hiredis.h>

#ifdef WIN32

#define NO_QFORKIMPL //这一行必须加才能正常使用
#include <Win32_Interop/win32fixes.h>
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")

#endif

#include "uv.h"
#include "redis_wrapper.h"

#define my_malloc malloc
#define my_free free

struct connect_req {
	char* ip;
	int port;

	void (*open_cb)(const char* err, void* context);

	char* err;
	void* context;
};

struct redis_context {
	void* pConnContext;
	uv_mutex_t lock;

	int is_closed;
};

static void
connect_work(uv_work_t* req) {
	struct connect_req* connReq = (struct connect_req*)req->data;
	struct timeval timeout = { 1, 500000 };
	redisContext* rcontext = redisConnectWithTimeout(connReq->ip, connReq->port, timeout);
	if (rcontext->err) {
		printf("Connection error: %s\n", rcontext->errstr);
		connReq->err = strdup(rcontext->errstr);
		connReq->context = NULL;
		redisFree(rcontext);
	} else {
		struct redis_context* c = (struct redis_context*)my_malloc(sizeof(struct redis_context));
		memset(c, 0, sizeof(struct redis_context));

		c->pConnContext = rcontext;
		uv_mutex_init(&(c->lock));
		connReq->context = c;
		connReq->err = NULL;
	}
}

static void
after_connwork_complete(uv_work_t* req, int status) {
	struct connect_req* connReq = (struct connect_req*)req->data;
	connReq->open_cb(connReq->err, connReq->context);

	if (connReq->ip) {
		free(connReq->ip);
	}
	if (connReq->err) {
		free(connReq->err);
	}

	my_free(connReq);
	my_free(req);
}

void
redis_wrapper::connect(char* ip,
	int port,
	void (*open_cb)(const char* err,
		void* context)) {
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	struct connect_req* req = (struct connect_req*)my_malloc(sizeof(struct connect_req));
	memset(req, 0, sizeof(connect_req));

	req->ip = strdup(ip);
	req->port = port;
	req->open_cb = open_cb;

	w->data = req;
	uv_queue_work(uv_default_loop(), w, connect_work, after_connwork_complete);
}

static void
close_work(uv_work_t* req) {
	struct redis_context* r = (redis_context*)(req->data);
	uv_mutex_lock(&(r->lock)); 

	//MYSQL* pConn = (MYSQL*)r->pConnContext;
	//mysql_close(pConn);

	redisContext* rc = (redisContext*)r->pConnContext;
	redisFree(rc);
	r->pConnContext = NULL;

	uv_mutex_unlock(&(r->lock));
}

static void
after_close_complete(uv_work_t* req, int status) {
	struct redis_context* r = (redis_context*)(req->data);
	my_free(r);
	my_free(req);
}

void
redis_wrapper::close_redis(void* context) {
	struct redis_context* c = (struct redis_context*)context;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	w->data = context;

	c->is_closed = 1;

	close_work(w);
	uv_queue_work(uv_default_loop(), w, close_work, after_close_complete);
}


struct query_req {
	void* context;
	char* cmd;
	void(*query_cb)(const char* err, redisReply* result);

	char* err;
	redisReply* result;
};

static void
query_work(uv_work_t* req) {
	struct query_req* r = (struct query_req*)req->data;

	struct redis_context* r2mysql = (struct redis_context*)(r->context);
	redisContext* rc = (redisContext*)r2mysql->pConnContext;

	uv_mutex_lock(&(r2mysql->lock));
	r->err = NULL;
	redisReply* reply = (redisReply*)redisCommand(rc, r->cmd);
	if (reply) {
		r->result = reply;
	}
	uv_mutex_unlock(&(r2mysql->lock));
}

static void
after_query_complete(uv_work_t* req, int status) {
	struct query_req* r = (query_req*)req->data;
	r->query_cb(r->err, r->result);

	if (r->cmd) {
		my_free(r->cmd);
	}
	if (r->result) {
		freeReplyObject(r->result);
	}
	if (r->err) {
		my_free(r->err);
	}

	my_free(r);
	my_free(req);
}

void
redis_wrapper::query(void* context,
	char* cmd,
	void (*query_cb)(const char* err, redisReply* result)) {
	struct redis_context* c = (struct redis_context*)context;
	// c->is_closed = 1;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	struct query_req* r = (struct query_req*)my_malloc(sizeof(struct query_req));
	memset(r, 0, sizeof(struct query_req));

	r->context = context;
	r->cmd = strdup(cmd);
	r->query_cb = query_cb;

	w->data = r;

	uv_queue_work(uv_default_loop(), w, query_work, after_query_complete);
}