-------------------------------------------------------------------------------
-- Copyright (c) 2013 Sierra Wireless and others.
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

local racon = require 'racon'
local u = require 'unittest'
local t = u.newtestsuite("racon")
local waiting_notification = 1
local ASSET_ID = "racon_asset_id"

local function updateNotificationCb(package, version, url, parameters)
   if package == "my_pkg" and version == "my_version" and url == "/toto/my_file" then
      waiting_notification = 0
   end
end

local function dataWritingCb()
   
end

function t :setup()
   u.assert(racon.init())
end

function t :teardown()
   u.assert(waiting_notification == 0)
end

function t :test_01_Init()
   -- Initializing the library many times must work
   u.assert(racon.init())
   u.assert(racon.init())
   u.assert(racon.init())
end

function t :test_02_TriggerPolicy()
   -- Trigger default policy
   u.assert(racon.triggerPolicy(nil))
   -- Trigger one existing policy
   u.assert(racon.triggerPolicy("now"))
   -- Trigger "never" policy: must fail
   u.assert_nil(racon.triggerPolicy("never"))
   -- Trigger unknown policy: must fail
   u.assert_nil(racon.triggerPolicy("plop"))
end

function t: test_03_connectToServer()
   -- Test using sync connection
   u.assert(racon.connectToServer())
   u.assert(racon.connectToServer(nil))
   u.assert(racon.connectToServer(0))

   u.assert(racon.connectToServer(10))
   -- Test using negative latency
   -- expected behavior here: negative value casted into unsigned int
   -- resulting value above INT_MAX, the max value accepted, so it's rejected
   u.assert_nil(racon.connectToServer(-5))
end

function t: test_04_Create_Start_Close()
   local asset = racon.newAsset(ASSET_ID)
   u.assert(asset)
   u.assert(asset:start())
   u.assert(asset:close())
end

function t: test_05_asset_pushData()
   local asset = racon.newAsset(ASSET_ID)
   u.assert(asset)
   u.assert(asset:start())

   u.assert(asset:pushData("titi.test.toto1", 42, "now"))
   u.assert(asset:pushData("titi.toto2", 43, "now"))
   u.assert(asset:pushData("toto3", 44, "now"))
   u.assert(asset:pushData("toto4", 45, "now"))
   u.assert(asset:pushData("toto5", 46, "now"))
   u.assert(asset:pushData("toto6", 47, "now"))
   u.assert(asset:pushData("toto7", 47.455555, "now"))
   u.assert(asset:pushData("toto8", "foo", "now"))
   u.assert(asset:pushData("toto8", { nil }, "now"))
   u.assert(racon.triggerPolicy("*"))
   u.assert(asset:close())
end

function t: test_06_Acknowledge()
   u.assert(racon.acknowledge(0, 0, "BANG BANG BANG", "now", 0))
   u.assert(racon.triggerPolicy("now"))
end

function t: test_07_UpdateNotification()
   local rpc = require 'rpc'
   local asset = racon.newAsset(ASSET_ID)
   u.assert(asset)

   u.assert(asset:start())
   u.assert(asset:setUpdateHook(updateNotificationCb))
   rpc.newclient("localhost", 2012):call('agent.asscon.sendcmd', 'racon_asset_id', 'SoftwareUpdate', { 'racon_asset_id.my_pkg', 'my_version', '/toto/my_file', {foo='bar', num=42, float=0.23}})

   u.assert(asset:close())
end

function t: test_08_TableManipulation()
   local asset = racon.newAsset(ASSET_ID)
   u.assert(asset)

   u.assert(asset:start())
   local t = asset:newTable("test", {"column1", "column2", "column3"}, "ram", "now", 0)
   u.assert(t)

   u.assert(t:pushRow({ 1234, 1234.1234, "test" }))
   u.assert(t:pushRow({ 1234, 1234.1234, "test", "test2", "test3" }))
   print("FIXME!!")
   --FIXME: t:pushRow should return an error in case of the pushed data is > the number of column 
   -- or should return a notification explaining the row has been sent automatically
   u.assert(asset:close())
end
