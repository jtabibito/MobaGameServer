#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/logManager.h"

#include "lua_wrapper.h"

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

static void do_log_message(void(*log)(const char* file_name, const char* msg, int line_num), const char* msg) {	lua_Debug info;	int depth = 0;	while (lua_getstack(g_lua_state, depth, &info)) {		lua_getinfo(g_lua_state, "S", &info);		lua_getinfo(g_lua_state, "n", &info);		lua_getinfo(g_lua_state, "l", &info);		if (info.source[0] == '@') {			log(&info.source[1], msg, info.currentline);			return;		}		++depth;	}	if (depth == 0) {		log("trunk", msg, 0);	}}

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


void 
lua_wrapper::init() {
	// init lua virtual machine
	g_lua_state = luaL_newstate();
	lua_atpanic(g_lua_state, lua_panic);

	// registe base libs for lua virtual machine
	luaL_openlibs(g_lua_state);

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