#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

class session;
class ws_protocol {
public:
	static bool ws_shake_hand(session* s, char* body, int len);
	static int read_ws_header(unsigned char* pkg_data, int pkg_len, int* pkg_size, int* out_header_size);
	static void parser_ws_recv_data(unsigned char* raw_data, unsigned char* mark, int raw_len);
	static unsigned char* package_ws_data(const unsigned char* raw_data, int len, int* ws_data_len);
	static void free_package_data(unsigned char* ws_pkg);
};


#endif	// !__WEBSOCKET_H__