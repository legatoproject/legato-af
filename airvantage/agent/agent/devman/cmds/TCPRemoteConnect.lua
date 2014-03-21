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

local sched  = require 'sched'
local socket = require 'socket'
local lock   = require 'sched.lock'

require 'pack'
local string = string
local log = log
local pairs = pairs
require 'coxpcall'
local copcall = copcall

--On device
local tunnel = {
    lock = nil,
    server_socket = nil,
    service_socket = {}}

local function service_close_connection(client_id, send_close)
    if tunnel.service_socket[client_id] then
        tunnel.service_socket[client_id]:close()
        tunnel.service_socket[client_id] = nil
    end

    if send_close and tunnel.server_socket then
        if tunnel.lock then tunnel.lock:acquire() end
        tunnel.server_socket:send(string.pack(">Hbb", 0 ,2 ,client_id))
        if tunnel.lock then tunnel.lock:release() end
    end
end

local function disconnect()
    if tunnel.lock then
        tunnel.lock:destroy()
        tunnel.lock = nil
    end
    if tunnel.server_socket then
        tunnel.server_socket:close()
        tunnel.server_socket = nil
    end
    for client_id in pairs(tunnel.service_socket) do
        service_close_connection(client_id, false)
    end
    log ('TCPTUNNEL', 'WARNING', "disconnected")
end

local function service_reader(client_id)
    log ('TCPTUNNEL', 'INFO', "starting service reader clientId=%d", client_id)
    local i, j, data, size, err,_
    while tunnel.service_socket[client_id] and not err do
        -- read service
        data, err = tunnel.service_socket[client_id]:receive('*')
        if not data or not tunnel.server_socket then
            break
        end
        size = string.len(data)
        --log ('TCPTUNNEL', 'DEBUG', "service->clientId=%d size=%d", client_id, size)
        sched.wait()
        -- send to device
        if tunnel.lock then tunnel.lock:acquire() end
        if not tunnel.server_socket then
            break
        end
        i = 1
        j = ((i + 65534) < size) and (i + 65534) or size
        while j >= i and not err do
            --log ('TCPTUNNEL', 'DEBUG', "->clientId=%d size=%d (%d)", client_id, j-i+1, j)
            _, err = tunnel.server_socket:send(string.pack(">Hbb", j-i+1, 3, client_id))
            if not err then
                _, err = tunnel.server_socket:send(string.sub(data, i, j))
                i = j + 1
                j = ((i + 65534) < size) and (i + 65534) or size
            end
        end
        if tunnel.lock then tunnel.lock:release() end
    end
    service_close_connection(client_id, true)
    log ('TCPTUNNEL', 'WARNING', "closing service reader clientId=%d", client_id)
end

local function server_reader()
    log ('TCPTUNNEL', 'INFO', "started")
    local data, size, cmd_id, client_id, status, err, laddr, lport
    while tunnel.server_socket do
        -- read from device
        data, err = tunnel.server_socket:receive(4)
        if not data then break end
        status, size, cmd_id, client_id = string.unpack(data, ">Hbb")
        assert(cmd_id and client_id)
        -- process command and send to client (if cmd=3)
        if cmd_id == 3 and size > 0 and tunnel.server_socket then --DATA
            data, err = tunnel.server_socket:receive(size)
            if data and tunnel.service_socket[client_id] then
                status, err = tunnel.service_socket[client_id]:send(data)
                if err then
                    service_close_connection(client_id, true)
                end
            else
                service_close_connection(client_id, true)
            end
        elseif cmd_id == 2 then --CLOSE
            service_close_connection(client_id, false)
        elseif cmd_id == 1 and size > 0 and tunnel.server_socket then --CONNECT
            service_close_connection(client_id, false)
            data, err = tunnel.server_socket:receive(size)
            if data then
                status, laddr, lport = string.unpack(data, ">PH")
                if laddr and lport then
                    status, err = copcall(socket.connect, laddr, lport)
                    if status then
                        tunnel.service_socket[client_id] = err
                        sched.run(service_reader, client_id)
                    else
                        service_close_connection(client_id, true)
                    end
                end
            end
        else
            log ('TCPTUNNEL', 'WARNING', "bad command[cmdId=%d size=%d] ", cmd_id, size)
        end
    end
    disconnect()
end

local function TCPRemoteConnect()
    local config = require 'agent.config'
    assert(config.device.tcprconnect and config.device.tcprconnect.addr and config.device.tcprconnect.port, "failed to load default parameters for server connection")
    if tunnel.server_socket then
        disconnect()
    end
    -- connect to server
    local status, err, _
    status, err = copcall(socket.connect, config.device.tcprconnect.addr, config.device.tcprconnect.port)
    assert(status and err, "failed to create socket on server")
    -- send connect commmand
    tunnel.server_socket = err
    _, err = tunnel.server_socket:send(string.pack(">Hbb", 0, 4, 0))
    if err then tunnel.server_socket:close() tunnel.server_socket = nil end
    assert(err == nil, "failed to send 'connect' to server")
    -- activate server listenning loop
    tunnel.lock = lock.new()
    sched.run(server_reader)
    return "ok"
end

return TCPRemoteConnect