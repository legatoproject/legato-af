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

local sched  = require 'sched'
local M = {}

local function remote_patching()
   local rpc = require 'rpc'

   local client = rpc.newclient()
   if not client then
      print("Ready agent not started")
      return
   end

   local init_config = client:newexec(function(...)
					 require "agent.system"
					 package.loaded["agent.system"].reboot = function(delay, reason)
					    local sched = require 'sched'
					    log("ASSCON", "INFO", "Reboot requested, reason: %s", tostring(reason))
					    delay = ((tonumber(delay) or 5) > 5) and tonumber(delay) or 5
					    sched.signal("system", "stop", "reboot")
					    sched.run(function() sched.wait(delay); log("ASSCON", "INFO", "system_stubs: Rebooting...") end)
					 end
				      end)
   init_config()
   sched.signal("system_stubs", "patched")
end

function M.setup(dir)
   sched.run(remote_patching)
   sched.wait("system_stubs", "patched")
end

return M
