#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/logManager.h"

#include "tolua_fix.h"
#include "lua_wrapper.h"
#include "mysql_export2lua.h"
#include "redis_export2lua.h"

lua_State* g_lua_state = NULL;

static void 
print_error(const char* fname,
	const char* msg, 
	int line_number) {
	log::write2log(fname, line_number, WRONG, msg);
}

static void 
print_warning(const char* fname,
	const char* msg, 
	int line_number) {
	log::write2log(fname, line_number, WARNING, msg);
}

static void 
print_debug(const char* fname,
	const char* msg, 
	int line_number) {
	log::write2log(fname, line_number, DEBUG, msg);
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
	const char* msg = luaL_checkstring(L, -1);
	if (msg) { // lua调用信息 file name and line
		do_log_message(print_debug, msg);
	}
	return 0;
}

static int 
lua_log_warning(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg) { // lua调用信息 file name and line
		do_log_message(print_warning, msg);
	}
	return 0;
}


static int 
lua_log_error(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg) { // lua调用信息 file name and line
		do_log_message(print_error, msg);
	}
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

void 
lua_wrapper::init() {
	// init lua virtual machine
	g_lua_state = luaL_newstate();
	lua_atpanic(g_lua_state, lua_panic);

	// registe base libs for lua virtual machine
	luaL_openlibs(g_lua_state);
	toluafix_open(g_lua_state);

	register_mysql_export(g_lua_state);
	register_redis_export(g_lua_state);

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
lua_wrapper::entry_lua_file(const char* lua_file) {
	if (luaL_dofile(g_lua_state, lua_file)) {
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
