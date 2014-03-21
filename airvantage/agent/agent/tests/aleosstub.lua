-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local u = require"unittest"
local devicetree = require 'devicetree'

--to check end-to-end management of notification, niltoken, etc, this test uses devicetree to do set/get.

local t=u.newtestsuite("aleosvars")


function t:setup()
    --enable aleos stub
    require"agent.devman.extvars.aleosstub"
    u.assert(racon.init())
    u.assert(devicetree.init())
end

local function get_val(leaf, vtype)
    local res = devicetree.get(leaf)
    u.assert_not_nil(res, "Cannot get leaf variable:"..leaf)
    u.assert(type(res) == vtype, "bad variable type")
end


--test all leaf types (string, integer, bool, ...)
function t:test_01_get_leaf()
    get_val("system.net.state", "string")-- string
    get_val("system.net.rssi", "number")-- integer
    get_val("system.test.bool", "boolean")--bool
    get_val("system.test.double", "number")--double
end

function t:test_02_get_node()
    local node = "system.net"
    local res, list = devicetree.get(node)
    u.assert_nil(res, "Failed listing node")
    u.assert_table(list, "Failed listing node");
    --listing result are not ordered, sort table before asserting content
    table.sort(list)
    u.assert_clone_tables({"channel", "ip", "link_state", "rssi", "service", "state"}, list)
end

function t:test_03_get_absent()
    local leaf = "system.somenonexistingleaf"
    local res, list = devicetree.get(leaf)
    u.assert_nil(res, "Getting absent node/leaf failed")
    u.assert_nil(list, "Getting absent node/leaf failed")
end



local function set_val(leaf, new_value)
    local res, err = devicetree.set(leaf, new_value)
    u.assert_not_nil(res, "Setting writable variable with correct type must success")
    u.assert_nil(err, "Setting writable variable with correct type must success")
    local res, err = devicetree.get(leaf)
    u.assert_not_nil(res, "Getting variable after Set failed")
    u.assert_nil(err,"Getting variable after Set failed")
    u.assert_equal(new_value, res, "Getting variable after Set failed did not returned written value")
end

-- set leafs of all types
function t:test_04_set_leaf()
    set_val("system.gps.latitude", 42);--integer
    set_val("system.test.bool", false)--boolean
    set_val("system.net.state", "plop")--string
    set_val("system.test.double", 3.14)--double
end

function t:test_05_set_badtype()
    local leaf = "system.gps.latitude"
    local res, err = devicetree.set(leaf, "sometext")
    u.assert_nil(res, "Setting variable with bad type must fail")
    u.assert_string(err, "Setting variable with bad type must return error string")
end

function t:test_06_set_leaf_absent()
    local leaf = "system.somenonexistingleaf"
    local res, err = devicetree.set(leaf, "somevalue")
    u.assert_nil(res, "Setting variable with bad type must fail")
    u.assert_string(err, "Setting variable with bad type must return error string")
end


function t:test_07_set_node()
    local node = "system.gps"
    local res, err = devicetree.set(node, "somevalue")
    u.assert_nil(res, "Setting node must fail")
    u.assert_string(err, "Setting node must fail with error string")
end

-- Test for register/notify stuff may use register+set functions to trigger notifications

--Until de-register function is implemented (at least for the high level function,


local function registerleaf()
    local leaf = "system.gps.latitude"
    local count = 1
    local values={31,32,33};
    local function callback(...)
        local t=...
        if type(t)== "table" then
            local tsize  = 0
            for k,v in pairs(t) do tsize = tsize+1 end
            if tsize==1 and t[leaf] then
                --p("callback1: leaf changed", ...)
                if t[leaf] ~= values[count] then
                    p("test_08_register_leaf: values notified in wrong order!!!!!")
                    sched.signal("aleosvars:test_08_register_leaf", "wrong order");
                    return
                else
                    count = count + 1
                    if count == 4 then sched.signal("aleosvars:test_08_register_leaf", "ok") end
                    if count > 4 then p("test_08_register_leaf: callback1 Called too much times!!!!!") end
                    return
                end
            end
            p("callback1: unexpected content", ...)
        end
    end

    local resid, err = devicetree.register(leaf, callback)
    u.assert_not_nil(resid, "Register on a leaf must success")
    u.assert_nil(err, "Register on a leaf must success")
    u.assert(devicetree.set(leaf, 31))
    u.assert(devicetree.set(leaf, 32))
    --add one more call with same value to set to check that set with previous value has no effect
    u.assert(devicetree.set(leaf, 32))
    u.assert(devicetree.set(leaf, 33))
    local ev = sched.wait("aleosvars:test_08_register_leaf", {"ok", "wrong order", 2})
    u.assert(devicetree.unregister(resid))
    u.assert_equal("ok", ev, "Did not get correctly notified of changes")

end



function t:test_08_register_leaf()
    for i =1, 1000,1 do registerleaf() end
end

function t:test_09_register_node()
    local node = "system.net"
    local count = 0
    local function callback2(...)
        local t=...
        --p("callback2:", ...)
        if type(t)== "table" then
            for k,v in pairs(t) do
                assert(type(k)=="string")
                assert(k:match("^"..node.."%.*"))
            end
            --p("callback2: node changed")
            count = count + 1
            if count == 3 then sched.signal("aleosvars:test_09_register_node", "ok") end
            if count > 3 then p("test_09_register_leaf: callback2 Called too much times!!!!!") end
            return
        end
        p("callback2: unexpected content", ...)
    end
    local resid, err = devicetree.register(node, callback2)
    u.assert_not_nil(resid, "Register on a node must success")
    u.assert_nil(err, "Register on a node must success")
    u.assert(devicetree.set("system.net.rssi", 55))
    u.assert(devicetree.set("system.net.link_state", 33))
    u.assert(devicetree.set("system.net.channel", 99))
    local ev = sched.wait("aleosvars:test_09_register_node", {"ok", 2})
    u.assert(devicetree.unregister(resid))
    u.assert(devicetree.set("system.net.rssi", 54))
    u.assert(devicetree.set("system.net.link_state", 32))
    u.assert(devicetree.set("system.net.channel", 93))
    u.assert_equal("ok", ev, "Did not get correctly notified of changes")
end


function t:test_10_register_mixed()
    local node = "system.test"
    local leaf = "system.test.double"
    local count = 0
    local function callback3(...)
        local t = ...
        for k,v in pairs(t) do
            assert(type(k)=="string")
            assert(k:match("^"..node.."%.*"))
        end

        --p("callback3: leaf changed", ...)
        count = count + 1
        if count == 3 then sched.signal("aleosvars:test_10_register_mixed", "ok") end
        if count > 3 then p("test_10_register_mixed: callback3 Called too much times!!!!!") end

    end

    local resid, err = devicetree.register(leaf, callback3)
    u.assert_not_nil(resid, "Register on a leaf must success")
    u.assert_nil(err, "Register on a leaf must success")
    local resid2, err = devicetree.register(node, callback3)
    u.assert_not_nil(resid2, "Register on a node must success")
    u.assert_nil(err, "Register on a node must success")

    u.assert(devicetree.set(leaf, 1.2))
    u.assert(devicetree.set("system.test.bool", true))
    local ev = sched.wait("aleosvars:test_10_register_mixed", {"ok", 2})
    u.assert(devicetree.unregister(resid))
    u.assert(devicetree.unregister(resid2))
    u.assert_equal("ok", ev, "Did not get correctly notified of changes")
end

--test niltoken is sent when some registred leaf/node is deleted.
function t:test_11_register_node_with_delete()
    local node = "system.test"
    local result
    local niltoken = require"niltoken"
    local function callback4(...)
        local t=...
        --p("callback4:", ...)
        if type(t)== "table" and t["system.test.bool"] == niltoken then
            result=true
            sched.signal("test_11_register_node_with_delete", "ok")
            return
        end
        p("callback4: unexpected content", ...)
    end

    local resid, err = devicetree.register(node, callback4)
    u.assert_not_nil(resid, "Register on a node must success")
    u.assert_nil(err, "Register on a node must success")
    u.assert(devicetree.set("system.test.bool", nil))
    local ev = sched.wait("test_11_register_node_with_delete", {"ok", 2})
    u.assert(devicetree.unregister(resid))
    u.assert_equal("ok", ev, "Did not get correctly notified of changes")
    u.assert(result, "did not received niltoken in callblack")
end

