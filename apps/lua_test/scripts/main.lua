-- 初始化日志module
Logger.init("../../apps/logs", "RunningLog", true)
-- end

-- 初始化协议module
local proto_type = {
	PROTO_JSON = 0,
	PROTO_BUF = 1,
}
ProtoMan.init(proto_type.PROTO_BUF)
-- end
if ProtoMan.proto_type()==proto_type.PROTO_BUF then
	local cmd_name_map = require("cmd_name_map")
	if cmd_name_map then
		ProtoMan.register_protobuf_cmd_map(cmd_name_map)
	end
end

-- 初始化网络端口
Netbus.tcp_listen(6080)
Netbus.udp_listen(6081)
Netbus.ws_listen(6082)
-- end

print("---START SERVER: SUCCESSED")

--local echo_server = require("echo_server")
--service.register(echo_server.stype, echo_server.service);

local talkingRoom = require("talkingRoom")
local ret = Service.register(talkingRoom.stype, talkingRoom.service)
if ret then
	print("---REGISTE TALKINGROOM SERVICE: SUCCESSED")
else
	print("---REGISTE TALKINGROOM SERVICE: DEFAULT")
end
