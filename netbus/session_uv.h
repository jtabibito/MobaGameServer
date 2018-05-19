#ifndef __SESSION_UV_H__
#define __SESSION_UV_U__

// ����4k�㹻
#define RECV_LEN 4096

enum {
	TCP_SOCKET,
	WS_SOCKET,
};

class uv_session : session {
public:
	// ����malloc�Ľṹ
	char* ipaddr[32];
	int client_port;
	uv_tcp_t tcp_handle;
	uv_shutdown_t shutdown;
	uv_write_t w_req;
	uv_buf_t w_buf;

public:
	char recv_buf[RECV_LEN];
	int recved;
	int prototype;

private:
	void init_session();
	void exit_session();

public:
	static uv_session* create();
	static void destroy(uv_session* s);

public:
	virtual void close();
	virtual void send_data(unsigned char* body, int len);
	virtual const char* get_address(int* client_port);
};

#endif // !__SESSION_UV_H__