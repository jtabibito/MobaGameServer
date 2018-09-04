#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../db/redis_wrapper.h"
#include "redis_export2lua.h"
#include "lua_wrapper.h"

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
lua_redis_connect(lua_State* L) {
	// 拿到lua脚本中的参数
	char* ip = (char*)tolua_tostring(L, 1, NULL);
	if (ip == NULL) {
		goto lua_failed;
	}

	int port = (int)tolua_tonumber(L, 2, 0);

	int handle = toluafix_ref_function(L, 3, 0);

	redis_wrapper::connect(ip, port, on_open_cb, (void*)handle);

lua_failed:
	return 0;
}

static int
lua_redis_close(lua_State* L) {
	void* context = tolua_touserdata(L, 1, 0);
	if (context) {
		redis_wrapper::close_redis(context);
	}
	return 0;
}

static void
pushRedisResult2lua(redisReply* result) {
	switch (result->type) {
	case REDIS_REPLY_STRING: {
		lua_pushstring(lua_wrapper::lua_state(), result->str);
		break;
	}
	case REDIS_REPLY_STATUS: {
		lua_pushstring(lua_wrapper::lua_state(), result->str);
		break;
	}
	case REDIS_REPLY_INTEGER: {
		lua_pushinteger(lua_wrapper::lua_state(), result->integer);
		break;
	}
	case REDIS_REPLY_NIL: {
		lua_pushnil(lua_wrapper::lua_state());
		break;
	}
	case REDIS_REPLY_ARRAY: {
		lua_newtable(lua_wrapper::lua_state());
		int index = 1;
		for (int i = 0; i < result->elements; i++) {
			pushRedisResult2lua(result->element[i]);
			lua_rawseti(lua_wrapper::lua_state(), -2, index);
			++index;
		}
	}
	}
}

static void
on_lua_query_cb(const char* err, redisReply* result, void* udata) {
	if (err) {
		lua_pushstring(lua_wrapper::lua_state(), err);
		lua_pushnil(lua_wrapper::lua_state());
	} else {
		lua_pushnil(lua_wrapper::lua_state());
		if (result) { // 把result push to lua
			pushRedisResult2lua(result);
		} else {
			lua_pushnil(lua_wrapper::lua_state());
		}
	}

	lua_wrapper::executeFunctionByHandle((int)udata, 2);
	lua_wrapper::removeByHandle((int)udata);
}

static int
lua_redis_query(lua_State* L) {
	void* context = tolua_touserdata(L, 1, 0);
	if (!context) {
		goto lua_failed;
	}
	
	char* cmd = (char*)tolua_tostring(L, 2, 0);
	if (cmd == NULL) {
		goto lua_failed;
	}

	int handle = toluafix_ref_function(L, 3, 0);
	if (handle == 0) {
		goto lua_failed;
	}

	redis_wrapper::query(context, cmd, on_lua_query_cb, (void*)handle);
lua_failed:
	return 0;
}

int 
register_redis_export(lua_State* L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "redis_wrapper", 0);
		tolua_beginmodule(L, "redis_wrapper");
		tolua_function(L, "connect", lua_redis_connect);
		tolua_function(L, "close_redis", lua_redis_close);
		tolua_function(L, "query", lua_redis_query);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}
