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

local sms = require 'racon.sms'
local u = require 'unittest'
local t = u.newtestsuite("sms")
local rpc = require 'rpc'

local regId = nil
local receivedSMS = 0
local SENDERP = "33606060606"
local MESSAGEP= "TEST MESSAGE"

local function sms_handler(sender, message)
   if sender == SENDERP and message == MESSAGEP then
      receivedSMS = 1
   end
end

function t :setup()
   assert(sms.init())
end

function t :teardown()
   u.assert_equal(receivedSMS, 1)
end

function t :test_Register()
   local s, err
   while true do
      s, err = sms.register(sms_handler, SENDERP, MESSAGEP)
      if s and not err then break end
      if not s and err ~= "error 517 [hint: ipc broken]" then assert(nil) end
      sched.wait(1)
   end
   assert(type(s) == "number" and s > 0)
   regId = s
   rpc.newclient():call('sched.signal', 'messaging', 'sms', {message="TEST MESSAGE", address="33606060606"})
end

function t: test_Unregister()
   local s, err
   while true do
      s, err = sms.unregister(regId)
      if s and not err then break end
      if not s and err ~= "error 517 [hint: ipc broken]" then assert(nil) end
      sched.wait(1)
   end

   s, err = sms.unregister(666)
   assert(not s and err == "invalid id to unregister")
end

function t :test_SendSMS()
   local s, err
   while true do
      s, err = sms.send(SENDERP, MESSAGEP, "8bits")
      if s and not err then break end
      if not s and err ~= "error 517 [hint: ipc broken]" then assert(nil) end
      sched.wait(1)
   end
end
