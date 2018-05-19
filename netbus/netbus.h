#ifndef __NETBUS_H__
#define __NETBUS_H__

class netbus {
public:
	static netbus* instance();

public:
	void start_tcp_server(int port);	// 提供启动tcp协议的端口
	void start_ws_server(int port);		// 提供web协议端口
	void run();
};

#endif // !__NETBUS_H__