#ifndef __NETBUS_H__
#define __NETBUS_H__

class session;

class netbus {
public:
	static netbus* instance();

public:
	void init();
	void tcp_listen(int port);	// 提供启动tcp协议的端口
	void udp_listen(int port);	// 提供启动udp协议的端口
	void ws_listen(int port);		// 提供启动web服务的端口
	void run();
	void connect2otherServer(char* server_ip, int port,
		void(*on_connected)(int err, session* s, void* udata),
		void* udata);
};


#endif // !__NETBUS_H__