#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../netbus/netbus.h"

#include "lua_wrapper.h"
#include "netbus_export2lua.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"

static int
lua_tcp_listen(lua_State* tolua_S) {
	int argc = lua_gettop(tolua_S);
	if (argc != 1) {
		goto lua_failed;
	}

	int port = (int)lua_tointeger(tolua_S, 1);
	netbus::instance()->tcp_listen(port);
lua_failed:
	return 0;
}

static int
lua_udp_listen(lua_State* tolua_S) {
	int argc = lua_gettop(tolua_S);
	if (argc != 1) {
		goto lua_failed;
	}

	int port = (int)lua_tointeger(tolua_S, 1);
	netbus::instance()->udp_listen(port);
lua_failed:
	return 0;
}

static int
lua_ws_listen(lua_State* tolua_S) {
	int argc = lua_gettop(tolua_S);
	if (argc != 1) {
		goto lua_failed;
	}

	int port = (int)lua_tointeger(tolua_S, 1);
	netbus::instance()->ws_listen(port);
lua_failed:
	return 0;
}

int 
register_netbus_export(lua_State* L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Netbus", 0);
		tolua_beginmodule(L, "Netbus");

		tolua_function(L, "tcp_listen", lua_tcp_listen);
		tolua_function(L, "udp_listen", lua_udp_listen);
		tolua_function(L, "ws_listen", lua_ws_listen);

		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}

