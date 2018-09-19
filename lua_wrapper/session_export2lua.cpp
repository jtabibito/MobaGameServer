#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua_wrapper.h"
#include "../netbus/session.h"
#include "../utils/logManager.h"
#include "../netbus/proto_man.h"
#include "google/protobuf/message.h"
using namespace google::protobuf;

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"

// session.close(session)
static int
lua_session_close(lua_State* tolua_S) {
	session* s = (session*)tolua_touserdata(tolua_S, 1, NULL);
	if (s == NULL) {
		goto lua_failed;
	}

	s->close();

lua_failed:
	return 0;
}

static google::protobuf::Message*
lua_table2protobuf(lua_State* L,
	int stackIdx,
	const char* msg_name) {
	if (!lua_istable(L, stackIdx)) {
		log_error("[LUA ERROR] '%d' is not a lua table type", stackIdx);
		return NULL;
	}

	Message* message = proto_man::create_message(msg_name);
	if (!message) {
		log_error("can not find message '%s'", msg_name);
		return NULL;
	}

	const Descriptor* descriptor = message->GetDescriptor();
	const Reflection* reflection = message->GetReflection();

	for (int32_t index = 0; index < descriptor->field_count(); ++index) {
		const FieldDescriptor* fd = descriptor->field(index);
		const string& name = fd->name();

		bool isRequired = fd->is_required();
		bool bRepeated = fd->is_repeated();
		lua_pushstring(L, name.c_str());
		lua_rawget(L, stackIdx);

		bool isNil = lua_isnil(L, -1);
		if (bRepeated) {
			if (isNil) {
				lua_pop(L, 1);
				continue;
			} else {
				bool isTable = lua_istable(L, -1);
				if (!isTable) {
					log_error("[LUA ERROR] '%s' is not a table", name.c_str());
					proto_man::release_message(message);
					return NULL;
				}
			}

			lua_pushnil(L);
			for (; lua_next(L, -2) != 0;) {
				switch (fd->cpp_type()) {
				case FieldDescriptor::CPPTYPE_DOUBLE: {
					double value = luaL_checknumber(L, -1);
					reflection->AddDouble(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_FLOAT: {
					float value = luaL_checknumber(L, -1);
					reflection->AddFloat(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_INT64:
				{
					int64_t value = luaL_checknumber(L, -1);
					reflection->AddInt64(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_UINT64:
				{
					uint64_t value = luaL_checknumber(L, -1);
					reflection->AddUInt64(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_ENUM: // ��int32һ������
				{
					int32_t value = luaL_checknumber(L, -1);
					const EnumDescriptor* enumDescriptor = fd->enum_type();
					const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
					reflection->AddEnum(message, fd, valueDescriptor);
					break;
				}
				case FieldDescriptor::CPPTYPE_INT32:
				{
					int32_t value = luaL_checknumber(L, -1);
					reflection->AddInt32(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_UINT32:
				{
					uint32_t value = luaL_checknumber(L, -1);
					reflection->AddUInt32(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_STRING:
				{
					size_t size = 0;
					const char* value = luaL_checklstring(L, -1, &size);
					reflection->AddString(message, fd, std::string(value, size));
					break;
				}
				case FieldDescriptor::CPPTYPE_BOOL:
				{
					bool value = lua_toboolean(L, -1);
					reflection->AddBool(message, fd, value);
					break;
				}
				case FieldDescriptor::CPPTYPE_MESSAGE:
				{
					Message* value = lua_table2protobuf(L, lua_gettop(L), fd->message_type()->name().c_str());
					if (!value) {
						log_error("convert to message %s failed whith value %s\n", fd->message_type()->name().c_str(), name.c_str());
						proto_man::release_message(value);
						return NULL;
					}
					Message* msg = reflection->AddMessage(message, fd);
					msg->CopyFrom(*value);
					proto_man::release_message(value);
					break;
				}
				default:
					break;
				}

				// remove value�� keep the key
				lua_pop(L, 1);
			}
		} else {
			if (isRequired) {
				if (isNil) {
					log_error("can not find required field %s\n", name.c_str());
					proto_man::release_message(message);
					return NULL;
				}
			} else {
				if (isNil) {
					lua_pop(L, 1);
					continue;
				}
			}
			switch (fd->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				double value = luaL_checknumber(L, -1);
				reflection->SetDouble(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_FLOAT:
			{
				float value = luaL_checknumber(L, -1);
				reflection->SetFloat(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_INT64:
			{
				int64_t value = luaL_checknumber(L, -1);
				reflection->SetInt64(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT64:
			{
				uint64_t value = luaL_checknumber(L, -1);
				reflection->SetUInt64(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_ENUM: // ��int32һ������
			{
				int32_t value = luaL_checknumber(L, -1);
				const EnumDescriptor* enumDescriptor = fd->enum_type();
				const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
				reflection->SetEnum(message, fd, valueDescriptor);
				break;
			}
			case FieldDescriptor::CPPTYPE_INT32:
			{
				int32_t value = luaL_checknumber(L, -1);
				reflection->SetInt32(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT32:
			{
				uint32_t value = luaL_checknumber(L, -1);
				reflection->SetUInt32(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING:
			{
				size_t size = 0;
				const char* value = luaL_checklstring(L, -1, &size);
				reflection->SetString(message, fd, std::string(value, size));
				break;
			}
			case FieldDescriptor::CPPTYPE_BOOL:
			{
				bool value = lua_toboolean(L, -1);
				reflection->SetBool(message, fd, value);
				break;
			}
			case FieldDescriptor::CPPTYPE_MESSAGE:
			{
				Message* value = lua_table2protobuf(L, lua_gettop(L), fd->message_type()->name().c_str());
				if (!value) {
					log_error("convert to message %s failed whith value %s \n", fd->message_type()->name().c_str(), name.c_str());
					proto_man::release_message(message);
					return NULL;
				}
				Message* msg = reflection->MutableMessage(message, fd);
				msg->CopyFrom(*value);
				proto_man::release_message(value);
				break;
			}
			default:
				break;
			}
		}

		// pop value
		lua_pop(L, 1);
	}

	return message;
}

// {1: stype, 2: ctype, 3: utag, 4: body}
static int
lua_send_msg(lua_State* tolua_S) {
	session* s = (session*)tolua_touserdata(tolua_S, 1, 0);
	if (s == NULL) {
		goto lua_failed;
	}

	// stack: 1: session, 2: table
	if (!lua_istable(tolua_S, 2)) {
		goto lua_failed;
	}

	struct cmd_msg msg;

	int n = luaL_len(tolua_S, 2);
	if (n != 4) {
		goto lua_failed;
	}

	lua_pushnumber(tolua_S, 1);
	lua_gettable(tolua_S, 2);
	msg.stype = luaL_checkinteger(tolua_S, -1);

	lua_pushnumber(tolua_S, 2);
	lua_gettable(tolua_S, 2);
	msg.ctype = luaL_checkinteger(tolua_S, -1);

	lua_pushnumber(tolua_S, 3);
	lua_gettable(tolua_S, 2);
	msg.utag = luaL_checkinteger(tolua_S, -1);

	lua_pushnumber(tolua_S, 4);
	lua_gettable(tolua_S, 2);
	if (proto_man::proto_type() == PROTO_JSON) {
		msg.body = (char*)lua_tostring(tolua_S, -1);
		s->send_msg(&msg);
	} else {
		if (!lua_istable(tolua_S, -1)) {
			msg.body = NULL;
		} else {
			const char* msg_name = proto_man::pb_cmd_ctype2name(msg.ctype);
			msg.body = lua_table2protobuf(tolua_S, lua_gettop(tolua_S), msg_name);
			s->send_msg(&msg);
			proto_man::release_message((google::protobuf::Message*)msg.body);
		}
	}

lua_failed:
	return 0;
}

static int
lua_get_addr(lua_State* tolua_S) {
	session* s = (session*)tolua_touserdata(tolua_S, 1, NULL);
	if (s == NULL) {
		goto lua_failed;
	}

	int client_port;
	const char* ip = s->get_address(&client_port);

	lua_pushstring(tolua_S, ip);
	lua_pushinteger(tolua_S, client_port);
	return 2;
lua_failed:
	return 0;
}

int 
register_session_export(lua_State* L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Session", 0);
		tolua_beginmodule(L, "Session");

		tolua_function(L, "close", lua_session_close);
		tolua_function(L, "send_msg", lua_send_msg);
		tolua_function(L, "get_address", lua_get_addr);

		tolua_endmodule(L);
	}
	lua_pop(L, 1);

	return 0;
}
