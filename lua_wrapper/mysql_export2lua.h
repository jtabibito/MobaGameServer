#ifndef __MYSQL_EXPORT2LUA_H__
#define __MYSQL_EXPORT2LUA_H__

struct lua_State;

int register_mysql_export(lua_State* L);

#endif // !__MYSQL_EXPORT2LUA_H__