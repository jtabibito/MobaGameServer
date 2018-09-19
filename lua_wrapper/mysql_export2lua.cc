#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua_wrapper.h"
#include "../db/mysql_wrapper.h"
#include "mysql_export2lua.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"

static void
on_open_cb(const char* err, void* context, void* udata) {
	if (err) {
		lua_pushstring(lua_wrapper::lua_state(), err);
		lua_pushnil(lua_wrapper::lua_state());
	} else {
		lua_pushnil(lua_wrapper::lua_state());
		tolua_pushuserdata(lua_wrapper::lua_state(), context);
	}

	lua_wrapper::executeFunctionByHandle((int)udata, 2);
	lua_wrapper::removeByHandle((int)udata);
}

static int 
lua_mysql_connect(lua_State* L) {
	// 拿到lua脚本中的参数
	char* ip = (char*)tolua_tostring(L, 1, NULL);
	if (ip == NULL) {
		goto lua_failed;
	}

	int port = (int)tolua_tonumber(L, 2, 0);

	char* db_name = (char*)tolua_tostring(L, 3, NULL);
	if (db_name == NULL) {
		goto lua_failed;
	}

	char* uname = (char*)tolua_tostring(L, 4, NULL);
	if (uname == NULL) {
		goto lua_failed;
	}

	char* upwd = (char*)tolua_tostring(L, 5, NULL);
	if (upwd == NULL) {
		goto lua_failed;
	}

	int handle = toluafix_ref_function(L, 6, 0);

	mysql_wrapper::connect(ip, port, db_name, uname, upwd, on_open_cb, (void*)handle);

lua_failed:
	return 0;
}

static int
lua_mysql_close(lua_State* L) {
	void* context = tolua_touserdata(L, 1, 0);
	if (context) {
		mysql_wrapper::close_mysql(context);
	}
	return 0;
}

static void
push_mysqlrow(MYSQL_ROW row, int num) {
	lua_newtable(lua_wrapper::lua_state());
	int index = 1;
	for (int i = 0; i < num; i++) {
		if (row[i] == NULL) {
			lua_pushnil(lua_wrapper::lua_state());
		} else {
			lua_pushstring(lua_wrapper::lua_state(), row[i]);
		}
		lua_rawseti(lua_wrapper::lua_state(), -2, index);
		++index;
	}
}

static void
on_lua_query_cb(const char* err, MYSQL_RES* result, void* udata) {
	if (err) {
		lua_pushstring(lua_wrapper::lua_state(), err);
		lua_pushnil(lua_wrapper::lua_state());
	} else {
		lua_pushnil(lua_wrapper::lua_state());
		if (result) { // 把查询结果push成表{{}, {}, {}, ...}
			lua_newtable(lua_wrapper::lua_state());
			int index = 1;
			int num = mysql_num_fields(result);
			MYSQL_ROW row;
			while (row = mysql_fetch_row(result)) {
				push_mysqlrow(row, num);
				lua_rawseti(lua_wrapper::lua_state(), -2, index);
				++index;
			}
		} else {
			lua_pushnil(lua_wrapper::lua_state());
		}
	}

	lua_wrapper::executeFunctionByHandle((int)udata, 2);
	lua_wrapper::removeByHandle((int)udata);
}

static int
lua_mysql_query(lua_State* L) {
	void* context = tolua_touserdata(L, 1, 0);
	if (!context) {
		goto lua_failed;
	}
	
	char* sql = (char*)tolua_tostring(L, 2, 0);
	if (sql == NULL) {
		goto lua_failed;
	}

	int handle = toluafix_ref_function(L, 3, 0);
	if (handle == 0) {
		goto lua_failed;
	}

	mysql_wrapper::query(context, sql, on_lua_query_cb, (void*)handle);
lua_failed:
	return 0;
}

int 
register_mysql_export(lua_State* L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "MySqlWrapper", 0);
		tolua_beginmodule(L, "MySqlWrapper");
		tolua_function(L, "connect", lua_mysql_connect);
		tolua_function(L, "close_mysql", lua_mysql_close);
		tolua_function(L, "query", lua_mysql_query);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}