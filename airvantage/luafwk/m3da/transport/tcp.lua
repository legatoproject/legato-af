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

-- `M.new(url)` returns a transport instance, which honors the M3DA transport API:
--
-- * `:send(src)` sends data bytes, given as an LTN12 source, over the network;
--   returns true upon success, nil+errmsg upon failure
--
-- * Data bytes coming from the network are pushed into `self.sink`, which
--   must have been set externally (normally by an `m3da.session` instance).

local socket = require 'socket'
require 'socket.url'

local M  = { }
local MT = { __index=M, __type='m3da.transport' }

M.port        = false
M.servername  = false
M.socket      = false
M.sink        = false
M.defaultport = 44900

--- Create or recreate the TCP connection to the server.
function M :getsocket()
    checks('m3da.transport')
    if self.socket then return self.socket end
    local errmsg
    self.socket, errmsg = socket.connect(self.servername, self.port)
    if self.socket then sched.run(M.monitor, self) end
    log('M3DA-TRANSPORT', 'DEBUG', "Opening socket")
    return self.socket, errmsg
end

--- Reading loop: monitor an open TCP socket for incoming data, and cleans up
--  when it's shut down remotely.
-- Automatically attached to the socket, when it's created, for its lifetime.
function M :monitor()
    checks('m3da.transport')
    assert(self.sink, "missing session sink in transport layer")
    local src, snk = socket.source(self.socket), self.sink
    local data, err
    repeat
        data, err = self.socket :receive '*'
        if data then
            log('M3DA-TRANSPORT', 'DEBUG', "Received %d bytes from server", #data)
        end
        local status_snk, err_snk = self.sink(data, err)
        if not status_snk then
            err = tostring(err_snk)
            if err~='closed' then
                log('M3DA-TRANSPORT', 'ERROR', "Error when consuming incoming data: %s", err)
            end
            break
        end
    until not data
    if self.socket then
        log('M3DA-TRANSPORT', err=='closed' and 'DEBUG' or 'ERROR', "Closing socket: %s", err)
        self.socket :close()
        self.socket = false
    end
end

--- Sends the payload of an ltn12 source to the server.
function M :send(src)
    checks('m3da.transport', '!')
    local skt, errmsg = self :getsocket()
    if not skt then return nil, errmsg end
    return ltn12.pump.all(src, socket.sink(skt))
end

function M.new(url)
    checks('string')
    local self = { }
    local cfg = socket.url.parse(url)
    assert(cfg.scheme == 'tcp')
    if cfg.path or cfg.query then
        log('M3DA-TRANSPORT', 'ERROR', "TCP transport doesn't support paths or queries in URL")
        return nil, "invalid config"
    end
    self.servername, self.port = cfg.host, tonumber(cfg.port) or M.defaultport
    return setmetatable(self, MT)
end

return M