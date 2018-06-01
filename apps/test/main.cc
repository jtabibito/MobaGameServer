#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "../../netbus/proto_man.h"
#include "../../netbus/netbus.h"
#include "proto/pf_cmd_map.h"

int main(int argc, char* argv[]) {
	proto_man::init(PROTO_BUF);
	init_pf_cmd_map();

	netbus::instance()->init();
	netbus::instance()->start_tcp_server(6080);
	netbus::instance()->start_tcp_server(6081); // ��������˿�
	netbus::instance()->start_ws_server(8001);

	netbus::instance()->run();
	return 0;
}