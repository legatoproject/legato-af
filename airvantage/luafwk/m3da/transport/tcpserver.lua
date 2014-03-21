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
-- * Data bytes coming from the network are pushed into `self.sink`, an LTN12
--   sink which must have been set externally (normally by an
--   `m3da.session` instance).

require 'socket.url'
local src2string = require 'utils.ltn12.source'.tostring

local M  = { }
local MT = { __index=M, __type='m3da.transport' }

-- Outside connection
function M.flush()
    log('M3DA-TRANSPORT', 'DETAIL', "TCP connection received; %d message sources to serve", #M.responses)
end

--- Reading loop: monitor an open TCP socket for incoming data, and cleans up
--  when it's shut down remotely.
-- Automatically attached to the socket, when it's created, for its lifetime.
function M :monitor()
    checks('m3da.transport')
    assert(self.sink, "missing session sink in transport layer")
    repeat
        local data, err = self.socket :receive '*'
        if data then self.sink(data) end
    until not data
    self.socket :close()
    self.socket = false
end

function M :onaccept(skt)
    checks('m3da.transport', '!')
    if self.socket then -- close previous connection
        log('M3DA-TRANSPORT', 'DEBUG', "Changing the active connected socket")
        self.socket :close()
    end
    self.socket = skt
    -- Monitor incoming msg and socket closing
    sched.run(M.monitor, self)
    -- Flush unsent messages
    log('M3DA-TRANSPORT', 'DEBUG', "%d buffered messages to flush", #self.responses)
    for _, src in ipairs(self.responses) do self :send (src) end
    self.responses = { }
end

function M.new(url)
    checks('string')
    local cfg = socket.url.parse(url)
    local port = tonumber(cfg.port) or M.defaultport
    local host = cfg.host or '*'
    local self = {
        socket       = false, -- connected socket
        sink         = false,
        responses    =  { } }
    setmetatable(self, MT)
    self.listensocket = socket.bind(host, port, function(skt) self:onaccept(skt) end)
    return self
end

-- Buffer data, it will be sent as a response to the next connection
function M :send (src)
    checks('m3da.transport', 'function')
    if self.socket then
        log('M3DA-TRANSPORT', 'DEBUG', "Send data immediately")
        return ltn12.pump.all(src, socket.sink(self.socket))
    else
        log('M3DA-TRANSPORT', 'DEBUG', "Buffering data, to be sent upon next connection")
        table.insert(self.responses, src)
    end
    return 'ok'
end

return M