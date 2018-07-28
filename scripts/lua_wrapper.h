#ifndef __LUA_WRAPPER_H__
#define __LUA_WRAPPER_H__

#include "lua.hpp"

class lua_wrapper {
public: 
	static void init();
	static void exit();

	static bool entry_lua_file(const char* lua_file);	// 定义文件入口
};

#endif // !__LUA_WRAPPER_H__