#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua_wrapper.h"
#include "service_export2lua.h"
#include "../netbus/service.h"
#include "../netbus/service_man.h"
#include "../utils/logManager.h"
#include "../netbus/proto_man.h"
#include "google\protobuf\message.h"
using namespace google::protobuf;

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"

#define SERVICE_FUNCTION_MAPPING "service_function_mapping"

static void
init_service_function_map(lua_State* L) {
	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
}

static unsigned int s_function_ref_id = 0;
static unsigned int
save_service_function(lua_State* L, int lo, int def) {
	// function at lo
	if (!lua_isfunction(L, lo)) return 0;

	s_function_ref_id++;

	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: fun ... refid_fun */
	lua_pushinteger(L, s_function_ref_id);                      /* stack: fun ... refid_fun refid */
	lua_pushvalue(L, lo);                                       /* stack: fun ... refid_fun refid fun */

	lua_rawset(L, -3);                  /* refid_fun[refid] = fun, stack: fun ... refid_ptr */
	lua_pop(L, 1);                                              /* stack: fun ... */

	return s_function_ref_id;

	// lua_pushvalue(L, lo);                                           /* stack: ... func */
	// return luaL_ref(L, LUA_REGISTRYINDEX);
}

static void
get_service_function(lua_State* L, int refid) {
	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: ... refid_fun */
	lua_pushinteger(L, refid);                                  /* stack: ... refid_fun refid */
	lua_rawget(L, -2);                                          /* stack: ... refid_fun fun */
	lua_remove(L, -2);                                          /* stack: ... fun */
}

static bool
push_service_function(int nHandle) {
	get_service_function(lua_wrapper::lua_state(), nHandle);
	if (!lua_isfunction(lua_wrapper::lua_state(), -1)) {
		log_error("[LUA ERROR] function refid '%d' does not reference lua function", nHandle);
		lua_pop(lua_wrapper::lua_state(), nHandle);
		return false;
	}
	return true;
}

static int
exe_function(int numArgs) {
	int functionIdx = -(numArgs + 1);

	if (!lua_isfunction(lua_wrapper::lua_state(), functionIdx)) {
		log_error("[LUA ERROR] function refid '%d' does not reference lua function", functionIdx);
		lua_pop(lua_wrapper::lua_state(), numArgs + 1);
		return 0;
	}

	int traceback = 0;
	lua_getglobal(lua_wrapper::lua_state(), "__G__TRACKBACK__");
	if (!lua_isfunction(lua_wrapper::lua_state(), -1)) {
		lua_pop(lua_wrapper::lua_state(), 1);
	} else {
		lua_insert(lua_wrapper::lua_state(), functionIdx - 1);
		traceback = functionIdx - 1;
	}

	int error = 0;
	error = lua_pcall(lua_wrapper::lua_state(), numArgs, 1, traceback);
	if (error) {
		if (traceback == 0) {
			log_error("[LUA ERROR] %s", lua_tostring(lua_wrapper::lua_state(), -1));
			lua_pop(lua_wrapper::lua_state(), 1);
		} else {
			lua_pop(lua_wrapper::lua_state(), 2);
		}
		return 0;
	}

	int ret = 0;
	if (lua_isnumber(lua_wrapper::lua_state(), -1)) {
		ret = (int)lua_tointeger(lua_wrapper::lua_state(), -1);
	} else if (lua_isboolean(lua_wrapper::lua_state(), -1)) {
		ret = (int)lua_toboolean(lua_wrapper::lua_state(), -1);
	}
	lua_pop(lua_wrapper::lua_state(), 1);

	if (traceback) {
		lua_pop(lua_wrapper::lua_state(), 1);
	}

	return ret;
}

static int
execute_service_function(int nHandle, int numArgs) {
	int ret = 0;
	if (push_service_function(nHandle)) {
		if (numArgs > 0) {
			lua_insert(lua_wrapper::lua_state(), -(numArgs + 1));
		}
		ret = exe_function(numArgs);
	}
	lua_settop(lua_wrapper::lua_state(), 0);
	return ret;
}

class lua_service : public service {
public:
	unsigned int lua_recv_cmd_handle;
	unsigned int lua_disconnect_handle;
public:
	virtual bool on_session_recv_cmd(session* s, struct cmd_msg* msg);
	virtual void on_session_disconnect(session* s);
};

static void
push_proto_message2lua(const Message* message) {
	lua_State* state = lua_wrapper::lua_state();
	if (!message) {
		return;
	}

	const Reflection* reflection = message->GetReflection();

	lua_newtable(state);

	const Descriptor* descriptor = message->GetDescriptor();
	for (int32_t index = 0; index < descriptor->field_count(); ++index) {
		const FieldDescriptor* fd = descriptor->field(index);
		const std::string& name = fd->lowercase_name();

		lua_pushstring(state, name.c_str());

		bool bRepeated = fd->is_repeated();
		if (bRepeated) {
			lua_newtable(state);
			int size = reflection->FieldSize(*message, fd);
			for (int i = 0; i < size; ++i) {
				char str[32] = { 0 };
				switch (fd->cpp_type()) {
				case FieldDescriptor::CPPTYPE_DOUBLE: {
					lua_pushnumber(state, reflection->GetRepeatedDouble(*message, fd, i));
					break;
				}
				case FieldDescriptor::CPPTYPE_FLOAT: {
					lua_pushnumber(state, reflection->GetRepeatedFloat(*message, fd, i));
					break;
				}
				case FieldDescriptor::CPPTYPE_INT64: {
					sprintf(str, "%lld", (long long)reflection->GetRepeatedInt64(*message, fd, i));
					lua_pushstring(state, str);
					break;
				}
				case FieldDescriptor::CPPTYPE_UINT64: {
					sprintf(str, "llu", (unsigned long long)reflection->GetRepeatedUInt64(*message, fd, i));
					lua_pushstring(state, str);
					break;
				}
				case FieldDescriptor::CPPTYPE_ENUM: {
					lua_pushinteger(state, reflection->GetRepeatedEnum(*message, fd, i)->number());
					break;
				}
				case FieldDescriptor::CPPTYPE_INT32: {
					lua_pushinteger(state, reflection->GetRepeatedInt32(*message, fd, i));
					break;
				}
				case FieldDescriptor::CPPTYPE_UINT32: {
					lua_pushinteger(state, reflection->GetRepeatedUInt32(*message, fd, i));
					break;
				}
				case FieldDescriptor::CPPTYPE_STRING: {
					std::string vector = reflection->GetRepeatedString(*message, fd, i);
					lua_pushlstring(state, vector.c_str(), vector.size());
					break;
				}
				case FieldDescriptor::CPPTYPE_BOOL: {
					lua_pushboolean(state, reflection->GetRepeatedBool(*message, fd, i));
					break;
				}
				case FieldDescriptor::CPPTYPE_MESSAGE: {
					push_proto_message2lua(&(reflection->GetRepeatedMessage(*message, fd, i)));
					break;
				}
				default:
					break;
				}
				lua_rawseti(state, -2, i + 1);
			}
		} else {
			char str[32] = { 0 };
			switch (fd->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE: {
				lua_pushnumber(state, reflection->GetDouble(*message, fd));
				break;
			}
			case FieldDescriptor::CPPTYPE_FLOAT: {
				lua_pushnumber(state, reflection->GetFloat(*message, fd));
				break;
			}
			case FieldDescriptor::CPPTYPE_INT64: {
				sprintf(str, "%lld", (long long)reflection->GetInt64(*message, fd));
				lua_pushstring(state, str);
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT64: {
				sprintf(str, "llu", (unsigned long long)reflection->GetUInt64(*message, fd));
				lua_pushstring(state, str);
				break;
			}
			case FieldDescriptor::CPPTYPE_ENUM: {
				lua_pushinteger(state, reflection->GetEnum(*message, fd)->number());
				break;
			}
			case FieldDescriptor::CPPTYPE_INT32: {
				lua_pushinteger(state, reflection->GetInt32(*message, fd));
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT32: {
				lua_pushinteger(state, reflection->GetUInt32(*message, fd));
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING: {
				std::string vector = reflection->GetString(*message, fd);
				lua_pushlstring(state, vector.c_str(), vector.size());
				break;
			}
			case FieldDescriptor::CPPTYPE_BOOL: {
				lua_pushboolean(state, reflection->GetBool(*message, fd));
				break;
			}
			case FieldDescriptor::CPPTYPE_MESSAGE: {
				push_proto_message2lua(&(reflection->GetMessage(*message, fd)));
				break;
			}
			default:
				break;
			}
		}
		lua_rawset(state, -3);
	}
}

// protobuf: message key, value -> lua table
// json: json str -> lua
// {1: stype, 2: ctype, 3: utag, 4: body_table_or_jsonstr}
bool
lua_service::on_session_recv_cmd(session* s, struct cmd_msg* msg) {
	// call lua function here:
	tolua_pushuserdata(lua_wrapper::lua_state(), (void*)s);
	int index = 1;
	lua_newtable(lua_wrapper::lua_state());
	lua_pushinteger(lua_wrapper::lua_state(), msg->stype);
	lua_rawseti(lua_wrapper::lua_state(), -2, index);
	++index;
	lua_pushinteger(lua_wrapper::lua_state(), msg->ctype);
	lua_rawseti(lua_wrapper::lua_state(), -2, index);
	++index;
	lua_pushinteger(lua_wrapper::lua_state(), msg->utag);
	lua_rawseti(lua_wrapper::lua_state(), -2, index);
	++index;

	if (!msg->body) {
		lua_pushnil(lua_wrapper::lua_state());
		lua_rawseti(lua_wrapper::lua_state(), -2, index);
		++index;
	} else {
		if (proto_man::proto_type() == PROTO_JSON) {
			lua_pushstring(lua_wrapper::lua_state(), (char*)msg->body);
		} else { // PROTOBUF
			push_proto_message2lua((Message*)msg->body);
		}
		lua_rawseti(lua_wrapper::lua_state(), -2, index);
		++index;
	}

	execute_service_function(this->lua_recv_cmd_handle, 2);
	return true;
}

void
lua_service::on_session_disconnect(session* s) {
	// call lua function here:
	tolua_pushuserdata(lua_wrapper::lua_state(), (void*)s);
	execute_service_function(this->lua_disconnect_handle, 1);
}

static int 
lua_register_service(lua_State* L) {
	int stype = tolua_tonumber(L, 1, 0);
	bool ret = false;

	if (!lua_istable(L, 2)) {
		goto lua_failed;
	}
	
	unsigned int lua_recv_cmd_handle;
	unsigned int lua_disconnect_handle;

	lua_getfield(L, 2, "on_session_recv_cmd");
	lua_getfield(L, 2, "on_session_disconnect");

	lua_recv_cmd_handle = save_service_function(L, 3, 0);
	lua_disconnect_handle = save_service_function(L, 4, 0);
	if (lua_recv_cmd_handle == 0 || lua_disconnect_handle == 0) {
		goto lua_failed;
	}

	lua_service* ls = new lua_service();
	ls->lua_recv_cmd_handle = lua_recv_cmd_handle;
	ls->lua_disconnect_handle = lua_disconnect_handle;
	ret = service_man::register_service(stype, ls);

lua_failed:
	lua_pushboolean(L, ret ? 1 : 0);
	return 1;
}

int 
register_service_export(lua_State* L) {
	init_service_function_map(L);

	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Service", 0);
		tolua_beginmodule(L, "Service");
		tolua_function(L, "register", lua_register_service);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}
