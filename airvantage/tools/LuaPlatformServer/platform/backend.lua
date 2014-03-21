-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local M = { }

platform.backend = M

platform.history = { }

local m3da = require 'm3da.bysant'
local m3da_deserialize = m3da.deserializer()

local function m3da_serialize(x)
   local s, t = assert(m3da.serializer{ })
    assert(s(x))
    return table.concat(t)
end

function M.to_device(message)
    checks('table')
    local str = assert(m3da_serialize(message))
    local function src_factory()
        return ltn12.source.string(str)
    end
    M.session :send (src_factory)
    table.insert(platform.history, {os.date(), true, message})
end

function M.msghandler(serialized_message)
    local message, offset = nil, 1
    repeat
        message, offset = m3da_deserialize(serialized_message, offset
        )
        if message then
            log('PLATFORM', 'INFO', "Received a message from device: %s", sprint(message))
            M.from_device(message)
        end
    until not message
    M.session :send(ltn12.source.empty, {status=200})
    return 'ok'
end

function M.init(cfg, from_device)
    checks('table', 'function')
    M.from_device = from_device
    local transport_name = socket.url.parse(cfg.url).scheme
    local session_name = cfg.authentication and 'security' or 'default'
    local transport_mod = require ('m3da.transport.'..transport_name)
    local session_mod = require ('m3da.session.'..session_name)
    M.transport = assert(transport_mod.new(cfg.url))
    M.session = assert(session_mod.new{
        transport      = M.transport,
        msghandler     = M.msghandler,
        authentication = cfg.authentication,
        localid       = cfg.localid })
    log('PLATFORM', 'INFO', "Backend started with transport %q and session %q",
        transport_name, session_name)
    return 'ok'
end

return M