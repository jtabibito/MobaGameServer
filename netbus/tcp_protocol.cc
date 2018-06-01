#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp_protocol.h"
#include "../utils/cache_alloc.h"

extern cache_allocer* wbuf_allocer;

bool
tcp_protocol::read_header(unsigned char* data,
	int data_len,
	int* pkg_size,
	int* out_header_size) {
	if (data_len < 2) {
		return false;
	}
	*pkg_size = (data[0] | data[1] << 8);
	*out_header_size = 2;
	return true;
}

unsigned char* 
tcp_protocol::package(const unsigned char* raw_data,
	int len, int* pkg_len) {
	int head_sz = 2;
	
	*pkg_len = (head_sz + len);
	// unsigned char* dbuf = (unsigned char*)malloc(head_sz + len);
	unsigned char* dbuf = (unsigned char*)CacheAlloc(wbuf_allocer, *pkg_len);
	dbuf[0] = (unsigned char)(*pkg_len) & 0x000000ff;
	dbuf[1] = (unsigned char)(((*pkg_len) & 0x0000ff00) >> 8);
	
	memcpy(dbuf + head_sz, raw_data, len);

	return dbuf;
}

void
tcp_protocol::release_package(unsigned char* tcp_pkg) {
	// free(tcp_pkg);
	CacheFree(wbuf_allocer, tcp_pkg);
}