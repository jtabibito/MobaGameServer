#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <map>

#include "proto_man.h"
#include "google\protobuf\message.h"

#include "../utils/cache_alloc.h"
#include "../utils/small_alloc.h"

extern cache_allocer* wbuf_allocer;
#define my_malloc small_alloc
#define my_free small_free

//#define MAX_PF_MAP_SIZE 1024
#define CMD_HEAD_LEN 8

static int g_proto_type = PROTO_BUF;
//static char* g_pf_map[MAX_PF_MAP_SIZE];
//static int g_cmd_count = 0;

static std::map<int, std::string> g_pb_cmd_map;

void
proto_man::init(int proto_type) {
	g_proto_type = proto_type;
}

void
proto_man::register_protobuf_cmd_map(std::map<int, std::string>& map) {
	std::map<int, std::string>::iterator it;
	for (it = map.begin(); it != map.end(); it++) {
		g_pb_cmd_map[it->first] = it->second;
	}
}

const char* 
proto_man::pb_cmd_ctype2name(int ctype) {
	return g_pb_cmd_map[ctype].c_str(); 
}

//
//void
//proto_man::register_pf_cmd_map(char** pf_map,
//	int len) {
//	len = (MAX_PF_MAP_SIZE - g_cmd_count) < len ? (MAX_PF_MAP_SIZE - g_cmd_count) : len;
//
//	for (int i = 0; i < len; i++) {
//		g_pf_map[g_cmd_count + i] = strdup(pf_map[i]);
//	}
//
//	g_cmd_count += len;
//}

google::protobuf::Message*
proto_man::create_message(const char* type_name) {
	google::protobuf::Message* message = NULL;

	const google::protobuf::Descriptor* descriptor =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);

	if (descriptor) {
		const google::protobuf::Message* prototype =
			google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype) {
			message = prototype->New();
		}
	}

	return message;
}

void
proto_man::release_message(google::protobuf::Message* msg) {
	delete msg;
}

// 2byte 服务号 | 命令号2byte | 用户标记4byte | 数据体
bool
proto_man::decode_cmd_msg(unsigned char* cmd,
	int cmd_len,
	struct cmd_msg** out_msg) {
	*out_msg = NULL;

	if (cmd_len < CMD_HEAD_LEN) {
		return false;
	}

	struct cmd_msg* msg = (struct cmd_msg*)my_malloc(sizeof(struct cmd_msg));
	memset(msg, 0, sizeof(struct cmd_msg));
	msg->stype = cmd[0] | (cmd[1] << 8);
	msg->ctype = cmd[2] | (cmd[3] << 8);
	msg->utag = cmd[4] | (cmd[5] << 8) | (cmd[6] << 16) | (cmd[7] << 24);
	msg->body = NULL;

	*out_msg = msg;
	if (cmd_len == CMD_HEAD_LEN) {
		return true;
	}



	if (g_proto_type == PROTO_JSON) {	// json 
		int json_len = cmd_len - CMD_HEAD_LEN;
		// char* json_str = (char*)malloc(json_len + 1);
		char* json_str = (char*)CacheAlloc(wbuf_allocer, json_len + 1);
		memcpy(json_str, cmd + CMD_HEAD_LEN, json_len);
		json_str[json_len] = 0;

		msg->body = (void*)json_str;
	} else { // protobuf
		/*if (msg->ctype < 0 || msg->ctype >= g_cmd_count || g_pf_map[msg->ctype] == NULL) {
			free(msg);
			*out_msg = NULL;
			return false;
		}*/
		google::protobuf::Message* pMsg = create_message(g_pb_cmd_map[msg->ctype].c_str());
		if (pMsg == NULL) {
			my_free(msg);
			*out_msg = NULL;
			return false;
		}
		if (!pMsg->ParseFromArray(cmd + CMD_HEAD_LEN, cmd_len - CMD_HEAD_LEN)) {
			my_free(msg);
			*out_msg = NULL;
			release_message(pMsg);
			return false;
		}
		msg->body = pMsg;
	}

	return true;
}

void 
proto_man::cmd_msg_free(struct cmd_msg* msg) {
	if (msg->body) {
		if (g_proto_type == PROTO_JSON) {
			// free(msg->body);
			CacheFree(wbuf_allocer, msg->body);
			msg->body = NULL;
		} else {
			google::protobuf::Message* pMsg = (google::protobuf::Message*)msg->body;
			delete pMsg;
			msg->body = NULL;
		}
	}
	my_free(msg);
}

int
proto_man::proto_type() {
	return g_proto_type;
}

unsigned char*
proto_man::encode_msg_to_raw(const struct cmd_msg* msg,
	int* out_len) {
	int raw_len = 0;
	unsigned char* raw_data = NULL;

	*out_len = 0;
	if (g_proto_type == PROTO_JSON) {
		char* json_str = NULL;
		int len = 0;
		if (msg->body) {
			json_str = (char*)msg->body;
			len = strlen(json_str + 1);
		}
		// raw_data = (unsigned char*)malloc(CMD_HEAD_LEN + len);
		raw_data = (unsigned char*)CacheAlloc(wbuf_allocer, CMD_HEAD_LEN + len);
		if (msg->body != NULL) {
			memcpy(raw_data + CMD_HEAD_LEN, json_str, len - 1);
			raw_data[CMD_HEAD_LEN + len] = 0;
		}
		*out_len = len + CMD_HEAD_LEN;
	} else if (g_proto_type == PROTO_BUF) {
		google::protobuf::Message* pMsg;
		int pf_len = 0;
		if (msg->body) {
			pMsg = (google::protobuf::Message*)msg->body;
			pf_len = pMsg->ByteSize();
		}
		// raw_data = (unsigned char*)malloc(CMD_HEAD_LEN + pf_len);
		raw_data = (unsigned char*)CacheAlloc(wbuf_allocer, CMD_HEAD_LEN + pf_len);
		if (msg->body != NULL) {
			if (!pMsg->SerializePartialToArray(raw_data + CMD_HEAD_LEN, pf_len)) {
				//free(raw_data);
				CacheFree(wbuf_allocer, raw_data);
				return NULL;
			}
		}
		*out_len = pf_len + CMD_HEAD_LEN;
	} else {
		return NULL;
	}

	raw_data[0] = (msg->stype) & 0x000000ff;
	raw_data[1] = (msg->stype) & 0x0000ff00 >> 8;
	raw_data[2] = (msg->ctype) & 0x000000ff;
	raw_data[3] = (msg->ctype) & 0x0000ff00 >> 8;
	memcpy(raw_data + 4, &msg->utag, 4);

	return raw_data;
}

void
proto_man::msg_raw_free(unsigned char* raw) {
	//free(raw);
	CacheFree(wbuf_allocer, raw);
}