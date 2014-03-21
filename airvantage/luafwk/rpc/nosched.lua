-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local log    = require 'log'
local socket = require 'socket'
local common = require 'rpc.common'
local proxy  = require 'rpc.proxy'

require 'pack' -- string.pack, string.unpack

local M = { _NAME = 'rpc.nosched' }

function common.session_mt :_waitcallresponse (seqnum)
    return nil, "call from rpc.nosched not implemented"
end

function M.loop(session)
    local skt = session.skt
    log("LUARPC", "DEBUG", "new connection: %s-%s", skt:getpeername())
    local header, err_msg, payload
    while true do
        header, err_msg = skt :receive(6)
        if not header then break end
        local _, t, seqnum, size = header :unpack(">bbI")
        payload, err_msg = skt :receive(size)
        if not payload then -- Invalid message
            break
        elseif t == 0 then -- RPC Call
            local index, func_name = proxy.deserialize(session, payload, 1)
            local params = {select(2, proxy.deserialize(session, payload, nil, index))}
            --log('LUARPC','WARNING',"call func_name=%q, params=%s", func_name, sprint(params))
            common.execute(skt, seqnum, func_name, params)
        else -- There shouldn't be any response, we never call anything.
            err_msg = "Unexpected response"
            break
        end
    end

    skt:close() -- make sure the oscket is closed locally so that a later send on that socket will fail!
    local clean_exit = err_msg == "closed"
    if err_msg then
        local level = clean_exit and "DEBUG" or "ERROR"
        log("LUARPC", level, "Lua RPC connection closed: %s", err_msg)
    end
    if clean_exit then return true else return nil, err_msg end
end


local session_index = 0

-- To be exported as rpc.newsession
function M.newsession(skt)
    session_index = session_index + 1
    local self = setmetatable ({seqnum=0, skt=skt, id=session_index}, common.session_mt)
    -- rpc.loop will be provided either by rpc.sched or rpc.nosched
    return rpc.loop(self)
end


function M.newserver(address, port)
    checks('?string', '?number')
    local  server, err = socket.bind(address or "localhost", port or 1999)
    if not server then return nil, err end
    local client, err = server :accept()
    if not client then return nil, err end
    server :close() -- close the server socket as we do need it anymore
    M.newsession(client)
end

function M.newclient(address, port)
    checks('?string', '?number')
    local  client, err = socket.connect(address or "localhost", port or 1999)
    if not client then return nil, err end
    print(client :getsockname())
    M.newsession(client)
 end


return M