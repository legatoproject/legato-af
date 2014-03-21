-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require 'sched'
local rpc = require 'rpc'
local u = require 'unittest'



local t=u.newtestsuite("rpc")


local server
local client


function t:setup()
    local err0
    sched.run(function() server, err0 = rpc.newserver("localhost", 5678) end)
    sched.wait()
    local err1
    client, err1 = rpc.newclient("localhost", 5678)
    u.assert_table(server, err0)
    u.assert_table(client, err1)
    rawset(_G, "RPCxUnitCall", true) -- prevent strict to cry...
    rawset(_G, "RPCxUnitVar", true)
end

function t:test_print()
    local r
    u.assert_no_error(function() r = {client:call("print", "hello from client")} end)
    u.assert(next(r) == nil) -- print call return 0 arguments
    u.assert_no_error(function() r = {server:call("print", "hello from server")} end)
    u.assert(next(r) == nil) -- print call return 0 arguments
end

function t:test_eval()
    _G.RPCxUnitVar = 1
    u.assert_equal(_G.RPCxUnitVar, client:eval("RPCxUnitVar"))
    _G.RPCxUnitVar = "Some String"
    u.assert_equal(_G.RPCxUnitVar, client:eval("RPCxUnitVar"))
    _G.RPCxUnitVar = {t={1, 2, 3}, "a", "b", "c"}
    u.assert_clone_tables(_G.RPCxUnitVar, client:eval("RPCxUnitVar"))
end

function t:test_newexec_string()
    local f = client:newexec("return ... + 1")
    u.assert_function(f)
    u.assert_no_error(function() f(1) end)
    u.assert_equal(43, f(42))

    f = client:newexec("error('hi')")
    u.assert_function(f)
    u.assert_error(f)
end

function t:test_newexec_stringbuffer()
    local f = client:newexec({"return", " ... + 1"})
    u.assert_function(f)
    u.assert_no_error(function() f(1) end)
    u.assert_equal(43, f(42))
end

function t:test_newexec_function()
    local function g(x) return x+1 end
    local f = client:newexec(g)
    u.assert_function(f)
    u.assert_no_error(function() f(1) end)
    u.assert_equal(43, f(42))

    local y = 1
    function g(x) return x+y end
    u.assert_error(function() f = client:newexec(g) end)
end


function t:test_call_basic()
    local called
    _G.RPCxUnitCall = function() called = true end
    client:call("RPCxUnitCall")
    u.assert_true(called)
end

function t:test_call_multiple_params()
    local called
    _G.RPCxUnitCall = function(a, b, c, d, e) called = c or a..b..d..e end
    client:call("RPCxUnitCall", "Hello", " ", nil, "there", "!")
    u.assert_equal("Hello there!", called)
end

function t:test_call_multiple_return()
    _G.RPCxUnitCall = function() return "a", nil, "three" end
    local r = {client:call("RPCxUnitCall")}
    u.assert_table(r)
    u.assert_clone_tables({[1]="a", [3]="three"}, r)
end

function t:test_call_error()
    _G.RPCxUnitCall = function() error({"some error"}) end
    u.assert_error(function() client:call("RPCxUnitCall") end)
    local r = {copcall(client.call, client, "RPCxUnitCall")}
    u.assert_false(r[1])
    u.assert_clone_tables({"some error"}, r[2])
end

function t:test_call_notfound()
    local _, err = client:call("rpc.RPCxUnitCallxxx")
    u.assert_equal("function not existing", err)
end

function t:test_call_notafunc()
    _G.RPCxUnitVar = 1
    local _, err = client:call("RPCxUnitVar")
    u.assert_equal("not a function", err)
end


function t:test_client_close_before_call()
    local s, c
    sched.run(function() s = rpc.newserver(nil, 4567) end)
    sched.wait()
    c = rpc.newclient(nil, 4567)
    sched.wait()
    c.skt:close()
    c = nil
    sched.wait()
    local r, err = s:eval("print")
    u.assert_nil(r)
    u.assert_equal("closed", err)
end

function t:test_client_close_while_calling()
    local s, c, errs, errc
    sched.run(function() s, errs = rpc.newserver(nil, 1234) end)
    sched.wait()
    c, errc = rpc.newclient(nil, 1234)
    u.assert_table(s)
    u.assert_nil(errs)
    u.assert_table(c)
    u.assert_nil(errc)
    local r, err
    t = sched.run(function() r, err = s:call("sched.wait", 5)  end)
    sched.wait()
    c.skt:close()
    c = nil
    sched.wait(t, "die")
    u.assert_nil(r)
    u.assert_equal("closed", err)
end

function t:test_serveronly()
    local s, err = rpc.newmultiserver("*", 7890)
    u.assert_userdata(s, err)
    local c, err = rpc.newclient(nil, 7890)
    u.assert_table(c)
    u.assert_nil(err)

    _G.RPCxUnitVar = 1
    u.assert_equal(_G.RPCxUnitVar, c:eval("RPCxUnitVar"))

    s:close()
    c.skt:close()
end




function t:teardown()
    client.skt:close()
    server.skt:close()
    client = nil
    server = nil
    _G.RPCxUnitCall = nil
    _G.RPCxUnitVar = nil
end