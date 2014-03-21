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

-------------------------------------------------------------------------------
-- Allows to redirect Lua logs to TCP sockets, by adding a function
-- log.settarget() to the log module.
-- See function comments below for details.
-------------------------------------------------------------------------------

require 'log'

-------------------------------------------------------------------------------
-- * `original_displayloger`: allows to revert to default behavior.
-- * `clients`: list of all currently connected TCP clients
-- * `server`: currently running TCP server, can be nil.
local original_displaylogger = log.displaylogger
local clients = { }
local server  = nil

-------------------------------------------------------------------------------
-- Closes all TCP sockets, clients and server.
local function kill_sockets()
    for s, _ in pairs (clients) do s :close(); clients[s] = nil end
    if server then server :close(); server = nil end
end

-------------------------------------------------------------------------------
-- Dispatches log messages to all TCP client sockets.
local function tcplogger(_, _, msg)
    for c, _ in pairs(clients) do
        local status = c :send(msg.."\r\n")
        if not c then c :close(); clients[c]=nil end
    end
end

-------------------------------------------------------------------------------
-- Redirects all messages on the Telnet shell.
local function shelllogger(_, _, msg)
    print (msg.."\r\n$ ")
end

-------------------------------------------------------------------------------
-- Allows to redirect display logs.
--  * `log.tcp "shell"` will send logs to the telnet shell;
--  * `log.tcp(n)`, with `n` a port number, starts a TCP server
--    socket on that port. Every client connecting to this port will
--    receive the logs; there can be several clients simultaneously.
--  * `log.tcp "<host>:<port>"` connects to the given IP, on the
--    given TCP port, and sends logs to the established connection.
--  * `log.tcp()` reverts to the default logging behavior.
function log.tcp(x, port)
    if type(x)=='string' and x :match "^([^:]+):(%d+)$" then
        x, port = x :match "^([^:]+):(%d+)$"; port = tonumber(port)
    end

    if x=='shell' then
        -- print on shell --
        kill_sockets()
        log.displaylogger = shelllogger

    elseif tonumber(x) then
        -- start log server socket --
        if server then server :close() end
        local function srv_hook (client) clients[client] = true end
        server = socket.bind('*', tonumber(x), srv_hook)
        log.displaylogger = tcplogger

    elseif type(x)=='string' and port then
        -- start log client socket --
        log.displaylogger = tcplogger
        local client, msg = socket.connect (x, port)
        if client then clients[client] = true else error (msg) end

    elseif not x then
        -- back to default --
        kill_sockets()
        log.displaylogger = original_displayloger

    else error "Invalid log.settarget parameter" end
end

return log.tcp
