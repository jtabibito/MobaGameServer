#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

#include "uv.h"
#include "session.h"
#include "session_uv.h"

#include "../utils/cache_alloc.h"
#include "websocket.h"
#include "tcp_protocol.h"
#include "proto_man.h"
#include "service_man.h"

#define SESSION_CACHE_CAPACITY 5000
#define WQ_CACHE_CAPACITY 4096

#define WBUF_CACHE_CAPACITY 1024
#define CMD_CACHE_SIZE 1024

struct cache_allocer* session_allocer = NULL;
static cache_allocer* wr_allocer = NULL;
cache_allocer* wbuf_allocer = NULL;

void init_session_allocer() {
	if (session_allocer == NULL) {
		session_allocer = CreateCacheAllocer(SESSION_CACHE_CAPACITY, sizeof(uv_session));
	}

	if (wr_allocer == NULL) {
		wr_allocer = CreateCacheAllocer(WQ_CACHE_CAPACITY, sizeof(uv_write_t));
	}

	if (wbuf_allocer == NULL) {
		wbuf_allocer = CreateCacheAllocer(WBUF_CACHE_CAPACITY, CMD_CACHE_SIZE);
	}
}

extern "C" {
	static void on_close(uv_handle_t* handle);

	static void 
	after_write(uv_write_t* req, int status) {
		if (status == 0) {
			printf("write success\n");
		}
		// free(req);
		CacheFree(wr_allocer, req);
	}

	static void 
	on_shutdown(uv_shutdown_t* req, int status) {
		uv_close((uv_handle_t*)req->handle, on_close);
	}

	static void
	on_close(uv_handle_t* handle) {
		printf("close client\n");
		uv_session* s = (uv_session*)handle->data;
		uv_session::destroy(s);
	}
}

uv_session*
uv_session::create() {
	// uv_session* uv_s = new uv_session();
	uv_session* uv_s = (uv_session*)CacheAlloc(session_allocer, sizeof(uv_session));
	uv_s->uv_session::uv_session();

	uv_s->init_session();

	return (uv_session*)uv_s;	//temp
}

void
uv_session::destroy(uv_session* s) {
	// uv_session* uv_s = (uv_session*)s;
	// uv_s->exit_session();
	// delete uv_s;	//temp;

	s->exit_session();
	s->uv_session::~uv_session();
	CacheFree(session_allocer, s);
}

void 
uv_session::init_session() {
	memset(this->ipaddr, 0, sizeof(this->ipaddr));
	this->client_port = 0;
	this->recved = 0;
	this->isShutdown = false;
	this->isWS_shake = 0;
	this->long_pkg = NULL;
	this->longpkg_sz = 0;
}

void 
uv_session::exit_session() {

}

void
uv_session::close() {
	if (this->isShutdown) {
		return;
	}

	// broadcast player lose connection
	service_man::on_session_disconnect(this);
	// end

	this->isShutdown = true;
	uv_shutdown_t* req = &(this->shutdown);
	memset(req, 0, sizeof(uv_shutdown_t));
	uv_shutdown(req, (uv_stream_t*)&(this->tcp_handle), on_shutdown);
}

void 
uv_session::send_data(unsigned char* body, int len) {
	/*uv_write_t* w_req = &(this->w_req);
	uv_buf_t* w_buf = &(this->w_buf);*/
	
	// uv_write_t* w_req = (uv_write_t*)malloc(sizeof(uv_write_t));
	uv_write_t* w_req = (uv_write_t*)CacheAlloc(wr_allocer, sizeof(uv_write_t));
	uv_buf_t w_buf;

	if (this->prototype == WS_SOCKET) {
		if (this->isWS_shake) {
			int ws_pkg_len;
			unsigned char* ws_pkg = ws_protocol::package_ws_data(body, len, &ws_pkg_len);
			w_buf = uv_buf_init((char*)ws_pkg, ws_pkg_len);
			uv_write(w_req, (uv_stream_t*)&(this->tcp_handle), &w_buf, 1, after_write);
			ws_protocol::free_package_data(ws_pkg);
		}
		else {
			w_buf = uv_buf_init((char*)body, len);
			uv_write(w_req, (uv_stream_t*)&(this->tcp_handle), &w_buf, 1, after_write);
		}
	} else {
		// w_buf = uv_buf_init((char*)body, len);
		// uv_write(w_req, (uv_stream_t*)&(this->tcp_handle), &w_buf, 1, after_write);
		int tcp_pkg_len;
		unsigned char* tcp_pkg = tcp_protocol::package(body, len, &tcp_pkg_len);
		w_buf = uv_buf_init((char*)tcp_pkg, tcp_pkg_len);
		uv_write(w_req, (uv_stream_t*)&(this->tcp_handle), &w_buf, 1, after_write);
		tcp_protocol::release_package(tcp_pkg);
	}
}

const char*
uv_session::get_address(int* client_port) {
	*client_port = this->client_port;
	return *(this->ipaddr);
}

void
uv_session::send_msg(struct cmd_msg* msg) {
	unsigned char* encode_pkg = NULL;
	int encode_len;
	encode_pkg = proto_man::encode_msg_to_raw(msg, &encode_len);
	if (encode_pkg) {
		this->send_data(encode_pkg, encode_len);
		proto_man::msg_raw_free(encode_pkg);
	}
}