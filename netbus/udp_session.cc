#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

#include "uv.h"
#include "session.h"
#include "udp_session.h"

#include "../utils/cache_alloc.h"
#include "../utils/small_alloc.h"
#include "websocket.h"
#include "tcp_protocol.h"
#include "proto_man.h"
#include "service_man.h"

#define my_alloc small_alloc
#define my_free small_free

void 
udp_session::close() {

}

static void
on_uv_udp_send_end(uv_udp_send_t* req, int status) {
	if (status == 0) {
		// 
	}
	my_free(req);
}

void
udp_session::send_data(unsigned char* body, int len) {
	uv_buf_t w_buf;
	w_buf = uv_buf_init((char*)body, len);
	uv_udp_send_t* req = (uv_udp_send_t*)my_alloc(sizeof(uv_udp_send_t));

	uv_udp_send(req, this->udp_handle, &w_buf, 1, this->addr, on_uv_udp_send_end);
}

const char* 
udp_session::get_address(int* client_port) {
	*client_port = this->c_port;
	return this->c_address;
}

void
udp_session::send_msg(struct cmd_msg* msg) {
	unsigned char* encode_pkg = NULL;
	int encode_len;
	encode_pkg = proto_man::encode_msg_to_raw(msg, &encode_len);
	if (encode_pkg) {
		this->send_data(encode_pkg, encode_len);
		proto_man::msg_raw_free(encode_pkg);
	}
}
