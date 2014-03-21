-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local require = require
local sched = require 'sched'
local base = _G
require 'coxpcall'
local copcall = copcall
local string = require("string")
local math = require("math")
local socket = require("socket.core")
local tableutils = require 'utils.table'

-----------------------------------------------------------------------------
-- Addon in order to support multithread socket (non blocking the VM
-- This is a copas equivalent
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Redefines LuaSocket try/protect mechanism in a coroutine-safe way.
-----------------------------------------------------------------------------

local function statusHandler(status, ...)
    if status then return ... end
    return nil, ...
end

function socket.protect(func)
    return function (...)
        return statusHandler(copcall(func, ...))
    end
end

function socket.newtry(finalizer)
    local select =  base.select
    local error = base.error
    return function (...)
        local status = (...) or false
        if (status==false) then
            if finalizer then copcall(finalizer, select(2, ...) ) end
            error((select(2, ...)), 0)
        end
        return ...
    end
end

socket.try = base.assert

local monotonic_time = require 'sched.timer.core'.time
local table = require 'table'
local tonumber = base.tonumber
local debug = require 'debug'
local reg = debug.getregistry()

-- weak table that holds timeout for a socket. This allows not to use a table wrapper for each single socket
local blocktimeout = base.setmetatable({}, {__mode="kv"})
local totaltimeout = base.setmetatable({}, {__mode="kv"})

---------------------------------------------------------------------------
-- TCP blocking methods redifinitions
---------------------------------------------------------------------------

local tcpmeth = reg["tcp{master}"].__index

local oldtcp = socket.tcp
local oldtcpsend = tcpmeth.send
local oldtcpreceive = tcpmeth.receive
local oldtcpconnect = tcpmeth.connect
local oldtcpclose = tcpmeth.close
local oldtcpsettimeout = tcpmeth.settimeout
local oldtcpaccept = tcpmeth.accept
local oldtcplisten = tcpmeth.listen

function socket.tcp()
    local skt = base.assert(oldtcp())
    oldtcpsettimeout(skt, 0)
    return skt
end

local newtcp = {}

function newtcp.settimeout(self, timeout, mode)
    timeout = tonumber(timeout)
    timeout = timeout and (timeout>=0 or nil) and timeout -- negative timeout is equivalent to no timeout
    if not mode or mode:find("b") then blocktimeout[self] = timeout end
    if mode and mode:find("t") then totaltimeout[self] = timeout end
    return 1
end

function newtcp.send(self, data, i, j)
    local s, err, sent
    i = i or 1
    local lastIndex = i - 1
    --local tmark = os_time() -- time elapsed debug
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]

    repeat
        s, err, lastIndex = oldtcpsend(self, data, lastIndex + 1, j)
        if s or err ~= "timeout" then
            --return s, err, lastIndex, os_time()-tmark -- time elapsed debug
            return s, err, lastIndex
        end

        s, err = sched.fd.wait_writable(self, timelimit, timedelay)
        --if not s then return nil, err, lastIndex, os_time()-tmark end -- time elapsed debug
        if not s then return nil, err, lastIndex end
    until false
end

function newtcp.receive(self, pattern, prefix)
    local s, err, part
    local buffer = {prefix}
    local some_bytes
    if pattern == "*" then
        pattern = "*a"
        some_bytes = true
    end
    --local tmark = os_time() -- time elapsed debug
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]

    repeat
        s, err, part = oldtcpreceive(self, pattern)
        local v = s or part
        if v and #v>0 then table.insert(buffer, v) end

        if s then
            --return table.concat(buffer), nil, nil, os_time()-tmark -- time elapsed debug
            return table.concat(buffer)
        elseif err ~= "timeout" then
            --return nil, err, table.concat(buffer), os_time()-tmark -- time elapsed debug
            part = #buffer>0 and table.concat(buffer) or nil -- make sure not to return part if not partial data is available
            return nil, err, part
        elseif some_bytes and part and #part > 0 then
            --return table.concat(buffer), nil, nil, os_time()-tmark -- time elapsed debug
            return table.concat(buffer)
        end

        if tonumber(pattern) and part and #part > 0 then
            pattern = pattern - #part
        end

        s, err = sched.fd.wait_readable(self, timelimit, timedelay)
        --if not r then return nil, err, table.concat(buffer), os_time()-tmark  end -- time elapsed debug

        if not s then
            part = #buffer>0 and table.concat(buffer) or nil -- make sure not to return part if not partial data is available
            return nil, err, part
        end
    until false
end

function newtcp.connect(self, address, port)
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]
    local r, err = oldtcpconnect(self, address, port)
    if r or err ~= "timeout" then return r, err end
    r, err =  sched.fd.wait_writable(self, timelimit, timedelay)
    if not r then return nil, err end
    return oldtcpconnect(self, address, port)
end


local function accept(self)
    local r, err = oldtcpaccept(self)
    if r then oldtcpsettimeout(r, 0) end
    return r, err
end
function newtcp.accept(self)
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]
    oldtcpsettimeout(self, 0.1)
    local r, err = accept(self)
    if r or err ~= "timeout" then return r, err end
    r, err =  sched.fd.wait_readable(self, timelimit, timedelay)
    if not r then return nil, err end
    return accept(self)
end

-- Redefine socket:listen() so to add an additional optional parameter 'hook'
-- when defined hook must be a function that will be called when a new client socket
-- connects to this server socket.
-- backlog parameter is also optional
function newtcp.listen(self, backlog, hook)
    if base.type(backlog)=='function' then hook, backlog = backlog, nil end
    local r, err = oldtcplisten(self, backlog)
    if r and hook then
        local function spawnclient(server)
            local client = server:accept()
            if client then sched.run(hook, client, server) end
            return "again"
        end
        sched.fd.when_readable(self, spawnclient)
    end

    return r, err
end

function newtcp.close(self)
    sched.fd.close(self)
    return oldtcpclose(self)
end

tableutils.copy(newtcp, reg["tcp{master}"].__index, true)
tableutils.copy(newtcp, reg["tcp{client}"].__index, true)
tableutils.copy(newtcp, reg["tcp{server}"].__index, true)
reg["tcp{master}"].__gc = newtcp.close
reg["tcp{client}"].__gc = newtcp.close
reg["tcp{server}"].__gc = newtcp.close


---------------------------------------------------------------------------
-- UDP blocking methods redifinitions
---------------------------------------------------------------------------
local udpmeth = reg["udp{connected}"].__index
local newudp = {}
local oldudp = socket.udp
local oldudpclose = udpmeth.close
local oldudpsettimeout = udpmeth.settimeout
local oldudpreceive = udpmeth.receive
local oldudpreceivefrom = udpmeth.receivefrom

function socket.udp()
    local skt = base.assert(oldudp())
    oldudpsettimeout(skt, 0)
    return skt
end

function newudp.close(self)
    sched.fd.close(self)
    return oldudpclose(self)
end

newudp.settimeout = newtcp.settimeout

function newudp.receive(self, size)
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]

    local r, err = oldudpreceive(self, size)
    if r or err ~= "timeout" then return r, err end
    r, err =  sched.fd.wait_readable(self, timelimit, timedelay)
    if not r then return nil, err end
    return oldudpreceive(self, size)
end

function newudp.receivefrom(self, size)
    local timelimit = totaltimeout[self] and monotonic_time() + totaltimeout[self]
    local timedelay = blocktimeout[self]

    local r, ip, port, err = oldudpreceivefrom(self, size)
    if r or ip ~= "timeout" then return r, ip, port end
    r, err =  sched.fd.wait_readable(self, timelimit, timedelay)
    if not r then return nil, err end
    return oldudpreceivefrom(self, size)
end


tableutils.copy(newudp, reg["udp{connected}"].__index, true)
tableutils.copy(newudp, reg["udp{unconnected}"].__index, true)
reg["udp{connected}"].__gc = newudp.close
reg["udp{unconnected}"].__gc = newudp.close
