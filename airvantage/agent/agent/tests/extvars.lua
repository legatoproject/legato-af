--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local u = require"unittest"
local devicetree = require 'devicetree'

--to check end-to-end management of notification, niltoken, etc, this test uses devicetree to do set/get.

local t=u.newtestsuite("extvars")


function t:setup()
    u.assert(devicetree.init())
end

local function get_val(leaf, vtype, val)
    local res = devicetree.get(leaf)
    u.assert(res, "Cannot get leaf variable:"..leaf)
    u.assert(type(res) == vtype, "bad variable type")
    u.assert_equal(val, res, "Bad value received")
end


--test all leaf types (string, integer, bool, ...)
--also test values reception
function t:test_01_get_leaf()
    get_val("treehdlsample.string_value", "string", "foo")-- string
    get_val("treehdlsample.int_value", "number", 42)-- integer
    get_val("treehdlsample.bool_value", "boolean", true)--bool
    get_val("treehdlsample.double_value", "number", 23.99)--double
end

function t:test_02_get_node()
    local node = "treehdlsample"
    local res, list = devicetree.get(node)
    u.assert_nil(res, "Failed listing node")
    u.assert_table(list, "Failed listing node");
    --listing result are not ordered, sort table before asserting content
    table.sort(list)
    u.assert_clone_tables({"treehdlsample.1", "treehdlsample.2", "treehdlsample.4", "treehdlsample.8",
			   "treehdlsample.bool_value", "treehdlsample.double_value", "treehdlsample.int_value", 
			   "treehdlsample.string_value"}, list)
end

function t:test_03_get_absent()
    local leaf = "treehdlsample.somenonexistingleaf"
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
    set_val("treehdlsample.string_value", "plop");--string
    set_val("treehdlsample.int_value", 666)--number
    set_val("treehdlsample.bool_value", false)--boolean
    set_val("treehdlsample.double_value", 3.14)--number
end

function t:test_05_set_leaf_absent()
    local leaf = "treehdlsample.somenonexistingleaf"
    local res, err = devicetree.set(leaf, "somevalue")
    u.assert_nil(res, "Setting non existing variable must fail")
    u.assert_string(err, "Setting non existing variable must return error string")
end


function t:test_06_set_node()
    local node = "treehdlsample"
    local res, err = devicetree.set(node, "somevalue")
    u.assert_nil(res, "Setting node must fail")
    u.assert_string(err, "Setting node must fail with error string")
end

-- Test for register/notify stuff may use register+set functions to trigger notifications

--Until de-register function is implemented (at least for the high level function,


function t:test_07_register_leaf()
    local leafs = { "treehdlsample.double_value", "treehdlsample.int_value", "treehdlsample.string_value" }
    local count = 1
    local values={ ["treehdlsample.double_value"] =  {31.1,32.2,33.3}, ["treehdlsample.int_value"] = {31,32,33}, 
		   ["treehdlsample.string_value"] = {"foo", "bar", "foo"} }
    local cur_leaf=nil
    local function callback(...)
        local t=...
        if type(t)== "table" then
            local tsize  = 0
            for k,v in pairs(t) do tsize = tsize+1 end
            if tsize==1 and t[cur_leaf] then
	        if t[cur_leaf] ~= values[cur_leaf][count] then
                    p("test_07_register_leaf: values notified in wrong order!!!!!")
                    sched.signal("test_07_register_leaf", "wrong order");
                    return
                else
                    count = count + 1
                    if count == 4 then sched.signal("test_07_register_leaf", "ok") end
                    if count > 4 then p("test_07_register_leaf: callback1 Called too much times!!!!!") end
                    return
                end
            end
            p("callback1: unexpected content", ...)
        end
    end

    for _, leaf in pairs(leafs) do
       cur_leaf = leaf
       local resid, err = devicetree.register(leaf, callback)
       u.assert_not_nil(resid, "Register on a leaf must success")
       u.assert_nil(err, "Register on a leaf should return one argument")
       u.assert(devicetree.set(leaf, values[leaf][1]))
       u.assert(devicetree.set(leaf, values[leaf][2]))
       u.assert(devicetree.set(leaf, values[leaf][3]))
       local ev = sched.wait("test_07_register_leaf", {"ok", "wrong order", 2})
       u.assert(devicetree.unregister(resid))
       u.assert_equal("ok", ev, "Did not get correctly notified of changes")
       count = 1
    end
end

function t:test_08_register_node()
    local node = "treehdlsample"
    local count = 1
    local leafs = { "treehdlsample.double_value", "treehdlsample.int_value", "treehdlsample.bool_value", "treehdlsample.string_value" }
    local values = { 3.14, 666, true, "foobar"}
    local leaf = nil
    local function callback2(...)
        local t=...
        if type(t)== "table" then
	   if t[leaf] and t[leaf] == values[count] then 
	      sched.signal("test_08_register_node", "ok")
	   else
	      p("test_08_register_leaf: wrong order!!!")
	      sched.signal("test_08_register_node", "wrong order")
	   end
	   return
        end
        p("callback2: unexpected content", ...)
    end
    local resid, err = devicetree.register(node, callback2)
    u.assert_not_nil(resid, "Register on a node must success")
    u.assert_nil(err, "Register on a node must success")

    for i = 1, 1 do
       leaf = leafs[i]
       u.assert(devicetree.set(leaf, values[i]))
       local ev = sched.wait("test_08_register_node", {"ok", 2})
       u.assert_equal("ok", ev, "Did not get correctly notified of changes")
       count = count + 1
    end
    u.assert(devicetree.unregister(resid))
end
