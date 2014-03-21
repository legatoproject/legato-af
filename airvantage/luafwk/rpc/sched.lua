-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------
local sched  = require 'sched'
local common = require 'rpc.common'
local proxy  = require 'rpc.proxy'
local socket = require 'socket'
local checks = require 'checks'

local M = { _NAME = 'rpc.sched' }

M.SPAWN_CMD = [[LUA_PATH='./lua/?.lua;./lua/?/init.lua' LUA_CPATH='./lua/?.so' lua -e 'require"rpc".newclient(nil, %port)' &]]

function common.session_mt :_waitcallresponse (seqnum)
    while true do -- loop until we get a response with the correct seqnum
        local ev, status, res, rseqnum = sched.wait(self, {"response", "error"})
        if ev == "response" then
            if seqnum == rseqnum then
                log("LUARPC", "DEBUG", "Got response to call %i: status=%i", seqnum, status)
                if status == 0 then return unpack(res, 1, res.n)
                elseif status == 1 then
                    local err = res[1]
                    if type(err) == 'string' then err = "Error in remote call:\n"..err end
                    error(err, 2)
                elseif status == 2 then return nil, "function not existing"
                elseif status == 3 then return nil, "not a function"
                else return nil, "bad status "..status end
            else log('LUARPC', 'DEBUG', "Irrelevant response signal %i", rseqnum)
            end
        else return nil, status end
    end
end

--- Loop waiting for peer requests, normally in the background.
function M.loop (session)
    local skt, id = session.skt, session.id
    log("LUARPC", "DETAIL", "new connection: %s-%s", skt:getpeername())
    local err_msg
    while true do
        local header
        header, err_msg = skt :receive (6)
        if not header then break end
        local _, t, seqnum, size = header :unpack (">bbI")
        local payload
        payload, err_msg = skt :receive (size)
        if not payload then break end
        if t == 0 then -- RPC Call
            local index, func_name = proxy.deserialize(session, payload, 1)
            local params = {select(2, proxy.deserialize(session, payload, nil, index))}
            sched.run(common.execute, skt, seqnum, func_name, params)
        else    -- RPC Response
            local _, status = payload:unpack("b")
            -- TODO: offset rather than substring!
            payload = payload:sub(2, -1)
            local res = table.pack(select(2, proxy.deserialize(session, payload)))
            -- dispatch to the proper `:call()`
            sched.signal(session, "response", status, res, seqnum)
            log('LUARPC', 'DEBUG', "Response %i signalled", seqnum)
        end
    end

    skt:close() -- make sure the socket is closed locally, so that a later send on that socket will fail!
    sched.signal(session, "error", err_msg)
    if err_msg then log("LUARPC", err_msg=="closed" and "DEBUG" or "ERROR", "Lua RPC connection closed: %s", err_msg) end
end


local session_index = 0

-- To be exported as rpc.newsession
function M.newsession(skt)
    session_index = session_index + 1
    local self = setmetatable ({seqnum=0, skt=skt, id=session_index}, common.session_mt)
    -- rpc.loop will be provided either by rpc.sched or rpc.nosched
    sched.run(rpc.loop, self)
    return self
end


--- Creates a server, waits for one client to connect. Responds to client
--  requests, and allows to send requests to it.
function M.newserver (address, port)
    checks('?string', '?number')
    local server, skt, err
    server, err = socket.bind (address or "localhost", port or 1999)
    if not server then return nil, err end
    skt, err = server :accept()
    if not skt then return nil, err end
    server :close() -- close the server socket as we do need it anymore
    return M.newsession(skt)
end

--- Creates a passive server for multiple clients. Responds to client requests,
--  but won't allow to send commands to clients.
function M.newmultiserver (address, port)
    checks('?string', '?number')
    return socket.bind(address or "localhost", port or 1999, M.newsession)
end

--- Creates a new client peer, which connects to a pre-existing server peer
-- at the address and port specified as parameters.
function M.newclient (address, port)
    checks('?string', '?number')
    local skt, err = socket.connect(address or "localhost", port or 1999)
    if not skt then return nil, err end
    return M.newsession(skt)
end

--- Spawns a new Lua VM with an RPC peer on it, returns the local RPC peer object.
function M.spawn()
    local  server, err = socket.bind("localhost", 0)
    if not server then return nil, err end
    local _, port = server :getsockname()
    local status = os.execute (M.SPAWN_CMD :gsub('%%port', tostring(port)))
    if status ~= 0 then
        local msg = "Cannot spawn a new Lua VM, status="..tostring(status)
        log('LUARPC', 'ERROR', "%s", msg)
        return nil, msg
    end
    local client, err = server :accept()
    if not client then return nil, err end
    server :close() -- close the server socket as we do need it anymore
    log('LUARPC', 'INFO', "Lua VM spawned and connected through RPC")
    return M.newsession(client)
end

return M