#ifndef __NETBUS_H__
#define __NETBUS_H__

class netbus {
public:
	static netbus* instance();

public:
	void init();
	void start_tcp_server(int port);	// 提供启动tcp协议的端口
	void start_udp_server(int port);	// 提供启动udp协议的端口
	void start_ws_server(int port);		// 提供启动web服务的端口
	void run();
};


#endif // !__NETBUS_H__