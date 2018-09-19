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
#include "../utils/small_alloc.h"

#define my_malloc small_alloc
#define my_free small_free

static char*
my_strdup(const char* src) {
	int len = strlen(src) + 1;
	char* dst = (char*)my_malloc(len);
	strcpy(dst, src);
	return dst;
}

static void
free_strdup(char* str) {
	my_free(str);
}

struct connect_req {
	char* ip;
	int port;

	void (*open_cb)(const char* err, void* context, void* udata);

	char* err;
	void* context;
	void* udata;
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
		connReq->err = my_strdup(rcontext->errstr);
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
	connReq->open_cb(connReq->err, connReq->context, connReq->udata);

	if (connReq->ip) {
		free_strdup(connReq->ip);
	}
	if (connReq->err) {
		free_strdup(connReq->err);
	}

	my_free(connReq);
	my_free(req);
}

void
redis_wrapper::connect(char* ip,
	int port,
	void (*open_cb)(const char* err, void* context, void* udata),
	void* udata) {
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	struct connect_req* req = (struct connect_req*)my_malloc(sizeof(struct connect_req));
	memset(req, 0, sizeof(connect_req));

	req->ip = my_strdup(ip);
	req->port = port;
	req->udata = udata;
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

	// close_work(w);
	uv_queue_work(uv_default_loop(), w, close_work, after_close_complete);
}


struct query_req {
	void* context;
	char* cmd;
	void(*query_cb)(const char* err, redisReply* result, void* udata);

	char* err;
	redisReply* result;
	void* udata;
};

static void
query_work(uv_work_t* req) {
	struct query_req* r = (struct query_req*)req->data;

	struct redis_context* r2mysql = (struct redis_context*)(r->context);
	redisContext* rc = (redisContext*)r2mysql->pConnContext;

	uv_mutex_lock(&(r2mysql->lock));
	redisReply* reply = (redisReply*)redisCommand(rc, r->cmd);
	if (reply->type == REDIS_REPLY_ERROR) {
		r->err = my_strdup(reply->str);
		r->result = NULL;
		freeReplyObject(reply);
	} else {
		r->result = reply;
		r->err = NULL;
	}
	uv_mutex_unlock(&(r2mysql->lock));
}

static void
after_query_complete(uv_work_t* req, int status) {
	struct query_req* r = (query_req*)req->data;
	r->query_cb(r->err, r->result, r->udata);

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
	void (*query_cb)(const char* err, redisReply* result, void* udata),
	void* udata) {
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
	r->cmd = my_strdup(cmd);
	r->query_cb = query_cb;
	r->udata = udata;
	w->data = r;

	uv_queue_work(uv_default_loop(), w, query_work, after_query_complete);
}