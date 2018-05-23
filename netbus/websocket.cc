#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
using namespace std;

#include "../3rd/http_parser/http_parser.h"
#include "../3rd/crypto/base64_encoder.h"
#include "../3rd/crypto/sha1.h"

#include "session.h"
#include "websocket.h"


static int is_sec_key = 0;
static int has_sec_key = 0;
static int is_shake_end = 0;
static char value_sec_key[512];

static char* wb_migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static char* wb_accept = "HTTP/1.1 101 Switching Protocols\r\n"
"Upgrade:websocket\r\n"
"Connection: Upgrade\r\n"
"Sec-WebSocket-Accept: %s\r\n"
"WebSocket-Protocol:chat\r\n\r\n";

extern "C" {
	static int on_message_end(http_parser* p) {
		is_shake_end = 1;
		return 0;
	}
	
	static int on_ws_header_field(http_parser* p,
		const char *at,
		size_t length) {
		if (strncmp(at, "Sec-WebSocket-Key", length) == 0) {
			is_sec_key = 1;
		} else {
			is_sec_key = 0;
		}

		return 0;
	}

	static int on_ws_header_value(http_parser* p,
		const char *at, 
		size_t length) {
		if (is_sec_key) {
			has_sec_key = 1;
			strncpy(value_sec_key, at, length);
			value_sec_key[length] = 0;
		}
		return 0;
	}
}

bool
ws_protocol::ws_shake_hand(session* s, char* body, int len) {
	http_parser_settings settings;
	http_parser_settings_init(&settings);

	settings.on_header_field = on_ws_header_field;
	settings.on_header_value = on_ws_header_value;
	settings.on_message_complete = on_message_end;

	http_parser p;
	http_parser_init(&p, HTTP_REQUEST);
	is_sec_key = 0;
	has_sec_key = 0;
	is_shake_end = 0;
	http_parser_execute(&p, &settings, body, len);

	if (has_sec_key && is_shake_end) {
		printf("Sec-WebSocket-Key: %s\n", value_sec_key);
		static char key_migic[512];
		static char sha1_key_migic[SHA1_DIGEST_SIZE];
		static char sendto_client[512];
		sprintf(key_migic, "%s%s", value_sec_key, wb_migic);

		int sha1_sz;
		crypt_sha1((uint8_t*)key_migic, strlen(key_migic), (uint8_t*)&sha1_key_migic, &sha1_sz);
		int base64_sz;
		char* base64_buf = base64_encode((uint8_t*)sha1_key_migic, sha1_sz, &base64_sz);
		sprintf(sendto_client, wb_accept, base64_buf);
		base64_encode_free(base64_buf);
		
		s->send_data((unsigned char*)sendto_client, strlen(sendto_client));
		// send_data(stream, (unsigned char*)sendto_client, strlen(send_client));

		return true;
	}

	return false;
}

int
ws_protocol::read_ws_header(unsigned char* pkg_data, 
	int pkg_len, 
	int* pkg_size, 
	int* out_header_size) {
	if (pkg_data[0] != 0x81 && pkg_data[0] != 0x82) {
		return -1;
	}
	if (pkg_len < 2) {
		return -1;
	}
	unsigned int data_len = pkg_data[1] & 0x0000007f;
	int head_sz = 2;
	if (data_len == 126) {
		head_sz += 2;
		if (pkg_len < head_sz) {
			return -1;
		}

		data_len = pkg_data[3] | (pkg_data[2] << 8);
	} else if (data_len == 127) {
		head_sz += 8;
		if (pkg_len < head_sz) {
			return -1;
		}

		unsigned int low = pkg_data[5] | (pkg_data[4] << 8) | (pkg_data[3] << 16) | (pkg_data[2] << 24);
		unsigned int high = pkg_data[9] | (pkg_data[8] << 8) | (pkg_data[7] << 16) | (pkg_data[6] << 24);
		data_len = low;
	}

	head_sz += 4;
	*pkg_size = data_len + head_sz;
	*out_header_size = head_sz;

	return true;
}

void
ws_protocol::parser_ws_recv_data(unsigned char* raw_data,
	unsigned char* mark,
	int raw_len) {
	for (int i = 0; i < raw_len; i++) {
		raw_data[i] = raw_data[i] ^ mark[i % 4];
	}
}

// 打包好的准备发送的数据包
unsigned char*
ws_protocol::package_ws_data(const unsigned char* raw_data,
	int len,
	int* ws_data_len) {
	int head_sz = 2;
	if (len > 125 && len < 65536) {
		head_sz += 2;
	} else if (len >= 65536) {
		head_sz += 8;
		return NULL;
	}

	unsigned char* dbuf = (unsigned char*)malloc(head_sz + len);
	dbuf[0] = 0x81;
	if (len <= 125) {
		dbuf[1] = len;
	} else if (len > 125 && len < 65536) {
		dbuf[1] = 126;
		dbuf[2] = (len & 0x0000ff00) >> 8;
		dbuf[3] = (len & 0x000000ff);
	}
	memcpy(dbuf + head_sz, raw_data, len);
	*ws_data_len = (head_sz + len);

	return dbuf;
}

void
ws_protocol::free_package_data(unsigned char* ws_pkg) {
	free(ws_pkg);
}