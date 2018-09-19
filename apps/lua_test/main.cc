#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "../../netbus/proto_man.h"
#include "../../netbus/netbus.h"
#include "../../db/mysql_wrapper.h"
#include "../../db/redis_wrapper.h"
#include "../../utils/logManager.h"
#include "../../utils/timestamp.h"
#include "../../lua_wrapper/lua_wrapper.h"

int main(int argc, char* argv[]) {
	netbus::instance()->init();
	lua_wrapper::init();
	logger::init("../../apps/logs", "Running", true);
	//netbus::instance()->connect2otherServer("127.0.0.1", 7788, NULL, NULL);

	if (argc != 3) {
		string search_path = "../../apps/lua_test/scripts/";
		lua_wrapper::add_search_path(search_path);
		string path = search_path + "main.lua";
		lua_wrapper::entry_lua_file(path);
	} else {
		string search_path = argv[1];
		if (*(search_path.end() - 1) != '/') {
			search_path += "/";
		}
		lua_wrapper::add_search_path(search_path);

		string lua_file = search_path + argv[2];
		lua_wrapper::entry_lua_file(lua_file);
	}


	netbus::instance()->run();
	lua_wrapper::exit();
	
	return 0;
}