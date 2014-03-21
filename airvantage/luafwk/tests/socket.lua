-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Diego Nehab - Original tests for LuaSocket 2.0.2
--     Cuero Bugot for Sierra Wireless - Porting to unittest test harness
-------------------------------------------------------------------------------

local u = require 'unittest'
local socket = require 'socket'

local t=u.newtestsuite("socket")
local enableDebugTests

local control, data, err
local host = "localhost"
local port = "8383"

local function fail(...)
    local s = string.format(...)
    u.fail(s)
end

local assert = u.assert



local origtcpsend
local origtcpreceive
function t:setup()
    control, err = socket.connect(host, port)
    if not control then u.abort("Failed to setup TCP test server connection: ["..err.."]\r\nVerify that you launched the socket test server: \"lua luafwk/luasocket/sockettestsvr.lua\"") end
    control:setoption("tcp-nodelay", true)

    -- instrument the send/receive tcp client methods,
    -- in order to add time elapsed param (needed for the timeout tests)
    local mt=getmetatable(control).__index
    origtcpsend=mt.send
    origtcpreceive=mt.receive
    function mt.send(...)
        local t = os.time()
        local a, b, c = origtcpsend(...)
        return a, b, c, os.time()-t
    end
    function mt.receive(...)
        local t = os.time()
        local a, b, c = origtcpreceive(...)
        return a, b, c, os.time()-t
    end
end

function t:teardown()
    -- Restore original send/receive
    if control then
        local mt=getmetatable(control).__index
        mt.send=origtcpsend
        mt.receive=origtcpreceive
    end
end


local function test_methods(sock, methods)
    for _, v in pairs(methods) do
        u.assert_function(sock[v], 'function "'..v..'" missing')
    end
end

function t:test_tcp_methods()
    test_methods(socket.tcp(), {
    "accept",
    "bind",
    "close",
    "connect",
    "dirty",
    "getfd",
    "getpeername",
    "getsockname",
    "getstats",
    "setstats",
    "listen",
    "receive",
    "send",
    "setfd",
    "setoption",
    "setpeername",
    "setsockname",
    "settimeout",
    "shutdown",
    })
end

function t:test_udp_methods()
    test_methods(socket.udp(), {
    "close",
    "getpeername",
    "dirty",
    "getfd",
    "getpeername",
    "getsockname",
    "receive",
    "receivefrom",
    "send",
    "sendto",
    "setfd",
    "setoption",
    "setpeername",
    "setsockname",
    "settimeout"
    })
end



function remote(...)
    local s = string.format(...)
    s = string.gsub(s, "\n", ";")
    s = string.gsub(s, "%s+", " ")
    s = string.gsub(s, "^%s*", "")
    control:send(s .. "\n")
    control:receive()
end
local function reconnect()
    if data then data:close() end
    remote [[
        if data then data:close() data = nil end
        data = server:accept()
        data:setoption("tcp-nodelay", true)
    ]]
    data = assert(socket.connect(host, port))
    data:setoption("tcp-nodelay", true)
end

function t:test_readafterclose()
    local back, partial
    local str = 'little string'
    reconnect()
    --pass("trying repeated '*a' pattern")
    remote (string.format ([[
        data:send('%s')
        data:close()
        data = nil
    ]], str))
    back, err, partial = data:receive("*a")
    assert(back == str, "unexpected data read")
    back, err, partial = data:receive("*a")
    assert(back == nil and err == "closed", "should have returned 'closed'")

    reconnect()
    --pass("trying active close before '*a'")
    remote (string.format ([[
        data:close()
        data = nil
    ]]))
    data:close()
    back, err, partial = data:receive("*a")
    assert(back == nil and err == "closed", "should have returned 'closed'")

    reconnect()
    --pass("trying active close before '*l'")
    remote (string.format ([[
        data:close()
        data = nil
    ]]))
    data:close()
    back, err, partial = data:receive()
    assert(back == nil and err == "closed", "should have returned 'closed'")

    reconnect()
    --pass("trying active close before raw 1")
    remote (string.format ([[
        data:close()
        data = nil
    ]]))
    data:close()
    back, err, partial = data:receive(1)
    assert(back == nil and err == "closed", "should have returned 'closed'")

    reconnect()
    --pass("trying active close before raw 0")
    remote (string.format ([[
        data:close()
        data = nil
    ]]))
    data:close()
    back, err, partial = data:receive(0)
    assert(back == nil and err == "closed", "should have returned 'closed'")
end

function t:test_selectbugs()
    local r, s, e = socket.select(nil, nil, 0.1)
    assert(type(r) == "table" and type(s) == "table" and
        (e == "timeout" or e == "error"))
    --pass("both nil: ok")
    local udp = socket.udp()
    udp:close()
    r, s, e = socket.select({ udp }, { udp }, 0.1)
    assert(type(r) == "table" and type(s) == "table" and
        (e == "timeout" or e == "error"))
    --pass("closed sockets: ok")
    e = pcall(socket.select, "wrong", 1, 0.1)
    assert(e == false)
    e = pcall(socket.select, {}, 1, 0.1)
    assert(e == false)
    --pass("invalid input: ok")
end

function t:test_connect_timeout()
    local t = socket.gettime()
    local c, e = socket.tcp()
    assert(c, e)
    c:settimeout(0.1)
    local t = socket.gettime()
    local r, e = c:connect("10.0.0.1", 81)
    assert(not r, "should not connect")
    assert(socket.gettime() - t < 2, "took too long to give up.")
    c:close()
end


function t:test_empty_connect()
    reconnect()
    if data then data:close() data = nil end
    remote [[
        if data then data:close() data = nil end
        data = server:accept()
    ]]
    data, err = socket.connect("", port)
    if not data then
        data = socket.connect(host, port)
    else
        fail("gethostbyname returns localhost on empty string...")
    end
end

function t:test_connect_errors()
    local c, e = socket.connect("localhost", 1);
    assert(not c and e)
    local c, e = socket.connect("host.is.invalid", 1);
    assert(not c and e, e)
end


function t:test_rebind()
    local r
    local c = socket.bind("localhost", 0)
    local i, p = c:getsockname()
    local s, e = socket.tcp()
    assert(s, e)
    s:setoption("reuseaddr", false)
    r, e = s:bind("localhost", p)
    assert(not r, "managed to rebind!")
    assert(e)
end

local function isclosed(c)
    return c:getfd() == -1 or c:getfd() == (2^32-1)
end

function t:test_active_close()
    reconnect()
    if isclosed(data) then fail("should not be closed") end
    data:close()
    if not isclosed(data) then fail("should be closed") end
    data = nil
    local udp = socket.udp()
    if isclosed(udp) then fail("should not be closed") end
    udp:close()
    if not isclosed(udp) then fail("should be closed") end
end


function t:test_closed()
    local back, partial, err, total
    local str = 'little string'
    reconnect()
    --pass("trying read detection")
    remote (string.format ([[
        data:send('%s')
        data:close()
        data = nil
    ]], str))
    -- try to get a line
    back, err, partial = data:receive()
    if not err then fail("should have gotten 'closed'.")
    elseif err ~= "closed" then fail("got '"..err.."' instead of 'closed'.")
    elseif str ~= partial then fail("didn't receive partial result.")
    end
    reconnect()
    --pass("trying write detection")
    remote [[
        data:close()
        data = nil
    ]]
    total, err, partial = data:send(string.rep("ugauga", 100000))
    if not err then
        fail("failed: output buffer is at least %d bytes long!", total)
    elseif err ~= "closed" then
        fail("got '"..err.."' instead of 'closed'.")
    end
end


function t:test_accept_timeout()
    local s, e = socket.bind("*", 0, 0)
    assert(s, e)
    local t = socket.gettime()
    s:settimeout(1)
    local c, e = s:accept()
    assert(not c, "should not accept")
    assert(e == "timeout", string.format("wrong error message (%s)", e))
    t = socket.gettime() - t
    assert(t < 2, string.format("took to long to give up (%gs)", t))
    s:close()
end

function t:test_accept_errors()
    local d, e = socket.bind("*", 0)
    assert(d, e);
    local c, e = socket.tcp();
    assert(c, e);
    d:setfd(c:getfd())
    d:settimeout(2)
    local r, e = d:accept()
    assert(not r and e)
    local c, e = socket.udp()
    assert(c, e);
    d:setfd(c:getfd())
    local r, e = d:accept()
    assert(not r and e)
end

function t:test_getstats()
    reconnect()
    local t = 0
    for i = 1, 25 do
        local c = math.random(1, 100)
        remote (string.format ([[
            str = data:receive(%d)
            data:send(str)
        ]], c))
        data:send(string.rep("a", c))
        data:receive(c)
        t = t + c
        local r, s, a = data:getstats()
        assert(r == t, "received count failed" .. tostring(r)
            .. "/" .. tostring(t))
        assert(s == t, "sent count failed" .. tostring(s)
            .. "/" .. tostring(t))
    end
end


local function test_asciiline(len)
    reconnect()
    local str, str10, back
    str = string.rep("x", math.mod(len, 10))
    str10 = string.rep("aZb.c#dAe?", math.floor(len/10))
    str = str .. str10
    remote "str = data:receive()"
    assert(data:send(str.."\n"))
    remote "data:send(str ..'\\n')"
    back = assert(data:receive())
    if back == str then --pass("lines match")
    else fail("lines don't match") end
end


function t:test_asciiline()
    test_asciiline(1)
    test_asciiline(17)
    test_asciiline(200)
    test_asciiline(4091)
    test_asciiline(80199)
    test_asciiline(8000000)
    test_asciiline(80199)
    test_asciiline(4091)
    test_asciiline(200)
    test_asciiline(17)
    test_asciiline(1)
end



local function test_mixed(len)
    reconnect()
    local inter = math.ceil(len/4)
    local p1 = "unix " .. string.rep("x", inter) .. "line\n"
    local p2 = "dos " .. string.rep("y", inter) .. "line\r\n"
    local p3 = "raw " .. string.rep("z", inter) .. "bytes"
    local p4 = "end" .. string.rep("w", inter) .. "bytes"
    local bp1, bp2, bp3, bp4
    remote (string.format("str = data:receive(%d)",
    string.len(p1)+string.len(p2)+string.len(p3)+string.len(p4)))
    assert(data:send(p1..p2..p3..p4))
    remote "data:send(str); data:close()"
    bp1 = assert(data:receive())
    bp2 = assert(data:receive())
    bp3 = assert(data:receive(string.len(p3)))
    bp4 = assert(data:receive("*a"))
    if bp1.."\n" == p1 and bp2.."\r\n" == p2 and bp3 == p3 and bp4 == p4 then
        --pass("patterns match")
    else fail("patterns don't match") end
end

function t:test_mixed()
    test_mixed(1)
    test_mixed(17)
    test_mixed(200)
    test_mixed(4091)
    test_mixed(801990)
    test_mixed(4091)
    test_mixed(200)
    test_mixed(17)
    test_mixed(1)
end


local function test_rawline(len)
    reconnect()
    local str, str10, back, err
    str = string.rep(string.char(47), math.mod(len, 10))
    str10 = string.rep(string.char(120,21,77,4,5,0,7,36,44,100),
            math.floor(len/10))
    str = str .. str10
    remote "str = data:receive()"
    assert(data:send(str.."\n"))
    remote "data:send(str..'\\n')"
    back = assert(data:receive())
    if err then fail(err) end
    assert(back == str, "lines don't match")
end

function t:test_rawline()
    test_rawline(1)
    test_rawline(17)
    test_rawline(200)
    test_rawline(4091)
    test_rawline(80199)
    test_rawline(8000000)
    test_rawline(80199)
    test_rawline(4091)
    test_rawline(200)
    test_rawline(17)
    test_rawline(1)
end


local function test_raw(len)
    reconnect()
    local half = math.floor(len/2)
    local s1, s2, back, err
    s1 = string.rep("x", half)
    s2 = string.rep("y", len-half)
    remote (string.format("str = data:receive(%d)", len))
    assert(data:send(s1))
    assert(data:send(s2))
    remote "data:send(str)"
    back = assert(data:receive(len))
    assert( back == s1..s2, "blocks don't match")
end

function t:test_raw()
    test_raw(1)
    test_raw(17)
    test_raw(200)
    test_raw(4091)
    test_raw(80199)
    test_raw(8000000)
    test_raw(80199)
    test_raw(4091)
    test_raw(200)
    test_raw(17)
    test_raw(1)
end


local function test_nonblocking(size)
    reconnect()
    remote(string.format([[
        data:send(string.rep("a", %d))
        socket.sleep(0.5)
        data:send(string.rep("b", %d) .. "\n")
    ]], size, size))
    local err = "timeout"
    local part = ""
    local str, ret
    data:settimeout(0)
    while 1 do
        str, err, part = data:receive("*l", part)
        if err ~= "timeout" then break end
    end
    assert(str == (string.rep("a", size) .. string.rep("b", size)))
    reconnect()
    remote(string.format([[
        str = data:receive(%d)
        socket.sleep(0.5)
        str = data:receive(2*%d, str)
        data:send(str)
    ]], size, size))
    data:settimeout(0)
    local start = 0
    while 1 do
        ret, err, start = data:send(str, start+1)
        if err ~= "timeout" then break end
    end
    data:send("\n")
    data:settimeout(-1)
    local back = data:receive(2*size)
    assert(back == str, "'" .. back .. "' vs '" .. str .. "'")
end

function t:test_nonblocking()
    test_nonblocking(1)
    test_nonblocking(17)
    test_nonblocking(200)
    test_nonblocking(4091)
    test_nonblocking(80199)
    test_nonblocking(800000)
    test_nonblocking(80199)
    test_nonblocking(4091)
    test_nonblocking(200)
    test_nonblocking(17)
    test_nonblocking(1)
end



local function check_timeout(tm, sl, elapsed, err, opp, mode, alldone)
    if tm < sl then
        if opp == "send" then
            assert(not err or err == "timeout", err)
        else
            assert(err == "timeout")
        end
    else
        if mode == "total" then
            if elapsed > tm then
                assert(err == "timeout", err)
            elseif elapsed < tm then
                if err then fail(err) end
            else
                if alldone then
                    if err then fail("unexpected error '%s'", err) end
                else
                    assert(err == "timeout", err)
                end
            end
        else
            assert(not err, err)
        end
    end
end

local function test_totaltimeoutsend(len, tm, sl)
    reconnect()
    local str, err, total, elapsed, partial
    remote (string.format ([[
        data:settimeout(%d)
        str = data:receive(%d)
        print('server: sleeping for %ds')
        socket.sleep(%d)
        print('server: woke up')
        str = data:receive(%d)
    ]], 2*tm, len, sl, sl, len))
    data:settimeout(tm, "total")
    str = string.rep("a", 2*len)
    total, err, partial, elapsed = data:send(str)
    check_timeout(tm, sl, elapsed, err, "send", "total",
        total == 2*len)
end

function t:test_totaltimeoutsend()
    test_totaltimeoutsend(800091, 1, 3)
    test_totaltimeoutsend(800091, 2, 3)
    test_totaltimeoutsend(800091, 5, 2)
    test_totaltimeoutsend(800091, 3, 1)
end



local function test_totaltimeoutreceive(len, tm, sl)
    reconnect()
    local str, err, partial, elapsed
    remote (string.format ([[
        data:settimeout(%d)
        str = string.rep('a', %d)
        data:send(str)
        print('server: sleeping for %ds')
        socket.sleep(%d)
        print('server: woke up')
        data:send(str)
    ]], 2*tm, len, sl, sl))
    data:settimeout(tm, "total")
    local t = socket.gettime()
    str, err, partial, elapsed = data:receive(2*len)
    check_timeout(tm, sl, elapsed, err, "receive", "total",
    string.len(str or partial or "") == 2*len)
end

function t:test_totaltimeoutreceive()
    test_totaltimeoutreceive(800091, 1, 3)
    test_totaltimeoutreceive(800091, 2, 3)
    test_totaltimeoutreceive(800091, 3, 2)
    test_totaltimeoutreceive(800091, 3, 1)
end



local function test_blockingtimeoutsend(len, tm, sl)
    reconnect()
    local str, err, total, elapsed, partial
    remote (string.format ([[
        data:settimeout(%d)
        str = data:receive(%d)
        print('server: sleeping for %ds')
        socket.sleep(%d)
        print('server: woke up')
        str = data:receive(%d)
    ]], 2*tm, len, sl, sl, len))
    data:settimeout(tm)
    str = string.rep("a", 2*len)
    total, err,  partial, elapsed = data:send(str)
    check_timeout(tm, sl, elapsed, err, "send", "blocking",
        total == 2*len)
end

function t:test_blockingtimeoutsend()
    test_blockingtimeoutsend(128, 1, 3)
    test_blockingtimeoutsend(128, 2, 3)
    test_blockingtimeoutsend(128, 3, 2)
    test_blockingtimeoutsend(128, 3, 1)
end



local function test_blockingtimeoutreceive(len, tm, sl)
    reconnect()
    local str, err, partial, elapsed
    remote (string.format ([[
        data:settimeout(%d)
        str = string.rep('a', %d)
        data:send(str)
        print('server: sleeping for %ds')
        socket.sleep(%d)
        print('server: woke up')
        data:send(str)
    ]], 2*tm, len, sl, sl))
    data:settimeout(tm)
    str, err, partial, elapsed = data:receive(2*len)
    check_timeout(tm, sl, elapsed, err, "receive", "blocking",
        string.len(str or partial) == 2*len)
end

function t:test_blockingtimeoutreceive()
    test_blockingtimeoutreceive(128, 1, 3)
    test_blockingtimeoutreceive(128, 2, 3)
    test_blockingtimeoutreceive(128, 3, 2)
    test_blockingtimeoutreceive(128, 3, 1)
end
