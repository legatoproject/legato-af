-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local mon = require'agent.monitoring'
local u = require 'unittest'
local config = require 'agent.config'
local tm = require 'agent.treemgr'
local niltoken = require 'niltoken'



local t=u.newtestsuite("monitoring")
local varproxy

function t:setup()
    if not config.get("monitoring.activate") then u.abort("Monitoring not enabled in RA config, cannot run this test suite") end
    tm.set("ram.testexisting", "test")
    if tm.get(nil, "ram.testexisting") ~= "test" then u.abort("Need a working ramstore in order to do monitoring tests") end
    varproxy = require'agent.treemgr.table'.ram
end


--------------------------------------------------------------------------------
-- test triggers

function t:test_trigger_onchange1()
    u.assert(tm.set("ram.tests4.t1", 0))
    u.assert(tm.set("ram.tests4.t2", 0))
    local vars = varproxy.tests4
    local nvar
    local ngroup
    local function test()
        connect(onchange("ram.tests4.t1"), function(v) nvar = v end)
        connect(onchange("ram.tests4"), function(v) ngroup = v end)
    end
    u.assert(mon.load("tests4", test))
    sched.wait()
    
    nvar, ngroup = nil, nil
    vars.t2 = 1; sched.wait()
    u.assert_equal(nil, nvar)
    u.assert_clone_tables({["ram.tests4.t2"]=1}, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = 1; sched.wait();
    u.assert_clone_tables({["ram.tests4.t1"]=1}, nvar)
    u.assert_clone_tables({["ram.tests4.t1"]=1}, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = 1; sched.wait()
    u.assert_equal(nil, nvar)
    u.assert_equal(nil, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = nil; sched.wait()
    u.assert_clone_tables({["ram.tests4.t1"]=niltoken}, nvar)
    u.assert_clone_tables({["ram.tests4.t1"]=niltoken}, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = 1; sched.wait()
    u.assert_clone_tables({["ram.tests4.t1"]=1}, nvar)
    u.assert_clone_tables({["ram.tests4.t1"]=1}, ngroup)
end


-- Test registration on a non existing variable
function t:test_trigger_onchange2()
    u.assert(tm.set("ram.tests7.t1", 0))
    local ok
    local function test()
        connect(onchange("ram.tests7.t1"), function() ok=true end)
    end
    u.assert(mon.load("tests7", test))
    u.assert_equal(nil, ok)
    u.assert(tm.set("ram.tests7.t1", 1))
    sched.wait()
    u.assert_equal(true, ok)
end


function t:test_trigger_onhold1()
    u.assert(tm.set("ram.tests5.t1", 0))
    u.assert(tm.set("ram.tests5.t2", 0))
    local vars = varproxy.tests5
    local nvar
    local ngroup
    local function test()
        connect(onhold(3, "ram.tests5.t1"), function(v) nvar = v or true end)
        connect(onhold(3, "ram.tests5."), function(v) ngroup = v or true end)
    end
    u.assert(mon.load("tests5", test))

    nvar, ngroup = nil, nil
    sched.wait(5)
    u.assert_equal(true, nvar)
    u.assert_equal(true, ngroup)

    nvar, ngroup = nil, nil
    vars.t2 = 1; sched.wait(5)
    u.assert_equal(nil, nvar)
    u.assert_clone_tables({["ram.tests5.t2"]=1}, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = 1; sched.wait(5)
    u.assert_clone_tables({["ram.tests5.t1"]=1}, nvar)
    u.assert_clone_tables({["ram.tests5.t1"]=1}, ngroup)

    nvar, ngroup = nil, nil
    vars.t1 = 2; sched.wait(1)
    u.assert_equal(nil, nvar)
    u.assert_equal(nil, ngroup)
    vars.t1 = 3; sched.wait(1)
    u.assert_equal(nil, nvar)
    u.assert_equal(nil, ngroup)
    vars.t1 = 4; sched.wait(1)
    u.assert_equal(nil, nvar)
    u.assert_equal(nil, ngroup)
    vars.t1 = 6; sched.wait(5)
    u.assert_clone_tables({["ram.tests5.t1"]=6}, nvar)
    u.assert_clone_tables({["ram.tests5.t1"]=6}, ngroup)
end

-- Test on a periodic onhold
function t:test_trigger_onhold2()
    u.assert(tm.set("ram.tests3.t1", 0))
    local vars = varproxy.tests3
    local count = 0
    local function test()
        connect(onhold(-4, "ram.tests3.t3"), function(v) count = count+1 end)
    end
    u.assert(mon.load("tests3", test))

    u.assert_equal(0, count)
    sched.wait(9)
    u.assert_equal(2, count)
    vars.t3 = 2;  sched.wait(1)
    vars.t3 = 3;  sched.wait(1)
    vars.t3 = 4;  sched.wait(1)
    vars.t3 = 5;  sched.wait(1)
    vars.t3 = 6;  sched.wait(1)
    vars.t3 = 7;  sched.wait(1)
    vars.t3 = 8;  sched.wait(1)
    vars.t3 = 9;  sched.wait(1)
    vars.t3 = 10; sched.wait(1)
    vars.t3 = 11; sched.wait(1)
    vars.t3 = 12; sched.wait(1)
    vars.t3 = 13; sched.wait(1)
    u.assert_equal(2, count)
end

-- Test multiple holding variables
function t:test_trigger_onhold3()
    u.assert(tm.set("ram.tests9.t1", 0))
    u.assert(tm.set("ram.tests9.t2", 0))
    u.assert(tm.set("ram.tests9.t3", 0))
    u.assert(tm.set("ram.tests9.t4", 0))
    local vars = varproxy.tests9
    local count = 0
    local function test()
        connect(onhold(3, "ram.tests9.t1", "ram.tests9.t2", "ram.tests9.t3", "ram.tests9.t4"), function(v) count = count+1 end)
    end
    u.assert(mon.load("tests9", test))

    u.assert_equal(0, count)
    sched.wait(5)
    u.assert_equal(1, count)
    vars.t1 = 2; sched.wait(1)
    vars.t2 = 3; sched.wait(1)
    vars.t3 = 4; sched.wait(1)
    vars.t4 = 5; sched.wait(1)
    vars.t1 = 6; sched.wait(1)
    vars.t2 = 7; sched.wait(1)
    vars.t3 = 8; sched.wait(1)
    vars.t4 = 9; sched.wait(1)
    u.assert_equal(1, count)
end


function t:test_trigger_onperiod1()
    local count = 0
    local function test()
        connect(onperiod(2), function(v) count = count+1 end)
    end
    u.assert(mon.load("tests_onperiod1", test))

    u.assert_equal(0, count)
    sched.wait(5)
    u.assert_equal(2, count)
end

function t:test_trigger_onperiod2()
    local count = 0
    local function test() connect(onperiod(0), function(v) count = count+1 end) end
    u.assert(mon.load("tests_onperiod2.1", test))
    sched.wait(2)
    u.assert_equal(1, count)

    count = 0
    local function test() connect(onperiod(-1), function(v) count = count+1 end) end
    u.assert(mon.load("tests_onperiod2.2", test))
    u.assert_equal(0, count)
    sched.wait(2)
    u.assert_equal(1, count)
end

--onconnect trigger is deactivated until a proper srvcon architecture allows a propoer implementation
-- function t:test_trigger_onconnect()
--     local count = 0
--     local function test()
--         connect(onconnect(), function(v) count = count+1 end)
--     end
--     u.assert_table(mon.install("test", test, "now"))
-- 
--     local s = require 'agent.srvcon'
--     s.connect()
--     u.assert_equal(1, count)
--     s.connect()
--     u.assert_equal(2, count)
-- end


--onthreshold(threshold, var, edge)
function t:test_trigger_onthreshold1()
    u.assert(tm.set("ram.tests6.t1", 0)) -- create tests6 sub table
    local vars = varproxy.tests6
    local c0, c1, c2
    local function test()
        connect(onthreshold(42, "ram.tests6.t1", "down"), function() c0 = true  end)
        connect(onthreshold(42, "ram.tests6.t1", "up"), function() c1 = true end)
        connect(onthreshold(42, "ram.tests6.t1"), function() c2 = true end)
    end
    u.assert(mon.load("tests6", test))

    c0, c1, c2 = nil, nil, nil
    vars.t1 = 10
    vars.t1 = 40
    vars.t1 = 41
    sched.wait()
    u.assert_nil(c0)
    u.assert_nil(c1)
    u.assert_nil(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t1 = 42
    sched.wait()
    u.assert_nil(c0)
    u.assert_true(c1)
    u.assert_true(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t1 = 44
    vars.t1 = 3215
    vars.t1 = 42
    sched.wait()
    u.assert_nil(c0)
    u.assert_nil(c1)
    u.assert_nil(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t1 = 41
    sched.wait()
    u.assert_true(c0)
    u.assert_nil(c1)
    u.assert_true(c2)
end

function t:test_trigger_onthreshold2()
    u.assert(tm.set("ram.tests1.t1", 0)) -- create tests1 sub table
    local vars = varproxy.tests1
    local c0, c1, c2
    local function test()
        connect(onthreshold(-42, "ram.tests1.t2", "down"), function() c0 = true  end)
        connect(onthreshold(-42, "ram.tests1.t2", "up"), function() c1 = true end)
        connect(onthreshold(-42, "ram.tests1.t2"), function() c2 = true end)
    end
    u.assert(mon.load("tests1", test))

    c0, c1, c2 = nil, nil, nil
    vars.t2 = -42
    sched.wait()
    u.assert_nil(c0)
    u.assert_nil(c1)
    u.assert_nil(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t2 = -43
    sched.wait()
    u.assert_true(c0)
    u.assert_nil(c1)
    u.assert_true(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t2 = -44
    vars.t2 = -3215
    vars.t2 = -43
    sched.wait()
    u.assert_nil(c0)
    u.assert_nil(c1)
    u.assert_nil(c2)

    c0, c1, c2 = nil, nil, nil
    vars.t2 = -42
    sched.wait()
    u.assert_nil(c0)
    u.assert_true(c1)
    u.assert_true(c2)


    c0, c1, c2 = nil, nil, nil
    vars.t2 = 45
    sched.wait()
    u.assert_nil(c0)
    u.assert_nil(c1)
    u.assert_nil(c2)
end


--ondeadband(deadband, var)
function t:test_trigger_ondeadband()
    u.assert(tm.set("ram.tests2.t1", 0)) -- create tests2 sub table
    u.assert(tm.set("ram.tests2.t3", 0))
    local t = varproxy.tests2
    local c
    local function test()
        connect(ondeadband(5, "ram.tests2.t3", "down"), function() c = true  end)
    end
    u.assert(mon.load("tests2", test))

    c = nil
    t.t3 = 1
    t.t3 = 2
    t.t3 = 4
    t.t3 = -4
    t.t3 = 2
    sched.wait()
    u.assert_nil(c)

    c = nil
    t.t3 = 6
    sched.wait()
    u.assert_true(c)

    c = nil
    t.t3 = 145
    sched.wait()
    u.assert_true(c)

    c = nil
    t.t3 = -69
    sched.wait()
    u.assert_true(c)

    c = nil
    t.t3 = -145
    sched.wait()
    u.assert_true(c)

    c = nil
    t.t3 = -141
    t.t3 = -149
    t.t3 = -145
    t.t3 = -141
    sched.wait()
    u.assert_nil(c)
end


-- Sub function to make sure all local varaibles are collected when returning i nthe calling function
local function do_unload_load()
    -- Add all trigger in the rule in order to test them all
    local function test()
         connect(onchange("ram.tests8.t1"), function() end)
         connect(onhold(1, "ram.tests8.t1"), function() end)
         connect(onperiod(1), function() end)
         connect(ondate("* * * * *"), function() end)
         connect(onboot(), function() end)
         connect(onthreshold(1, "ram.tests8.t1"), function() end)
         connect(ondeadband(1, "ram.tests8.t1"), function() end)
    end

    local count
    for count = 1, 1000 do u.assert(mon.load("tests8_"..count, test)) end
    t.t1 = 1; sched.wait(1)
    for count = 1, 1000 do u.assert(mon.unload("tests8_"..count)) end
    --sched.wait()
    --collectgarbage("collect")
    --print("after1", collectgarbage("count"))
end

-- Test if there is no memory leek when loading/unloading a large number of scripts
function t:test_trigger_load_unload()
    u.assert(tm.set("ram.tests8.t1", 0)) -- create tests8 sub table
    local t = varproxy.tests8

    

    do_unload_load()
    sched.wait(1)
    collectgarbage("collect")
    local bc_count1 = collectgarbage("count")
    do_unload_load()
    do_unload_load()
    do_unload_load()
    sched.wait(1)
    collectgarbage("collect")
    sched.wait(1)
    collectgarbage("collect")
    sched.wait(1)
    collectgarbage("collect")
    local bc_count2 = collectgarbage("count")
    
    --print(bc_count1, bc_count2)
    u.assert(bc_count1>=bc_count2*0.95, string.format("Memory leak detected: more than 1%% of ram was not freed: before %d, after %d", bc_count1, bc_count2))

end

