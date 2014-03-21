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

local checks = require 'checks'
local l2b = require 'luatobin'
require 'print' -- TODO remove
require 'utils.table' -- pack/unpack
local M = { }

-- must not be valid luatobin prefixes
M.RECV_MARKER = '!'
M.SEND_MARKER = '?'

-- type -> method_name -> result_types
M.method_signatures = { }

-- path -> result_types
M.function_signatures = { }

-- register the signature of a local function/method's results
-- signature('snmp.open', '#snmp.session')
-- signature('#snmp.session', 'get', '#snmp.var')
function M.signature(what, a, b)
    if b then -- method signature
        checks('string', 'string', '?string|table|boolean')
        local class_name, method_name, results = what, a, b
        if type(results)~='table' then results = { results } end
        local methods = M.method_signatures[class_name]
        if methods then methods[method_name] = results
        else M.method_signatures[class_name] = { [method_name] = results } end
    else -- function signature
        checks('string', '?string|table|boolean')
        local func_name, results = what, a
        if type(results)~='table' then results = { results } end
        M.function_signatures[func_name] = results
    end
end

M.remoteref_mt = { __type='rpc.remoteref' }

-- session_id .. "/" .. ref_id -> ref_proxy
M.remoteref_store = setmetatable({ }, {__mode='v'})

function M.remoteref_mt :__tostring()
    return "<remote "..self.session.id..':'..self.n..">"
end

function M.remoteref_mt :__index(key)
    if type(key) ~= 'string' then return nil end
    return function(_, ...)
        return self.session :invoke (self, key, ...)
    end
end

function M.remoteref_serialize(self)
    log('LUARPC', 'DEBUG', "Serialize remote ref %i", self.n)
    return M.SEND_MARKER .. string.pack('>i', self.n)
end

function M.remoteref(session, n)
    checks('rpc.session', 'number')
    local key = session.id..'/'..n
    local self = M.remoteref_store[key]
    if self then return self else
        self = setmetatable({session=session, n=n}, M.remoteref_mt)
        M.remoteref_store[key] = self
        return self
    end
end

M.localref_mt = { __type='rpc.localref' }
M.localref_mt.__index = M.localref_mt

function M.localref_mt :__tostring()
    return "<local:"..self.n.." ("..self.type..")>"
end

function M.localref_mt :serialize()
    log('LUARPC', 'DEBUG', "Serialize local ref %i", self.n)
    return M.RECV_MARKER..string.pack('>i', self.n)
end

-- what -> { n=n, type=type }
-- References to be cleaned upon request by remote peer, or session end.
M.localref_byobj = { }
-- n -> what
M.localref_bynum = { }

local localref_index = 0

-- Retrieve or create a localref of appropriate type
function M.localref(what, stype)
    checks('?', '?string')

    -- Give up for immediate values
    local twhat = type(what)
    if twhat=='number' or twhat=='boolean' or twhat=='string' then return nil end

    -- Retrieve if already cached
    local ref = M.localref_byobj[what]
    if ref ~= nil then return ref end

    -- Give up for non-cached, non-typed values
    if not stype then return nil end

    -- Create, cache and return when a type is provided
    local n = localref_index+1; localref_index = n
    local ref = setmetatable({ n=n, type=stype }, M.localref_mt)
    M.localref_byobj[what] = ref
    M.localref_bynum[n] = what
    return ref
end

-- TODO: refuse to serialize references to the wrong session
function M.serialize (what, stype)
    checks('?', '?string')
    if getmetatable(what) == M.remoteref_mt then
        log('LUARPC', 'DEBUG', "Serialize remote ref")
        return M.remoteref_serialize(what)
    else
        log('LUARPC', 'DEBUG', "Serialize "..(stype or "structurally"))
    end
    local ref = M.localref(what, stype)
    if ref then return ref :serialize()
    else return l2b.serialize(what) end
end

function M.deserialize (session, bytes, n, offset)
    checks('rpc.session', 'string', '?number', '?number')
    offset = offset or 1
    if not n or n==0 then n=math.huge end
    local results = { }
    for i=1, n do
        if offset > #bytes then n=i-1; break end
        log('LUARPC', 'DEBUG', "Deserializing from offset %i / %i", offset, #bytes)
        local first_byte = bytes :sub (offset, offset)
        if first_byte == M.RECV_MARKER then
            local _, n = string.unpack(bytes, '>i', offset+1)
            log('LUARPC', 'DEBUG', "Deserialized remote ref %i", n)
            local obj = M.remoteref(session, n)
            offset = offset+5
            results[i] = obj
        elseif first_byte == M.SEND_MARKER then
            local _, n = string.unpack(bytes, '>i', offset+1)
            log('LUARPC', 'DEBUG', "Deserialized local ref %i", n)
            local obj = M.localref_bynum[n] or error ("Unknown local object "..n)
            offset = offset+5
            results[i] = obj
        else
            offset, results[i] = l2b.deserialize (bytes, 1, offset)
        end
    end
    --log('LUARPC', 'DEBUG', "End of deserialization, %i results", n)
    return offset, unpack(results, 1, n)
end

return M