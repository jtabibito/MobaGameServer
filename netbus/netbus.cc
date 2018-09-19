#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "uv.h"
#include "session.h"
#include "session_uv.h"
#include "udp_session.h"
#include "websocket.h"
#include "tcp_protocol.h"
#include "netbus.h"
#include "proto_man.h"
#include "service_man.h"

extern "C" {
	static void uv_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void after_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void on_shutdown(uv_shutdown_t* req, int status);
	static void	on_close(uv_handle_t* handle);
	static void after_write(uv_write_t* req, int status);
	static void on_recv_ws_data(uv_session* s);
	static void on_recv_comm(session* s, unsigned char* body, int len);
	static void on_recv_tcp_data(uv_session* s);

	static void
	uv_connection(uv_stream_t* server, int status) {
		// 接入客户端
		uv_session* s = (uv_session*)uv_session::create();
		uv_tcp_t* client = &(s->tcp_handle);
		memset(client, 0, sizeof(uv_tcp_t));
		uv_tcp_init(uv_default_loop(), client);
		client->data = (void*)s;
		uv_accept(server, (uv_stream_t*)client);

		struct sockaddr_in addr;
		int len = sizeof(addr);
		uv_tcp_getpeername(client, (sockaddr*)&addr, &len);
		uv_ip4_name(&addr, (char*)s->ipaddr, 64);
		s->client_port = ntohs(addr.sin_port);
		s->prototype = (int)(server->data);
		// printf("new client ip %s & port %d\n", s->ipaddr, s->client_port);

		// 使用eventloop管理事件
		uv_read_start((uv_stream_t*)client, uv_alloc_buf, after_read);
	}

	struct udp_recv_buf {
		char* recv_buf;
		size_t max_recv_size;
	};

	static void
	udp_uv_alloc_buf(uv_handle_t* handle,
		size_t suggested_size,
		uv_buf_t* buf) {

		suggested_size = (suggested_size < 8192) ? 8192 : suggested_size;

		struct udp_recv_buf* udp_buf = (struct udp_recv_buf*)handle->data;
		if (udp_buf->max_recv_size < suggested_size) {
			if (udp_buf->recv_buf) {
				// 释放掉buf空间
				free(udp_buf->recv_buf);
				udp_buf->recv_buf = NULL;
			}
			udp_buf->max_recv_size = suggested_size;
			udp_buf->recv_buf = (char*)malloc(udp_buf->max_recv_size);
		}

		buf->base = udp_buf->recv_buf;
		buf->len = udp_buf->max_recv_size;
	}

	static void
	after_uv_udp_recv(uv_udp_t* handle,
        ssize_t nread,
        const uv_buf_t* buf,
        const struct sockaddr* addr,
        unsigned flags) {

		udp_session udp_s;
		udp_s.udp_handle = handle;
		udp_s.addr = addr;
		uv_ip4_name((const sockaddr_in*)addr, udp_s.c_address, 32);
		udp_s.c_port = ntohs(((const sockaddr_in*)addr)->sin_port);

		on_recv_comm((session*)&udp_s, (unsigned char*)buf->base, nread);
	}

	static void
	uv_alloc_buf(uv_handle_t* handle,		// 发生读事件的句柄
		size_t suggested_size,				// 建议分配多少内存
		uv_buf_t* buf) {					// 准备的内存地址
		
		uv_session* s = (uv_session*)handle->data;
		
		if (s->recved < RECV_LEN) {
			*buf = uv_buf_init(s->recv_buf + s->recved, RECV_LEN - s->recved);
		} else {
			if (s->long_pkg == NULL) {	// 数据包无法读完, 分配出内存
				if (s->prototype == WS_SOCKET && s->isWS_shake) {    // websocket > RECV_LEN 的包
					int pkg_size;
					int head_size;
					ws_protocol::read_ws_header((unsigned char*)s->recv_buf, s->recved, &pkg_size, &head_size);
					s->longpkg_sz = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);
					memcpy(s->long_pkg, s->recv_buf, s->recved);
				} else {	// tcp > RECV_LEN 的包
					int pkg_size;
					int head_size;
					tcp_protocol::read_header((unsigned char*)s->recv_buf, s->recved, &pkg_size, &head_size);
					s->longpkg_sz = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);
					memcpy(s->long_pkg, s->recv_buf, s->recved);
				}
			}
			*buf = uv_buf_init(s->recv_buf + s->recved, s->longpkg_sz - s->recved);
		}
	}

	static void
	after_read(uv_stream_t* stream,			// 发生读事件的句柄
		ssize_t nread,						// 读的数据字节大小
		const uv_buf_t* buf) {				// 读的数据内存地址
		uv_session* s = (uv_session*)stream->data;
			// 连接断开
		if (nread < 0) {
			// uv_shutdown_t* req = &(s->shutdown);
			// uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
			// memset(req, 0, sizeof(uv_shutdown_t));
			// uv_shutdown(req, stream, on_shutdown);
			s->close();
			return;
		}

		//buf->base[nread] = 0;
		//printf("recv size %d\n", nread);
		//printf("%s\n", buf->base);

		//// test
		//s->send_data((unsigned char*)buf->base, nread);
		//s->recved = 0;
		s->recved += nread;
		if (s->prototype == WS_SOCKET) {	// if web socket
			if (s->isWS_shake == 0) {	// if dont shakehand
				if (ws_protocol::ws_shake_hand((session*)s, s->recv_buf, s->recved)) {
					s->isWS_shake = 1;
					s->recved = 0;
				}
			} else {	// websocket recv or send data
				on_recv_ws_data(s);
			}
		} else {	// else TCP socket
			on_recv_tcp_data(s);
		}
	}

	static void 
	on_shutdown(uv_shutdown_t* req, int status) {
		uv_close((uv_handle_t*)req->handle, on_close);
	}

	static void
	on_close(uv_handle_t* handle) {
		// printf("Client Closed\n");
		uv_session* s = (uv_session*)handle->data;
		uv_session::destroy(s);
	}

	static void 
	on_recv_ws_data(uv_session* s) {
		unsigned char* pkg_data = (unsigned char*)((s->long_pkg != NULL) ? s->long_pkg : s->recv_buf);
		while (s->recved > 0) {
			int pkg_sz = 0;
			int head_sz = 0;
			if (pkg_data[0] == 0x88) {
				// printf("Web用户退出\n");
				s->close();
				break;
			}
			// pkg_sz - head_sz = body_sz;
			if (ws_protocol::read_ws_header(pkg_data, s->recved, &pkg_sz, &head_sz) == -1) {
				break;
			}

			if (s->recved < pkg_sz) {
				break;
			}

			unsigned char* raw_data = pkg_data + head_sz;
			unsigned char* mark = raw_data - 4;
			ws_protocol::parser_ws_recv_data(raw_data, mark, pkg_sz - head_sz);

			on_recv_comm((session*)s, raw_data, pkg_sz - head_sz);

			if (s->recved > pkg_sz) {
				memmove(pkg_data, pkg_data + pkg_sz, s->recved - pkg_sz);
			}
			s->recved -= pkg_sz;

			if (s->recved == 0 && s->long_pkg != NULL) {
				free(s->long_pkg);
				s->long_pkg = NULL;
				s->longpkg_sz = 0;
			}
		}
	}

	static void
	on_recv_comm(session* s,
		unsigned char* body,
		int len) {
		// printf("Client command !\n");
		
		// test
		struct cmd_msg* msg = NULL;
		if (proto_man::decode_cmd_msg(body, len, &msg)) {
			/*unsigned char* encode_pkg = NULL;
			int encode_len;
			encode_pkg = proto_man::encode_msg_to_raw(msg, &encode_len);
			if (encode_pkg) {
				s->send_data(encode_pkg, encode_len);
				proto_man::msg_raw_free(encode_pkg);
			}*/

			if (!service_man::on_recv_cmd_msg((session*)s, msg)) {
				s->close();
			}
			proto_man::cmd_msg_free(msg);
		}
	}

	static void
	on_recv_tcp_data(uv_session* s) {
		unsigned char* pkg_data = (unsigned char*)((s->long_pkg != NULL) ? s->long_pkg : s->recv_buf);
		while (s->recved > 0) {
			int pkg_sz = 0;
			int head_sz = 0;
			// pkg_sz - head_sz = body_sz;
			if (!tcp_protocol::read_header(pkg_data, s->recved, &pkg_sz, &head_sz)) {
				break;
			}

			if (s->recved < pkg_sz) {
				break;
			}

			unsigned char* raw_data = pkg_data + head_sz;
			on_recv_comm((session*)s, raw_data, pkg_sz - head_sz);

			if (s->recved > pkg_sz) {
				memmove(pkg_data, pkg_data + pkg_sz, s->recved - pkg_sz);
			}
			s->recved -= pkg_sz;

			if (s->recved == 0 && s->long_pkg != NULL) {
				free(s->long_pkg);
				s->long_pkg = NULL;
				s->longpkg_sz = 0;
			}
		}
	}
}

static netbus g_netbus;
netbus* netbus::instance() {
	return &g_netbus;
}

void
netbus::tcp_listen(int port) {
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	int ret = uv_tcp_bind(listen, (const sockaddr*)&addr, 0);
	// 让event loop做监听管理, 当有人连接时eventloop就调用用户指定的处理函数uv_connection
	if (ret != 0) {
		// printf("Bind Error\n");
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_connection);
	listen->data = (void*)TCP_SOCKET;
}

void
netbus::udp_listen(int port) {
	uv_udp_t* server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
	memset(server, 0, sizeof(uv_udp_t));

	uv_udp_init(uv_default_loop(), server);

	struct udp_recv_buf* udp_buf = (struct udp_recv_buf*)malloc(sizeof(struct udp_recv_buf));
	memset(udp_buf, 0, sizeof(struct udp_recv_buf));

	server->data = udp_buf;

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	uv_udp_bind(server, (const struct sockaddr*)&addr, 0);
	
	uv_udp_recv_start(server, udp_uv_alloc_buf, after_uv_udp_recv);
}

void
netbus::ws_listen(int port) {
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	int ret = uv_tcp_bind(listen, (const sockaddr*)&addr, 0);
	// 让event loop做监听管理, 当有人连接时eventloop就调用用户指定的处理函数uv_connection
	if (ret != 0) {
		// printf("Bind Error\n");
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_connection);
	listen->data = (void*)WS_SOCKET;
}

void
netbus::run() {
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void
netbus::init() {
	service_man::init();
	init_session_allocer();
}

struct connect_cb {
	void(*on_connected)(int err, session* s, void* udata);
	void* udata;
};

static void 
after_connect(uv_connect_t* req,
	int status) {
	uv_session* s = (uv_session*)req->handle->data;
	struct connect_cb* cb = (struct connect_cb*)req->data;

	if (status) {
		if (cb->on_connected) {
			cb->on_connected(1, NULL, cb->udata);
		}
		s->close();
		free(cb);
		free(req);
		return;
	}

	uv_read_start(req->handle, uv_alloc_buf, after_read);
	if (cb->on_connected) {
		cb->on_connected(0, (session*)s, cb->udata);
	}
	free(cb);
	free(req);
}

void
netbus::connect2otherServer(char* server_ip, int port,
							void(*on_connected)(int err, session* s, void* udata),
							void* udata) {
	struct sockaddr_in* bind_addr;
	int ret = uv_ip4_addr(server_ip, port, bind_addr);
	if (ret) {
		return;
	}

	uv_session* s = (uv_session*)uv_session::create();
	uv_tcp_t* client = &(s->tcp_handle);
	memset(client, 0, sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);
	client->data = (void*)s;
	s->as_client = 1;
	s->prototype = TCP_SOCKET;
	strcpy(s->ipaddr, server_ip);
	s->client_port = port;

	uv_connect_t* connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	connect_cb* cb = (struct connect_cb*)malloc(sizeof(struct connect_cb));
	cb->on_connected = on_connected;
	cb->udata = udata;
	connect_req->data = (void*)cb;

	ret = uv_tcp_connect(connect_req, client, (const sockaddr*)&bind_addr, after_connect);
	if (ret) {
		return;
	}
}