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
--     David Francois     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require = require
local sched = require "sched"
local pack = require "pack"
local config = require "agent.config"
local log = require "log"
local timer = require "timer"
local socket = require "socket"

local string = string
local table = table
local tonumber = tonumber


module (...)

local status = 0

local skt
local pollingperiod
-- timers
local keepalive
local restart
local listener
local heartbeat

------------------------------------------------------------------
-- send an acknowledgment for the requestid given as parameter  --
------------------------------------------------------------------
local function sendack(reqid)
    if status > 2 and skt then
        local packet = string.pack(">bb", 0,reqid)
        local ret, err = skt:send(packet)
        if not ret then
            log("MEDIATION", "WARNING", "cannot send ack [%d]", reqid)
            return nil, err
        end
        log("MEDIATION", "INFO", "ack [%d] sent", reqid)
        return true
    end
    return nil, "cannot send ack"
end

------------------------------------------------------------------
-- send keep alive                                              --
------------------------------------------------------------------
local function sendkeepalive(currentid)
    local retries = tonumber(config.mediation.servers.retries) or 1
    local timeout = tonumber(config.mediation.timeout) or 1
    local packet = string.pack(">bbbA", 1, currentid, #config.agent.deviceId, config.agent.deviceId)
    while status > 1 and skt and retries > 0 do
        local sent, err = skt:send(packet)
        if not sent then
            log("MEDIATION", "WARNING", "cannot send keep alive, %s", err or "unknown")
            retries = retries - 1
        else
            log("MEDIATION", "DETAIL", "keep alive [%d] sent...", currentid)
            local ev = sched.wait("MEDIATION", "ACK"..currentid, timeout)
            if status > 1 and skt then
                if ev == "timeout" then
                    log("MEDIATION", "WARNING", "no ack [%d] received...", currentid)
                    retries = retries - 1
                else
                    log("MEDIATION", "DETAIL", "ack [%d] received", currentid)
                    if pollingperiod then
                        if keepalive then timer.cancel(keepalive) keepalive = nil end
                        keepalive = timer.new(pollingperiod, sendkeepalive, currentid % 255 + 1)
                    end
                    return true
                end
            end
        end
    end

    if status > 1 and skt then
        status = 1 skt:close()
    end
end

----------------------------------------------
-- mediation listener                       --
----------------------------------------------
local function mediationlistener(index)
    local s, err
    while status > 2 and skt do
        s, err = skt:receive()
        if not s then
            break
        else
            local offset, cmdid, reqid = string.unpack(s, ">bb")
            -- ack received
            if cmdid == 0 then
                sched.signal("MEDIATION", "ACK"..reqid)
                reqid = nil
            -- set period command received
            elseif cmdid == 2 then
                offset, pollingperiod = string.unpack(s, ">H", offset)
                log("MEDIATION", "INFO", "new polling period[%d]: %ds", reqid, pollingperiod)
            -- set address command received
            elseif cmdid == 3 then
                local ip1, ip2, ip3, ip4, serveraddr, serverport
                offset, ip1, ip2, ip3, ip4, serverport = string.unpack(s, ">bbbbH", offset)
                if ip1 and ip2 and ip3 and ip4 and serverport then
                    sendack(reqid)
                    reqid = nil
                    serveraddr = ip1 .. "." .. ip2 .. "." .. ip3 .. "." .. ip4
                    skt:setpeername('*')
                    skt:setpeername(serveraddr, serverport)
                    log("MEDIATION", "INFO", "switching to new mediation server on [%s:%d]", serveraddr, serverport)
                    local n = 1
                    while config.mediation.servers[n] do
                        n = n + 1
                    end
                    for i = n, 2, -1 do
                        config.mediation.servers[i] = config.mediation.servers[i-1]
                    end
                    config.mediation.servers[1] = {addr = serveraddr, port = serverport}
                end
            -- connect to platform command received
            elseif cmdid == 4 then
                log("MEDIATION", "INFO", "connection to server requested[%d]", reqid)
                local server = require "agent.srvcon"
                server.connect()
            else
                log("MEDIATION", "WARNING", "received unknown command[%d]: %d", reqid, cmdid)
            end
            if reqid then sendack(reqid) end
        end
    end
    if keepalive then timer.cancel(keepalive) keepalive = nil end
    sched.signal("MEDIATION", "STOPPED")
    log("MEDIATION", "WARNING", "connection lost on server(%d), '%s'", index, err or "unknown")
    if status > 0 and skt then
        status = 1 skt:close()
        sched.run(findserver, index)
    end
end

--------------------------------------------------------
-- get a running configuration or die trying          --
--------------------------------------------------------
function findserver(index)
    while status > 0 and config.mediation.servers[index + 1] do
        index = index + 1
        if skt then skt:close() skt = nil end
        --local serveraddr = socket.dns.toip(config.mediation.servers[index].addr)
        local serveraddr = config.mediation.servers[index].addr
        local serverport = config.mediation.servers[index].port
        local ok, err
        if status > 0 and serveraddr and serverport then
            skt, err = socket.udp()
            if skt then
                ok, err = skt:setpeername(serveraddr, serverport)
                if status > 0 and ok then
                    log("MEDIATION", "INFO", "listening on server(%d) [%s:%d]", index, serveraddr, serverport)
                    status = 3
                    if keepalive then timer.cancel(keepalive) keepalive = nil end
                    if heartbeat then sched.kill(heartbeat) heartbeat = nil end
                    if listener then sched.kill(listener) listener = nil end
                    listener = sched.run(mediationlistener, index)
                    heartbeat = sched.run(sendkeepalive, 1)
                    return
                end
            end
        end
        log("MEDIATION", "ERROR", "cannot listen on server(%d), %s", index, err or "wrong parameters")
    end
    if status > 0 and not restart then
        local delay = tonumber(config.mediation.retrydelay) or 1800
        log("MEDIATION", "INFO", "restarting in %ds", delay)
        restart = timer.new(delay, function() stop() start(pollingperiod) end)
    end
end

-------------------------
-- public start method --
-------------------------
function start(period, index)
    if status > 0 then
        log("MEDIATION", "WARNING", "already started (polling='%d')", pollingperiod or 0)
        return nil, "mediation is running"
    end
    status = 1
    period = tonumber(period) or 0
    if period > 0 then
        pollingperiod = period
    else
        pollingperiod = nil
    end
    log("MEDIATION", "INFO", "started (polling='%d')", pollingperiod or 0)
    findserver(index or 0)
end

-------------------------
-- public stop method --
-------------------------
function stop()
    if skt then skt:close() skt = nil end
    if keepalive then timer.cancel(keepalive) keepalive = nil end
    if restart then timer.cancel(restart) restart = nil end
    if status > 2 then sched.wait("MEDIATION", "STOPPED", 30) end
    if listener then sched.kill(listener) listener = nil end
    if heartbeat then sched.kill(heartbeat) heartbeat = nil end
    status = 0
    log("MEDIATION", "INFO", "stopped")
end

--------------------------------------------------
-- listen to netman state to starts and stops   --
--------------------------------------------------
local function netmanhook(event, bearername)
    if not config.mediation.pollingperiod[bearername] then
        log("MEDIATION", "ERROR", "cannot start, no polling period for bearer "..bearername)
    else
        stop() start(config.mediation.pollingperiod[bearername])
    end
end

---------------------------
-- initiliaze the modude --
---------------------------
function init()
    sched.sigrun("NETMAN", "CONNECTED", netmanhook)
    return "ok"
end
