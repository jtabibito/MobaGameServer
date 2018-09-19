#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "../utils/logManager.h"

#include "tolua_fix.h"
#include "lua_wrapper.h"
#include "mysql_export2lua.h"
#include "redis_export2lua.h"
#include "service_export2lua.h"
#include "session_export2lua.h"
#include "scheduler_export2lua.h"
#include "netbus_export2lua.h"
#include "protoman_export2lua.h"

lua_State* g_lua_state = NULL;

static void 
print_error(const char* fname,
	const char* msg, 
	int line_number) {
	logger::write2log(fname, line_number, WRONG, msg);
}

static void 
print_warning(const char* fname,
	const char* msg, 
	int line_number) {
	logger::write2log(fname, line_number, WARNING, msg);
}

static void 
print_debug(const char* fname,
	const char* msg, 
	int line_number) {
	logger::write2log(fname, line_number, DEBUG, msg);
}

static void 
do_log_message(void(*log)(const char* file_name, const char* msg, int line_num), const char* msg) {
	lua_Debug info;
	int depth = 0;
	while (lua_getstack(g_lua_state, depth, &info)) {

		lua_getinfo(g_lua_state, "S", &info);
		lua_getinfo(g_lua_state, "n", &info);
		lua_getinfo(g_lua_state, "l", &info);

		if (info.source[0] == '@') {
			log(&info.source[1], msg, info.currentline);
			return;
		}

		++depth;
	}
	if (depth == 0) {
		log("trunk", msg, 0);
	}
}

static int 
lua_log_debug(lua_State* L) {
	// 返回栈中元素个数->可变参数个数
	int nArgs = lua_gettop(L);

	std::string t;

	for (int i = 1; i <= nArgs; i++) {
		if (lua_istable(L, i)) {
			t += "table";
		} else if (lua_isnone(L, i)) {
			t += "none";
		} else if (lua_isnil(L, i)) {
			t += "nil";
		} else if (lua_isboolean(L, i)) {
			if (lua_toboolean(L, i) != 0) {
				t += "true";
			} else {
				t += "false";
			}
		} else if (lua_isfunction(L, i)) {
			t += "function";
		} else if (lua_islightuserdata(L, i)) {
			t += "lightuserdata";
		} else if (lua_isthread(L, i)) {
			t += "thread";
		} else {
			const char* str = lua_tostring(L, i);
			if (str) { // lua调用信息 file name and line
				t += lua_tostring(L, i);
			} else {
				t += lua_typename(L, lua_type(L, i));
			}
		}
		if (i != nArgs) {
			t += "\t";
		}
	}

	do_log_message(print_debug, t.c_str());
	return 0;
}

static int 
lua_log_warning(lua_State* L) {
	// 返回栈中元素个数->可变参数个数
	int nArgs = lua_gettop(L);

	std::string t;

	for (int i = 1; i <= nArgs; i++) {
		if (lua_istable(L, i)) {
			t += "table";
		} else if (lua_isnone(L, i)) {
			t += "none";
		} else if (lua_isnil(L, i)) {
			t += "nil";
		} else if (lua_isboolean(L, i)) {
			if (lua_toboolean(L, i) != 0) {
				t += "true";
			} else {
				t += "false";
			}
		} else if (lua_isfunction(L, i)) {
			t += "function";
		} else if (lua_islightuserdata(L, i)) {
			t += "lightuserdata";
		} else if (lua_isthread(L, i)) {
			t += "thread";
		} else {
			const char* str = lua_tostring(L, i);
			if (str) { // lua调用信息 file name and line
				t += lua_tostring(L, i);
			} else {
				t += lua_typename(L, lua_type(L, i));
			}
		}
		if (i != nArgs) {
			t += "\t";
		}
	}

	do_log_message(print_warning, t.c_str());
	return 0;
}


static int 
lua_log_error(lua_State* L) {
	// 返回栈中元素个数->可变参数个数
	int nArgs = lua_gettop(L);

	std::string t;

	for (int i = 1; i <= nArgs; i++) {
		if (lua_istable(L, i)) {
			t += "table";
		} else if (lua_isnone(L, i)) {
			t += "none";
		} else if (lua_isnil(L, i)) {
			t += "nil";
		} else if (lua_isboolean(L, i)) {
			if (lua_toboolean(L, i) != 0) {
				t += "true";
			} else {
				t += "false";
			}
		} else if (lua_isfunction(L, i)) {
			t += "function";
		} else if (lua_islightuserdata(L, i)) {
			t += "lightuserdata";
		} else if (lua_isthread(L, i)) {
			t += "thread";
		} else {
			const char* str = lua_tostring(L, i);
			if (str) { // lua调用信息 file name and line
				t += lua_tostring(L, i);
			} else {
				t += lua_typename(L, lua_type(L, i));
			}
		}
		if (i != nArgs) {
			t += "\t";
		}
	}

	do_log_message(print_error, t.c_str());
	return 0;
}


static int
lua_panic(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg) {
		do_log_message(print_error, msg);
	}
	return 0;
}

lua_State*
lua_wrapper::lua_state() {
	return g_lua_state;
}

static int
lua_logger_init(lua_State* L) {
	const char* path = lua_tostring(L, 1);
	if (path == NULL) {
		goto lua_failed;
	}

	const char* prefix = lua_tostring(L, 2);
	if (prefix == NULL) {
		goto lua_failed;
	}

	bool std_out = lua_toboolean(L, 3);
	logger::init((char*)path, (char*)prefix, std_out);

lua_failed:
	return 0;
}

static int
register_logger_export(lua_State* L) {
	lua_wrapper::reg_func2lua("print", lua_log_debug);

	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Logger", 0);
		tolua_beginmodule(L, "Logger");

		tolua_function(L, "debug", lua_log_debug);
		tolua_function(L, "warning", lua_log_warning);
		tolua_function(L, "error", lua_log_error);
		tolua_function(L, "init", lua_logger_init);

		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}

static int
lua_add_search_path(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	if (path) {
		std::string str = path;
		lua_wrapper::add_search_path(str);
	}
	return 0;
}

void 
lua_wrapper::init() {
	// init lua virtual machine
	g_lua_state = luaL_newstate();
	lua_atpanic(g_lua_state, lua_panic);

	// registe base libs for lua virtual machine
	luaL_openlibs(g_lua_state);
	toluafix_open(g_lua_state);
	lua_wrapper::reg_func2lua("add_search_path", lua_add_search_path);

	register_mysql_export(g_lua_state);
	register_redis_export(g_lua_state);
	register_service_export(g_lua_state);
	register_session_export(g_lua_state);
	register_logger_export(g_lua_state);
	register_scheduler_export(g_lua_state);
	register_netbus_export(g_lua_state);
	register_protoman_export(g_lua_state);

	// Here: to export lua function
	lua_wrapper::reg_func2lua("log_error", lua_log_error);
	lua_wrapper::reg_func2lua("log_debug", lua_log_debug);
	lua_wrapper::reg_func2lua("log_warning", lua_log_warning);
	
}

void 
lua_wrapper::exit() {
	if (g_lua_state != NULL) {
		lua_close(g_lua_state);
		g_lua_state = NULL;
	}
}

bool
lua_wrapper::entry_lua_file(std::string& lua_file) {
	if (luaL_dofile(g_lua_state, lua_file.c_str())) {
		lua_log_error(g_lua_state);
		return false;
	}
	return true;
}

void 
lua_wrapper::reg_func2lua(const char* name, 
	int(*lua_cfunc)(lua_State* L)) {
	lua_pushcfunction(g_lua_state, lua_cfunc);
	// 入栈全局变量
	lua_setglobal(g_lua_state, name);
}

static bool
pushFunctionByHandle(int  nHandle) {
	toluafix_get_function_by_refid(g_lua_state, nHandle);
	if (!lua_isfunction(g_lua_state, -1)) {
		log_error("[ lua ERROR ] function refid '%d' does not reference a lua function", nHandle);
		lua_pop(g_lua_state, 1);
		return false;
	}
	return true;
}

static int
executeFunction(int numArgs) {
	int functionIndex = -(numArgs + 1);
	if (!lua_isfunction(g_lua_state, functionIndex)) {
		log_error("value at stack [%d] is not function", functionIndex);
		lua_pop(g_lua_state, numArgs + 1); // remove function and arguments
		return 0;
	}
	
	int traceback = 0;
	lua_getglobal(g_lua_state, "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
	if (!lua_isfunction(g_lua_state, -1)) { 
		lua_pop(g_lua_state, 1);                                            /* L: ... func arg1 arg2 ... */
	} else {
		lua_insert(g_lua_state, functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
		traceback = functionIndex - 1;
	}

	int error = 0;
	error = lua_pcall(g_lua_state, numArgs, 1, traceback);                  /* L: ... [G] ret */
	if (error) { 
		if (traceback == 0) { 
			log_error("[LUA ERROR] %s", lua_tostring(g_lua_state, -1));        /* L: ... error */
			lua_pop(g_lua_state, 1); // remove error message from stack
		} 
		else                                                            /* L: ... G error */
		{ 
			lua_pop(g_lua_state, 2); // remove __G__TRACKBACK__ and error message from stack
		}
		return 0;
	}
	
	// get return value
	int ret = 0;
	if (lua_isnumber(g_lua_state, -1)) { 
		ret = (int)lua_tointeger(g_lua_state, -1);
	} else if (lua_isboolean(g_lua_state, -1)) {
		ret = (int)lua_toboolean(g_lua_state, -1);
	} // remove return value from stack
	lua_pop(g_lua_state, 1);                                                /* L: ... [G] */
	if (traceback) {
		lua_pop(g_lua_state, 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
	}

	return ret;
}

int 
lua_wrapper::executeFunctionByHandle(int nHandle, int numArgs) {
	int ret = 0;
	if (pushFunctionByHandle(nHandle)) {
		if (numArgs > 0) {
			lua_insert(g_lua_state, -(numArgs + 1));
		}
		
		ret = executeFunction(numArgs);
	}
	lua_settop(g_lua_state, 0);
	return ret;
}

void 
lua_wrapper::removeByHandle(int nHandle) {
	toluafix_remove_function_by_refid(g_lua_state, nHandle);
}

void
lua_wrapper::add_search_path(std::string& path) {
	char strPath[1024] = { 0 };
	sprintf(strPath, "local path = string.match([[%s]],[[(.*)/[^/]*$]])\n package.path = package.path .. [[;]] .. path .. [[/?.lua;]] .. path .. [[/?/init.lua]]\n", path.c_str());
	luaL_dostring(g_lua_state, strPath);
}