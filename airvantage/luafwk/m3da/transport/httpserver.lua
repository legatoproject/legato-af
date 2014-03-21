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

require 'web.server'
require 'socket.url'

local src2string = require 'utils.ltn12.source'.tostring

local M  = { }
local MT = { __index=M, __type='m3da.transport' }

-- Workaround: when the page is accessed and there's several transport instances, it needs to determine which
-- transport session must be triggered, as each has its own session and stack of responses to serve.
-- For now, only one session is accepted, and a reference to it is kept in `only_self`.
local only_self = false

-- Outside connection
function M.page(echo, env)
    local self = assert(only_self)
    local payload = env.body or ''
    log('M3DA-TRANSPORT', 'DETAIL',
        "HTTP request: $d bytes received, %d messages to send back",
        #payload,
        #self.responses)
    assert(self.sink, "Missing transport incoming sink")
    self.sink(assert(payload))
    for _, src in ipairs(self.responses) do
        local str = src2string(src)
        printf ('Serving %d bytes of response: %q', #str, str)
        if self.history then table.insert(self.history, str) end
        echo(str)
    end
    self.responses = { }
end

function M.new(url)
    checks('string')
    local self = {
        history   = false,
        sink      = false,
        responses = { }
    }
    setmetatable(self, MT)
    local cfg  = socket.url.parse(url)
    local path = cfg.path :gsub ('^/', '') or cfg.path
    if not web.site[path] then web.site[path] = M.page end
    assert(not only_self, "multiple M3DA  httpserver transport instances not supported")
    only_self=self
    log('M3DA-TRANSPORT', 'INFO', "Registering HTTP server on path %q", path)
    return self
end

-- Buffer data, it will be sent as a response to the next connection
function M :send(src)
    checks('m3da.transport', 'function')
    log('M3DA-TRANSPORT', 'DEBUG', "Buffering data, to be sent upon next connection")
    table.insert(self.responses, src)
    return 'ok'
end

return M