#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua_wrapper.h"

lua_State* g_lua_state = NULL;

void 
lua_wrapper::init() {
	// init lua virtual machine
	g_lua_state = luaL_newstate();

	// registe base libs for lua virtual machine
	luaL_openlibs(g_lua_state);
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
		return false;
	}
	return true;
}