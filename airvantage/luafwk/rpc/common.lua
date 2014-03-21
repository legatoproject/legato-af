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

local require = require

require 'utils.table' -- needed for table.pack. To be removed when we switch to Lua5.2

require 'pack'
require "coxpcall"
local checks = require 'checks'
local l2b = require 'luatobin'
local log = require 'log'
local upath = require 'utils.path'
local socket = require 'socket'
local loader = require 'utils.loader'
local proxy = require 'rpc.proxy'
require 'print'

local M = { }

-- This table associate remote function names to the matching functions.
-- When a function isn't found in this table, it is looked for as
-- `upath.get(_G, path)`, i.e. in the globals.
M.remotefunctions = { }

-- This table associate remote function names a function which returns the
-- types of proxy objects returned.
M.resulttypes = { }


function M.remotefunctions.__rpceval(skt, v)
    return upath.get(_G, v)
end

-- This table holds function pushed by peers for later invocation. They're
-- indexed by number, and the peer will call them by number.
-- TODO: Keys are numbers, what's the point of __mode='k'???
M.execstore = setmetatable({ }, { __mode="k" })

--- Receives a function, sent by the peer, as a script or upvalue-free function.
-- This function is registered with a numeric identifier `execid` locally, and
-- this id is sent back to the peer. The peer will then be able to call the
-- registered function through `__rpcrunexec()`.
-- Those functions are not intended to be called directly, the official API is
-- through the `:newexec()` method.
function M.remotefunctions.__rpcbuildexec(skt, script)
    if not M.execstore[skt] then M.execstore[skt] = {execid=1} end
    local store = M.execstore[skt]

    local f, err = script
    if     type(script) == "string" then f, err = loadstring(script, "@rpc")
    elseif type(script) == "table"  then f, err = loader.loadbuffer(script, "@rpc", true)
    end
    assert(type(f) == "function", err)
    local execid = store.execid
    store.execid = execid+1
    store[execid] = f
    return execid
end

--- Executes a a function previously uploaded by a peer, upon requests by
-- that peer.
function M.remotefunctions.__rpcrunexec(skt, execid, ...)
    local store = M.execstore[skt]
    local exec =  store and store[execid]
    if not exec then error("Undefined exec call") end
    return exec(...)
end

--- Executes a a function without returning its result through the RPC.
-- This is useful for functions which won't
function M.remotefunctions.__call0(skt, func_name, ...)
    error "Not implemented"
end

--- Executes an object invocation.
function M.remotefunctions.__rpcinvoke(skt, obj, method_name, ...)
    return obj[method_name](obj, ...)
end

function M.resulttypes.__rpcinvoke(obj, method_name, ...)
    local obj_ref = proxy.localref_byobj[obj]
    if obj_ref == nil then return { } end
    local obj_sig = proxy.method_signatures[obj_ref.type]
    if obj_sig == nil then return { } end
    local method_sig = obj_sig[method_name]
    if method_sig == nil then return { } end
    return method_sig
end

function M.resulttypes.__rpceval(obj)
        local obj_ref = proxy.localref_byobj[obj]
    local obj_type = obj_ref and obj_ref.type
    return obj_type and proxy.method_signatures[obj_type] or { }

end

--- Execute the function denoted by `func_name` and sends the response package
-- on the socket.
-- @param skt the TCP socket
-- @param seqnum sequence number of the response packet to send
-- @param func_name either:
--  * the name under which a function is registered in `M.remotefunctions`,
--  * or string `":"` to denote a method invocation,
--  * or the dot-separated path of a global variable holding a function.
-- @param params list of already deserialized function parameters
-- @return nothing, the success/failure notification is sent to the peer
--   through `skt`.
function M.execute(skt, seqnum, func_name, params)

    -- Runs `func_name` with its parameters, and never causes an error thanks to
    -- socket.protect.
    local status, r, _ = 0 , { n=0 }

    log('LUARPC', 'DEBUG', "Executing function '%s'", func_name)

    -- Retrieve the function; add a `skt` 1st param to it.
    local func = M.remotefunctions[func_name]
    if func then
        local raw_func = func
        func = function(...) return raw_func(skt, ...) end
    else _, func = pcall (upath.get, _G, func_name) end -- not in remote funcs, look for a global

    if func == nil then
        log('LUARPC', 'ERROR', "Function %q not found", func_name)
        status = 2
    elseif type(func)~="function" then
        log('LUARPC', 'ERROR', "%q is a %s, not a function", tostring(func), type(func))
        status = 3
    else
        r = table.pack(copcall(func, unpack(params)))
        if not r[1] then
            log('LUARPC', 'ERROR', "error while executing %s(%s): %q",
                func_name,
                sprint(params) :sub (2, -2),
                tostring(r[2]))
            status = 1
        end -- exception while executing
    end

    local payload = { }
    local res_types

    if status == 0 then
        -- Attempt to retrieve signature
        local resulttypes_f =  M.resulttypes[func_name]
        res_types = resulttypes_f and resulttypes_f(unpack(params)) or
            proxy.function_signatures[func_name] or { }
    else res_types = { } end

    -- Serialize results or copcall error
    for i = 1, r.n - 1 do
        local success, ser = pcall(proxy.serialize, r[i+1], res_types[i])
        if success then payload[i] = ser else
            log('LUARPC', 'ERROR', "Cannot serialize result #%i of %q",
                i, func_name)
            status, payload = 4, { proxy.serialize("Cannot serialize result") }
            break
        end
    end

    payload = table.concat(payload)
    skt :send (string.pack(">bbIb", 1, seqnum, #payload+1, status))
    skt :send (payload)
end

-- Metatable for RPC objects methods
M.session_mt = { __type='rpc.session' }
M.session_mt.__index = M.session_mt

--- Requests and returns the value of a global variable, specified as a
-- dot-separated path, from the peer.
function M.session_mt :eval (varname)
    return self :call ("__rpceval", varname)
end

--- Requests the creation and registration of a function on the peer, and builds
-- a local function which remotely calls the peer function. For instance,
-- `self:newexec[[print "hello"]]` returns a function which prints `"hello"` on
-- the peer's stdout everytime it's called.
function M.session_mt :newexec (script)
    if type(script) == "function" and debug.getinfo(script, "u").nups ~= 0 then
        error("RPC function exec must not have up values")
    end
    local k = self :call ("__rpcbuildexec", script)
    return function(...) return self:call("__rpcrunexec", k, ...) end
end

--- Sends a call request to the peer, waits for response, return it.
-- Response handling is scheduling-dependent, and is provided by either
-- rpc.sched or rpc.nosched.
function M.session_mt :call (func_name, ...)
    checks('rpc.session', 'string')
    local payload = { proxy.serialize(func_name) }
    local params = table.pack(...)
    local types = proxy.function_signatures[func_name] or { }
    for i = 1, params.n do
        --log('LUARPC', 'WARNING', "Adding parameter %i/%i %s to RPC call %q", i, params.n, sprint(params[i]), func_name)
        table.insert(payload, proxy.serialize(params[i]))--, types[i]))
    end

    local seqnum, err = self :_sendpayload(payload)

    if not seqnum then return nil, err
    else return self :_waitcallresponse (seqnum) end
    --[[else
        local r = { self :_waitcallresponse (seqnum) }
        log('LUARPC', 'DEBUG', "Got response with %i results", #r)
        return unpack(r)
    end--]]
end


--- Sends a payload to the peer; returns the sequence number upon success,
--  nil+error msg otherwise.
function M.session_mt :_sendpayload (payload)
    checks('?', 'string|table')
    if type(payload)=='table' then payload=table.concat(payload) end
    local seqnum, skt = self.seqnum, self.skt
    self.seqnum = (seqnum+1) % 256
    local i, err
    i, err = skt :send(string.pack(">bbI", 0, seqnum, #payload))
    if not i then return nil, err end
    i, err = skt :send(payload)
    if not i then return nil, err end
    return seqnum
end

function M.session_mt :close()
    self.skt :close()
end

function M.session_mt :invoke(obj, method_name, ...)
    return self :call ('__rpcinvoke', obj, method_name, ...)
end

function M.session_mt :call0(func_name, ...)
    return self :call ('__call0', func_name, ...)
end

function M.session_mt :remsignature(...)
    return self :call('rpc.signature', ...)
end

return M