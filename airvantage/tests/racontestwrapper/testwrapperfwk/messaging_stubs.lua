-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local M = {}

local function remote_patching()
   local rpc = require 'rpc'
   local sched = require 'sched'

   local client = rpc.newclient()
   if not client then
      print("Ready agent not started")
      return
   end

   local init_config = client:newexec(function(...)
					 local sms = require 'agent.asscon.sms'
					 local update = require 'agent.update'
					 package.loaded["messaging"].init = function()
					    local internal = require 'messaginghal'
					    internal.init()
					    return "ok"
					 end
					 package.loaded["messaging"].sendSMS = function(recepient, message, format)
					    print("sendSMS: recepient = " .. tostring(recepient) .. ", message = " .. tostring(message) .. ", format = " .. tostring(format))
					    return "ok"
					 end
					 sms.init()
					 update.init()
				      end)
   init_config()
   sched.signal("messaging_stubs", "patched")
end

function M.setup(dir)
   sched.run(remote_patching)
   sched.wait("messaging_stubs", "patched")
end

return M
