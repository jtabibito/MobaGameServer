#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uv.h"

#ifdef WIN32
#include <winsock.h>
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "libmysql.lib")
#endif

#include "mysql.h"
#include "mysql_wrapper.h"
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

	char* db_name;

	char* uname;
	char* upwd;

	void (*open_cb)(const char* err, void* context, void* udata);

	char* err;
	void* context;

	void* udata;
};

struct mysql_context {
	void* pConnContext;
	uv_mutex_t lock;

	int is_closed;
};

static void
connect_work(uv_work_t* req) {
	struct connect_req* connReq = (struct connect_req*)req->data;
	MYSQL* pConn = mysql_init(NULL);
	if (mysql_real_connect(pConn, connReq->ip, connReq->uname, connReq->upwd, connReq->db_name, connReq->port, NULL, 0)) {
		// connReq->context = pConn;
		struct mysql_context* c = (mysql_context*)my_malloc(sizeof(mysql_context));
		memset(c, 0, sizeof(mysql_context));

		c->pConnContext = pConn;
		uv_mutex_init(&(c->lock));

		connReq->context = c;
		connReq->err = NULL;
	} else {
		connReq->context = NULL;
		connReq->err = my_strdup(mysql_error(pConn));
	}
}

static void
after_connwork_complete(uv_work_t* req, int status) {
	struct connect_req* connReq = (struct connect_req*)req->data;
	connReq->open_cb(connReq->err, connReq->context, connReq->udata);

	if (connReq->ip) {
		free_strdup(connReq->ip);
	}
	if (connReq->db_name) {
		free_strdup(connReq->db_name);
	}
	if (connReq->uname) {
		free_strdup(connReq->uname);
	}
	if (connReq->upwd) {
		free_strdup(connReq->upwd);
	}
	if (connReq->err) {
		free_strdup(connReq->err);
	}

	my_free(connReq);
	my_free(req);
}

void
mysql_wrapper::connect(char* ip,
	int port,
	char* db_name,
	char* uname,
	char* pwd,
	void (*open_cb)(const char* err,
		void* context, void* udata),
	void* udata) {
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	struct connect_req* req = (struct connect_req*)my_malloc(sizeof(struct connect_req));
	memset(req, 0, sizeof(connect_req));

	req->ip = my_strdup(ip);
	req->port = port;
	req->db_name = my_strdup(db_name);
	req->uname = my_strdup(uname);
	req->upwd = my_strdup(pwd);
	req->open_cb = open_cb;
	req->udata = udata;

	w->data = req;

	uv_queue_work(uv_default_loop(), w, connect_work, after_connwork_complete);
}

static void
close_work(uv_work_t* req) {
	struct mysql_context* r = (mysql_context*)(req->data);
	uv_mutex_lock(&(r->lock)); 

	MYSQL* pConn = (MYSQL*)r->pConnContext;
	mysql_close(pConn);

	uv_mutex_unlock(&(r->lock));
}

static void
after_close_complete(uv_work_t* req, int status) {
	struct mysql_context* r = (mysql_context*)(req->data);

	my_free(r);
	my_free(req);
}

void
mysql_wrapper::close_mysql(void* context) {
	struct mysql_context* c = (struct mysql_context*)context;
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
	char* sql;
	void(*query_cb)(const char* err, MYSQL_RES* result, void* udata);

	char* err;
	MYSQL_RES* result;

	void* udata;
};

static void
query_work(uv_work_t* req) {
	struct query_req* r = (query_req*)req->data;

	// MYSQL* pConn = (MYSQL*)r->context;
	struct mysql_context* r2mysql = (mysql_context*)(r->context);

	uv_mutex_lock(&(r2mysql->lock));

	MYSQL* pConn = (MYSQL*)r2mysql->pConnContext;

	int ret = mysql_query(pConn, r->sql);
	if (ret != 0) {
		r->err = my_strdup(mysql_error(pConn));
		r->result = NULL;
		uv_mutex_unlock(&(r2mysql->lock));
		return;
	}

	r->err = NULL;
	
	MYSQL_RES* result = mysql_store_result(pConn);
	if (!result) {
		r->result = NULL;
		uv_mutex_unlock(&(r2mysql->lock));
		return;
	}
	r->result = result;

	/*
	MYSQL_ROW row;
	
	
	r->result = new std::vector<std::vector<std::string>>;
	int num = mysql_num_fields(result);

	std::vector<std::string> empty;
	std::vector<std::vector<std::string>>::iterator end_elem;
	while (row = mysql_fetch_row(result)) {
		r->result->push_back(empty);
		end_elem = r->result->end() - 1;
		for (int i = 0; i < num; i++) {
			end_elem->push_back(row[i]);
		}
	}

	mysql_free_result(result);
	*/

	uv_mutex_unlock(&(r2mysql->lock));
}

static void
after_query_complete(uv_work_t* req, int status) {
	struct query_req* r = (query_req*)req->data;
	r->query_cb(r->err, r->result, r->udata);

	if (r->sql) {
		my_free(r->sql);
	}
	if (r->result) {
		mysql_free_result(r->result);
		r->result = NULL;
	}
	if (r->err) {
		my_free(r->err);
	}
	my_free(r);
	my_free(req);
}

void
mysql_wrapper::query(void* context,
	char* sql,
	void (*query_cb)(const char* err, MYSQL_RES* result, void* udata),
	void* udata) {
	struct mysql_context* c = (struct mysql_context*)context;
	// c->is_closed = 1;
	if (c->is_closed) {
		return;
	}

	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));

	struct query_req* r = (query_req*)my_malloc(sizeof(query_req));
	memset(r, 0, sizeof(query_req));

	r->context = context;
	r->sql = my_strdup(sql);
	r->query_cb = query_cb;
	r->udata = udata;
	w->data = r;

	uv_queue_work(uv_default_loop(), w, query_work, after_query_complete);
}