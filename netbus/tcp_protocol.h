#ifndef __TCP_PROTOCOL_H__
#define __TCP_PROTOCOL_H__

class tcp_protocol {public:	static bool read_header(unsigned char* data, int data_len, int* pkg_size, int* out_header_size);	static unsigned char* package(const unsigned char* raw_data, int len, int* pkg_len);	static void release_package(unsigned char* tcp_pkg);};

#endif // !__TCP_PROTOCOL_H__