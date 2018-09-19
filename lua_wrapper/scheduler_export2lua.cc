#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/timelist.h"
#include "lua_wrapper.h"
#include "scheduler_export2lua.h"

#include "../utils/small_alloc.h"

#define my_malloc small_alloc
#define my_free small_free

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"

struct timer_repeat {
	int handle;
	int repeat_count;
};

static void
on_lua_repeat_timer(void* udata) {
	struct timer_repeat* tr = (struct timer_repeat*)udata;
	lua_wrapper::executeFunctionByHandle(tr->handle, 0);
	if (tr->repeat_count == -1) {
		return;
	}

	tr->repeat_count--;
	if (tr->repeat_count <= 0) {
		lua_wrapper::removeByHandle(tr->handle);
		my_free(tr);
		tr = NULL;
	}
}

//
//static void
//on_lua_once_timer(void* udata) {
//	struct timer_repeat* tr = (struct timer_repeat*)udata;
//	lua_wrapper::executeFunctionByHandle(tr->handle, 0);
//	lua_wrapper::removeByHandle(tr->handle);
//	free(tr);
//	tr = NULL;
//}

static int
lua_schedule_repeat(lua_State* tolua_S) {
	int handle = toluafix_ref_function(tolua_S, 1, 0);
	if (handle == 0) {
		goto lua_failed;
	}

	int after_sec = lua_tointeger(tolua_S, 2, 0);
	if (after_sec <= 0) {
		goto lua_failed;
	}

	int repeat_count = lua_tointeger(tolua_S, 3, 0);
	if (repeat_count == 0) {
		goto lua_failed;
	}
	if (repeat_count < 0) {
		repeat_count = -1;
	}

	int repeat_sec = lua_tointeger(tolua_S, 4, 0);
	if (repeat_sec <= 0) {
		repeat_sec = after_sec;
	}

	struct timer_repeat* tr = (struct timer_repeat*)my_malloc(sizeof(struct timer_repeat));
	tr->handle = handle;
	tr->repeat_count = repeat_count;

	struct TimeController* t = schedule_repeat(on_lua_repeat_timer, tr, after_sec, repeat_count, repeat_sec);
	tolua_pushuserdata(tolua_S, t);
	return 1;

lua_failed:
	if (handle != 0) {
		lua_wrapper::removeByHandle(handle);
	}
	lua_pushnil(tolua_S);
	return 1;
}

static int
lua_schedule_once(lua_State* tolua_S) {
	int handle = toluafix_ref_function(tolua_S, 1, 0);
	if (handle == 0) {
		goto lua_failed;
	}

	int after_sec = lua_tointeger(tolua_S, 2, 0);
	if (after_sec <= 0) {
		goto lua_failed;
	}

	struct timer_repeat* tr = (struct timer_repeat*)my_malloc(sizeof(struct timer_repeat));
	tr->handle = handle;
	tr->repeat_count = 1;

	struct TimeController* t = schedule_once(on_lua_repeat_timer, tr, after_sec);
	tolua_pushuserdata(tolua_S, t);
	return 1;

lua_failed:
	if (handle != 0) {
		lua_wrapper::removeByHandle(handle);
	}
	lua_pushnil(tolua_S);
	return 1;
}

static int
lua_schedule_cancel(lua_State* tolua_S) {
	if (!lua_isuserdata(tolua_S, 1)) {
		goto lua_failed;
	}

	struct TimeController* t = (struct TimeController*)lua_touserdata(tolua_S, 1);
	struct timer_repeat* tr = (struct timer_repeat*)get_timer_udata(t);
	lua_wrapper::removeByHandle(tr->handle);
	my_free(tr);
	tr = NULL;

	cancel_timer(t);

lua_failed:
	return 0;
}

int 
register_scheduler_export(lua_State* L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Scheduler", 0);
		tolua_beginmodule(L, "Scheduler");

		tolua_function(L, "schedule", lua_schedule_repeat);
		tolua_function(L, "once", lua_schedule_once);
		tolua_function(L, "cancel", lua_schedule_cancel);

		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}
