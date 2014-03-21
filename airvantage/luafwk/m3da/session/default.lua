-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- `M.new(cfg)` returns a session instance, which honors the M3DA session API:
--
-- * sends serialized M3DA messages from `:send(src_factory) to the transport
--   instance, wrapping them into M3DA envelopes as needed';
-- * sets a sink in the transport instance, to receive incoming data;
--
--   pushes completed, serialized M3MDA messages to the `msghandler()` passed
--   at init time.

local log      = require "log"
local ltn12    = require "ltn12"
local m3da     = require 'm3da.bysant'
local niltoken = require 'niltoken'
local m3da_deserialize = m3da.deserializer()

local M  = { }
local MT = { __index=M, __type='m3da.session' }

-- sessions are numbered, this counter keeps track of attributed numbers.
M.last_session_id = 0

-------------------------------------------------------------------------------
-- Creates an ltn12 sink to receive transport data as a bytes, and turn them
-- into complete M3DA envelopes pushed in the `self.incoming` pipe.
function M :newsink()
    local pending_data = ''
    local partial = nil
    return function (data, err)
        if not data then
            sched.signal(self, 'connection_closed')
            return nil, err
        end
        pending_data = pending_data .. data
        local envelope, offset
        envelope, offset, partial = m3da_deserialize (pending_data, partial)
        if offset=='partial' then return 'ok'
        elseif envelope then
            sched.signal(self, 'status', envelope.header.status)
            local payload = envelope.payload
            if payload==nil then payload=niltoken end
            self.incoming :send (payload)
            pending_data = pending_data :sub (offset, -1)
            return 'ok'
        else
            m3da_deserialize :reset()
            return nil, offset -- actually an error msg
        end
    end
end

-------------------------------------------------------------------------------
-- Wraps a source into an M3DA envelope and sends it through the session's
-- transport layer
-- @param src_factory a source factory, i.e. a function returning an ltn12 source
--   function. With the default session, the factory will only be called once,
--   but some more complex session managers might call it more than once,
--   e.g. because a message must be reemitted for security reasons.
-- @return `"ok"` or `nil` + error message
function M :send(src_factory, headers)
    checks('m3da.session', 'function', '?table')
    M.last_session_id = M.last_session_id + 1
    log("M3DA-SESSION", "INFO", "Opening default session #%d", M.last_session_id)
    local r, env_wrapper, errmsg
    headers = headers or { }
    headers.id=self.localid
    env_wrapper, errmsg = assert(m3da.envelope(headers))
    local source = ltn12.source.chain(src_factory(), env_wrapper)
    r, errmsg = self.transport :send (source)
    if not r then return nil, errmsg end
    log("M3DA-SESSION", "DEBUG", "Waiting for server response...")
    local ev, r = sched.wait(self, { 'status', 'connection_closed', 60 })
    if ev~='status' then
        log('M3DA-SESSION', 'ERROR', "Closed session #%d with error %s", M.last_session_id, ev)
        return nil, ev
    else
        log("M3DA-SESSION", "INFO", "Closing default session #%d with status %s.", M.last_session_id, tostring(r))
        return r
    end
end

-------------------------------------------------------------------------------
-- Report server messages to the handler provided by srvcon at module init time.
function M :monitor (handler)
    checks('m3da.session', 'function')
    while true do
        local msg = niltoken(self.incoming :receive()) or ''-- TODO: handle timeouts ?
        log('M3DA-SESSION', 'DEBUG', "Received a %d bytes message", #msg)
        handler(msg)
    end
end

M.mandatory_keys = { transport=1, msghandler=1, localid=1 }

function M.new(cfg)
    checks('table')
    local self = { }
    self.incoming = require 'sched.pipe' () -- queue where completed envelopes are pushed
    for key, _ in pairs(M.mandatory_keys) do
        local val = cfg[key]
        if not val then return nil, 'missing config key '..key end
        self[key]=val
    end
    setmetatable(self, MT)
    sched.run(M.monitor, self, cfg.msghandler)
    local sink, err_msg = self :newsink()
    if not sink then return nil, err_msg end
    self.transport.sink = sink
    return self
end

return M