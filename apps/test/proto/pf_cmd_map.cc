#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../netbus/proto_man.h"
#include "pf_cmd_map.h"

char* pf_cmd_map[] = {
	"LoginReq",
	"LoginRes",
};

void
init_pf_cmd_map() {
	proto_man::register_pf_cmd_map(pf_cmd_map, sizeof(pf_cmd_map) / sizeof(char*));
}
