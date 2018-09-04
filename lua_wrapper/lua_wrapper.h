#ifndef __LUA_WRAPPER_H__
#define __LUA_WRAPPER_H__

#include "lua.hpp"

class lua_wrapper {
public: 
	static void init();
	static void exit();

	static bool entry_lua_file(const char* lua_file);	// 定义文件入口
	static lua_State* lua_state();
public:
	static void reg_func2lua(const char* name, int (*lua_cfunc)(lua_State* L));

public:
	static int executeFunctionByHandle(int nHandle, int numArgs);
	static void removeByHandle(int nHandle);
};

#endif // !__LUA_WRAPPER_H__