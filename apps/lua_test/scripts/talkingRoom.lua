-- 保存客户端session
local session_list = {}

function broadcast_except(msg, except_session)
    for idx = 1, #session_list do
        if except_session ~= session_list[idx] then
            Session.send_msg(session_list[idx], msg)
        end
    end
end

function on_recv_login_cmd(s)
    local sip, sport = Session.get_address(s)
    -- 是否在集合中
    for idx = 1, #session_list do
        if s == session_list[idx] then --返回status -1
            print(">>>"..sip..":"..sport.." repeated join the room.")
            local msg = {1, 2, 0, {status = -1}}
            Session.send_msg(s, msg)
            return
        end
    end

    table.insert(session_list, s)
    local msg = {1, 2, 0, {status = 1}}
    Session.send_msg(s, msg)
    -- 广播消息
    
    print(">>>"..sip..":"..sport.." have been join the room.")
    msg = {1, 7, 0, {ip = sip, port = sport}}
    broadcast_except(msg, s)
end

function on_recv_exit_cmd(s)
    local ip, port = Session.get_address(s)
    for idx = 1, #session_list do
        if s == session_list[idx] then --返回status -1
            print("<<<"..ip.." : "..port.." quit the room.")
            table.remove(session_list, i)
            local msg = {1, 4, 0, {status = 1}}
            Session.send_msg(s, msg)
            -- 广播消息
            local sip, sport = Session.get_address(s)
            msg = {1, 8, 0, {ip = sip, port = sport}}
            broadcast_except(msg, s)
            return
        end
    end

    -- 不在聊天室
    local msg = {1, 4, 0, {status = -1}}
    Session.send_msg(s, msg)
end

function on_recv_sendmsg_cmd(s, str)
    for idx = 1, #session_list do
        if s == session_list[idx] then
            local msg = {1, 6, 0, {status = 1}}
            Session.send_msg(s, msg)
            -- 广播消息
            local sip, sport = Session.get_address(s)
            msg = {1, 9, 0, {ip = sip, port = sport, content = str}}
            broadcast_except(msg, s)
            return
        end
    end

    local msg = {1, 6, 0, {status = -1}}
    Session.send_msg(s, msg)
end

-- {stype, ctype, utag, msgbody}
function on_trm_recv_cmd(s, msg)
    local ctype = msg[2];
    local body = msg[4]
    if ctype == 1 then 
        on_recv_login_cmd(s)
    elseif ctype == 3 then
        on_recv_exit_cmd(s)
    elseif ctype == 5 then
        on_recv_sendmsg_cmd(s, body.content)
    end
end

function on_trm_session_disconnect(s)
    local ip, port = Session.get_address(s)
    for idx = 1, #session_list do
        if s == session_list[idx] then --返回status -1
            print("<<<"..ip.." : "..port.." have been disconnected with server.")
            table.remove(session_list, i)
            -- 广播消息
            local sip, sport = Session.get_address(s)
            msg = {1, 8, 0, {ip = sip, port = sport}}
            broadcast_except(msg, s)
            return
        end
    end
end

local trm_service = {
    on_session_recv_cmd = on_trm_recv_cmd,
    on_session_disconnect = on_trm_session_disconnect,
}

local talkingRoom = {
    stype = 1,
    service = trm_service,
}

return talkingRoom