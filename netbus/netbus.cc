#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "uv.h"
#include "session.h"
#include "session_uv.h"
#include "netbus.h"

extern "C" {
	static void uv_alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void after_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void on_shutdown(uv_shutdown_t* req, int status);
	static void	on_close(uv_handle_t* handle);
	static void after_write(uv_write_t* req, int status);

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
		printf("new client ip %s & port %d\n", s->ipaddr, s->client_port);

		// 使用eventloop管理事件
		uv_read_start((uv_stream_t*)client, uv_alloc_buf, after_read);
	}

	static void
	uv_alloc_buf(uv_handle_t* handle,		// 发生读事件的句柄
		size_t suggested_size,				// 建议分配多少内存
		uv_buf_t* buf) {					// 准备的内存地址
		
		uv_session* s = (uv_session*)handle->data;
		*buf = uv_buf_init(s->recv_buf + s->recved, RECV_LEN - s->recved);

	}

	static void
	after_read(uv_stream_t* stream,			// 发生读事件的句柄
		ssize_t nread,						// 读的数据字节大小
		const uv_buf_t* buf) {				// 读的数据内存地址
		uv_session* s = (uv_session*)stream->data;
			// 连接断开
		if (nread < 0) {
			uv_shutdown_t* req = &(s->shutdown);
			//uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
			memset(req, 0, sizeof(uv_shutdown_t));
			uv_shutdown(req, stream, on_shutdown);
			return;
		}

		buf->base[nread] = 0;
		printf("recv size %d\n", nread);
		printf("%s\n", buf->base);

		// test
		s->send_data((unsigned char*)buf->base, nread);
		s->recved = 0;
	}

	static void 
	on_shutdown(uv_shutdown_t* req, int status) {
		uv_close((uv_handle_t*)req->handle, on_close);
	}

	static void
	on_close(uv_handle_t* handle) {
		printf("Client Closed\n");
		uv_session* s = (uv_session*)handle->data;
		uv_session::destroy(s);
	}
}

static netbus g_netbus;
netbus* netbus::instance() {
	return &g_netbus;
}

void netbus::start_tcp_server(int port) {
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	int ret = uv_tcp_bind(listen, (const sockaddr*)&addr, 0);
	// 让event loop做监听管理, 当有人连接时eventloop就调用用户指定的处理函数uv_connection
	if (ret != 0) {
		cout << "Bind Error\n";
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_connection);
	listen->data = (void*)TCP_SOCKET;
}

void netbus::run() {
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
