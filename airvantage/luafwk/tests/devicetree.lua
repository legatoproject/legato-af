-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local devicetree = require 'racon.devicetree'
local u = require 'unittest'
local t = u.newtestsuite("devicetree")

local variableHasChanged = 0
local regId = nil

local function notifyVariablesCb()
   variableHasChanged = 1
end

function t :setup()
   assert(devicetree.init())
end

function t :teardown()
   assert(variableHasChanged == 1)
end

function t :test_dt_Register()
   local id, err

   while true do
      id, err = devicetree.register({ "config.toto", "config.tata" }, notifyVariablesCb)
      if id and not err then break end
      if not id and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end
   regId = id
end

function t :test_dt_SetAndGet()

   while true do
      local status, err = devicetree.set("config.toto", "toto")
      if status and not err then break end
      if not status and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end

   while true do
      local status, err = devicetree.set("config.tata", "tataw")
      if status and not err then break end
      if not status and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end

   while true do
      local status, err = devicetree.set("config.tata", "tata")
      if status and not err then break end
      if not status and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end

   local value, err
   while true do
      value, err = devicetree.get("config.toto")
      if value and not err then break end
      if not value and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end
   assert(type(value) == "string" and value == "toto")

   while true do
      value, err = devicetree.get("config.tata")
      if value and not err then break end
      if not value and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end
   assert(type(value) == "string" and value == "tata")

   while true do
      value, err = devicetree.get("config")
      if not value and err then break end
      if not value and type(err) == "string" and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end
   assert(value == nil and type(err) == "table")

   while true do
      value, err = devicetree.get("unexisting_node")
      if not value and err ~= "CLOSED" then break end
      sched.wait(1)
   end
   assert(err == "NOT_FOUND")
end

function t :test_dt_GetAndSetPrefixed()
   local value, list

   local status, err = devicetree.set("config.test", { ["key1"] = "value1", ["key2"] = "value2", ["key3"] = "value3" })
   u.assert_not_nil(status)
   u.assert_nil(err)

   value, list = devicetree.get("config", "test")
   u.assert_nil(value)
   u.assert_clone_tables({"config.test.key2", "config.test.key3", "config.test.key1"}, list)

   value, list = devicetree.get("config.test", "")
   u.assert_nil(value)
   u.assert_clone_tables({"key1", "key3", "key2"}, list)
end

function t :test_dt_CallbackAck()
    local id = devicetree.register("config.toto", function(...) return "ok" end)
    local res, err = devicetree.set("config.toto", "totow")
    u.assert(res == "ok" and not err)
    devicetree.unregister(id)

    id = devicetree.register("config.toto", function(...) end)
    local res, err = devicetree.set("config.toto", "toto")
    u.assert(res == "unhandled")
    devicetree.unregister(id)

    id = devicetree.register("config.toto", function(...) return nil, "test error" end)
    local res, err = devicetree.set("config.toto", "totow")
    u.assert(not res and type(err) == "table" and err[1] == "test error")
    devicetree.unregister(id)
end

function t :test_dt_ExtVarsErrorHandling()
   -- On the tree handler side, adding new leaf dynamically with an hw path as argument is not possible by default
   local res, err = devicetree.set("treehdlsample.666", "dummy_value")
   u.assert(not res and err:match("NOT_FOUND"))

   -- This is not permitted to change the type of a static value (declared inside the .map) 
   local res, err = devicetree.set("treehdlsample.int_value", "dummy_value")
   u.assert(not res and err:match("NOT_PERMITTED"))

   -- This is not allowed to store a non primitive type inside a leaf (at least, not for now)
   local res, err = devicetree.set("treehdlsample.int_value", { ["dummy"] = "dummy_table_value" })
   u.assert(not res and err:match("BAD_PARAMETER"))

   -- On extvars side, adding new leaf dynamically with a logical path is not possible by default.
   -- The treemgr does not find the corresponding hw path, which should be an interger for C handlers, 
   -- this is why error "BAD_PARAMETER" is thrown.
   local res, err = devicetree.set("treehdlsample.dummy_leaf", { "dummy_value" })
   u.assert(not res and err:match("BAD_PARAMETER"))
end

function t: test_dt_binary_string()
   local id = {}
   for i = 0, 255, 1 do
      table.insert(id,i)
   end
   local value = string.char(unpack(id))
   local res, err = devicetree.set("config.toto", value)
   u.assert(res == "ok")
   u.assert(not err)

   res, err = devicetree.get("config.toto")
   u.assert(res == value)
   u.assert(not err)
end

function t :test_dt_Unregister()
   while true do
      local status, err = devicetree.unregister(regId)
      if status and not err then break end
      if not status and err ~= "CLOSED" then assert(nil) end
      sched.wait(1)
   end
end

return t
