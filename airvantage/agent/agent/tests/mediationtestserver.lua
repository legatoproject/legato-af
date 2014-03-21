-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require 'sched'
local socket = require 'socket'
local config = require 'agent.config'
local log = require 'log'

require 'pack'
local string = string
local table = table
local tostring = tostring
local setmetatable = setmetatable
local type = type

module(...)

--modules definitions
local mediationserver, mediationtask, cmdackid

--cmd api
local cmdapi = {}

function cmdapi:setperiod(period)
    cmdackid = (cmdackid + 1) % 256
    local s, err = mediationserver:sendto(string.pack(">bbH", 2, cmdackid, period), self.host, self.port)
    if not s then return nil, err or "unknown" end
    return "ok"
end

function cmdapi:setaddress(hostname, port)
    cmdackid = (cmdackid + 1) % 256
    local ip, stats = socket.dns.toip(hostname)
    if not ip then return ip, stats end
    local ip1, ip2, ip3, ip4 = ip:match("(%d*)%.(%d*)%.(%d*)%.(%d*)")
    local s, err = mediationserver:sendto(string.pack(">bbbbbbH", 3, cmdackid, ip1, ip2, ip3, ip4, port), self.host, self.port)
    if not s then return nil, err or "unknown" end
    return "ok"
end

function cmdapi:connect()
    cmdackid = (cmdackid + 1) % 256
    local s, err = mediationserver:sendto(string.pack(">bb", 4, cmdackid), self.host, self.port)
    if not s then return nil, err or "unknown" end
    return "ok"
end

function cmdapi:custom(cmdid, payload)
    cmdackid = (cmdackid + 1) % 256
    local s, err = mediationserver:sendto(string.pack(">bb", cmdid, cmdackid)..payload, self.host, self.port)
    if not s then return nil, err or "unknown" end
    return "ok"
end

--server main loop
local function mediationloop(status)
    local s, err, host, port
    log("MEDSRV", "INFO", "server started")
    if status then
        status.countack = 0
        status.countkeepalive = 0
    end
    while mediationserver do
        s, host, port = mediationserver:receivefrom()
        if not s or not host or not port then
            break
        end
        clients = type(clients) == "table" and clients or {}
        if not clients[tostring(host)..tostring(port)] then
            clients[tostring(host)..":"..tostring(port)] = setmetatable({host=host, port=port}, {__index=cmdapi})
            table.insert(clients, clients[tostring(host)..":"..tostring(port)])
        end
        local offset, cmdid, ackid, deviceIdlength, deviceId = string.unpack(s, ">bbbA")
        log("MEDSRV", "INFO", "received [%d:%d] from [%s:%d]", cmdid, ackid, host, port)
        if cmdid == 1 then
            if status then status.countkeepalive = status.countkeepalive + 1 end
            s, err = mediationserver:sendto(string.pack(">bb", 0, ackid), host, port)
            if not s then break end
            if status then status.countack = status.countack + 1 end
        end
    end
    log("MEDSRV", "INFO", "server down")
end

--start, stop
function start(serveraddr, serverport, status)
    stop()
    config.mediation.servers = {{addr = serveraddr or "localhost", port = serverport or 8888}}
    mediationserver = socket.udp()
    mediationserver:settimeout(120)
    mediationserver:setsockname(serveraddr or "localhost", serverport or 8888)
    mediationtask = sched.run(mediationloop, status)
    return mediationserver, mediationtask
end

function stop()
    if mediationserver then
        mediationserver:close()
        mediationserver = nil
    end
    if mediationtask then
        sched.wait(5)
        sched.kill(mediationtask)
        mediationtask = nil
    end
    cmdackid = 0
    clients = nil
end
