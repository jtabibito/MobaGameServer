#ifndef __NETBUS_H__
#define __NETBUS_H__

class netbus {
public:
	static netbus* instance();

public:
	void init();
	void start_tcp_server(int port);	// �ṩ����tcpЭ��Ķ˿�
	void start_udp_server(int port);	// �ṩ����udpЭ��Ķ˿�
	void start_ws_server(int port);		// �ṩ����web����Ķ˿�
	void run();
};


#endif // !__NETBUS_H__