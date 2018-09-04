#ifndef __REDIS_EXPORT2LUA_H__
#define __REDIS_EXPORT2LUA_H__

struct lua_State;
int register_redis_export(lua_State* L);

#endif // !__REDIS_EXPORT2LUA_H__