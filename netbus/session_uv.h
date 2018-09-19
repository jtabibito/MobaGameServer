#ifndef __SESSION_UV_H__
#define __SESSION_UV_U__

// 假设4k足够
#define RECV_LEN 4096

enum {
	TCP_SOCKET,
	WS_SOCKET,
};

class uv_session : public session {
public:
	// 保存malloc的结构
	uv_tcp_t tcp_handle;
	char ipaddr[32];
	int client_port;

	uv_shutdown_t shutdown;
	bool isShutdown;	// 避免重复shutdown
	/*uv_write_t w_req;
	uv_buf_t w_buf;*/

public:
	char recv_buf[RECV_LEN];
	int recved;
	int prototype;
	char* long_pkg;
	int longpkg_sz;

public:
	int isWS_shake;

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
	virtual void send_msg(struct cmd_msg* msg);
};

void init_session_allocer();

#endif // !__SESSION_UV_H__
