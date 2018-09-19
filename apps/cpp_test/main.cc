#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
using namespace std;

#include "../../netbus/proto_man.h"
#include "../../netbus/netbus.h"
#include "proto/pf_cmd_map.h"
#include "../../db/mysql_wrapper.h"
#include "../../db/redis_wrapper.h"
#include "../../utils/logManager.h"
#include "../../utils/timestamp.h"
// #include "../../lua_wrapper/lua_wrapper.h"

static void
on_query_cb(const char* err, MYSQL_RES* result, void* udata) {
	if (err) {
		printf("err %s\n", err);
		return;
	}

	printf("success\n");
}

static void
on_open_cb(const char* err, void* context, void* udata) {
	if (err != NULL) {
		printf("%s\n", err);
		return;
	}
	printf("connect success\n");

	mysql_wrapper::query(context, "insert into studentinfo (name, id, sex) values(\"计霖\", 1615925449, \"男\")", on_query_cb);
	// mysql_wrapper::query(context, "update studentinfo set name = \"shakker2c\" where id = 1615925449", on_query_cb);
	// mysql_wrapper::close_mysql(context);
}

static void
test_db() {
	mysql_wrapper::connect("127.0.0.1", 3306, "class_data", "root", "null980311", on_open_cb);
}

static void 
on_query_redis(const char* err, redisReply* result, void* udata) {
	if (err) {
		printf("%s\n", err);
		return;
	}

	printf("success\n");
}

static void
on_redis_open(const char* err, void* context, void* udata) {
	if (err != NULL) {
		printf("%s\n", err);
		return;
	}
	printf("connect success\n");

	redis_wrapper::query(context, "select 15", on_query_redis);
	// redis_wrapper::close_redis(context);
}

static void
test_redisdb() {
	redis_wrapper::connect("127.0.0.1", 6379, on_redis_open);
}

int main(int argc, char* argv[]) {

	proto_man::init(PROTO_BUF);
	init_pf_cmd_map();
	logger::init("/Moba_Server/apps/DebugLog", "TestDebugLog", true);

	log_debug("%d", timestamp());
	log_debug("%d", timestamp_today());

	netbus::instance()->init();
	netbus::instance()->tcp_listen(6080);
	netbus::instance()->tcp_listen(6081); // 开启多个端口
	netbus::instance()->ws_listen(8001);
	netbus::instance()->udp_listen(8002);

	netbus::instance()->run();
	/*
	lua_wrapper::init();
	string path = "./main.lua";
	lua_wrapper::entry_lua_file(path);

	lua_wrapper::exit();
	*/

	return 0;
}