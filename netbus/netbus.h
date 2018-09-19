#ifndef __NETBUS_H__
#define __NETBUS_H__

class session;

class netbus {
public:
	static netbus* instance();

public:
	void init();
	void tcp_listen(int port);	// �ṩ����tcpЭ��Ķ˿�
	void udp_listen(int port);	// �ṩ����udpЭ��Ķ˿�
	void ws_listen(int port);		// �ṩ����web����Ķ˿�
	void run();
	void connect2otherServer(char* server_ip, int port,
		void(*on_connected)(int err, session* s, void* udata),
		void* udata);
};


#endif // !__NETBUS_H__