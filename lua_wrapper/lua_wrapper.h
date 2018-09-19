#ifndef __LUA_WRAPPER_H__
#define __LUA_WRAPPER_H__

#include "lua.hpp"
#include <string>

class lua_wrapper {
public: 
	static void init();
	static void exit();

	static bool entry_lua_file(std::string& lua_file);	// 定义文件入口
	static void add_search_path(std::string& path);
	static lua_State* lua_state();
public:
	static void reg_func2lua(const char* name, int (*lua_cfunc)(lua_State* L));

public:
	static int executeFunctionByHandle(int nHandle, int numArgs);
	static void removeByHandle(int nHandle);
};

#endif // !__LUA_WRAPPER_H__