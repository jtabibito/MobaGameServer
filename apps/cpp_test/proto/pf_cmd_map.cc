#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <map>

#include "../../../netbus/proto_man.h"
#include "pf_cmd_map.h"

//
//char* pf_cmd_map[] = {
//	"LoginReq",
//	"LoginRes",
//};

std::map<int, std::string> cmd_map = {
	{0, "LoginReq"},
	{1, "LoginRes"},
};

void
init_pf_cmd_map() {
	//proto_man::register_pf_cmd_map(pf_cmd_map, sizeof(pf_cmd_map) / sizeof(char*));
	proto_man::register_protobuf_cmd_map(cmd_map);
}
