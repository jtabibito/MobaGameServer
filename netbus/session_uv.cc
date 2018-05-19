#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

#include "uv.h"
#include "session.h"
#include "session_uv.h"

extern "C" {
	static void on_close(uv_handle_t* handle);

	static void 
	after_write(uv_write_t* req, int status) {
		if (status == 0) {
			printf("write success\n");
		}
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
	uv_session* uv_s = new uv_session();
	uv_s->init_session();

	return (uv_session*)uv_s;	//temp
}

void
uv_session::destroy(uv_session* s) {
	uv_session* uv_s = (uv_session*)s;
	uv_s->exit_session();
	s->exit_session();

	delete uv_s;	//temp;
}

void 
uv_session::init_session() {
	memset(this->ipaddr, 0, sizeof(this->ipaddr));
	this->client_port = 0;
	this->recved = 0;
}

void 
uv_session::exit_session() {

}

void
uv_session::close() {
	uv_shutdown_t* req = &(this->shutdown);
	memset(req, 0, sizeof(uv_shutdown_t));
	uv_shutdown(req, (uv_stream_t*)&(this->tcp_handle), on_shutdown);
}

void 
uv_session::send_data(unsigned char* body, int len) {
	uv_write_t* w_req = &(this->w_req);
	uv_buf_t* w_buf = &(this->w_buf);

	*w_buf = uv_buf_init((char*)body, len);
	uv_write(w_req, (uv_stream_t*)&(this->tcp_handle), w_buf, 1, after_write);
}

const char*
uv_session::get_address(int* client_port) {
	*client_port = this->client_port;
	return *(this->ipaddr);
}