-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local system = require 'racon.system'
local sched  = require 'sched'
local u = require 'unittest'
local t = u.newtestsuite("system")

local function reboot(reason)
   local s, err
   while true do
      s, err = system.reboot(reason)
      if s and not err then break end
      if not s and err ~= "error 517 [hint: ipc broken]" then assert(nil) end
      sched.wait(1)
   end
end

function t: setup()
   assert(system.init())
end

function t: test_Reboot()
   reboot("reason 1")
   reboot("reason 2")
   reboot(nil)
   reboot("")
end

return t
