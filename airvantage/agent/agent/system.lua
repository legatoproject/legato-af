-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--- System module provides utils to interaction with OS: run commands, reboot, etc.
-- @module system
--------------------------------------------------------------------------------

local sched = require 'sched'
local wrap  = require 'fdwrapper'
local io    = require 'io'

require 'print'

local print        = print
local p            = p
local assert       = assert
local getmetatable = getmetatable
local os           = os
local tonumber     = tonumber
local type         = type
local tostring     = tostring
local log          = require 'log'
module(...)

--------------------------------------------------------------------------------
--- Updates system time.
-- Internaly uses hwclock system call.
--@param time [os.date doc](http://www.lua.org/manual/5.1/manual.html#pdf-os.time).
--@return string representing the current date on success.
--@return nil followed by an error message otherwise.
--------------------------------------------------------------------------------
function settime(time)
   local cmd = os.date("date %m%d%H%M%Y.%S",time)
   local fd = popen(cmd)
   local read = fd:read("*l") --only one line is returned
   status = fd:close()

   if status ~= "0" then
      return nil,read
   end

   status = os.execute("hwclock -w -u")
   if status ~= 0 then
      return nil,"Date cannot be set to hardware clock"
   end

   return read
end

---Commands execution.
-- @section command

--- Power state management
-- @section sys

---Reboots the OperatingSystem
--@param delay number of seconds before rebooting.
--Default: 5 seconds.
--@return nothing
function reboot(delay, reason)
    log("ASSCON", "INFO", "Reboot requested, reason: %s", tostring(reason))
    delay = ((tonumber(delay) or 5) > 5) and tonumber(delay) or 5
    sched.signal("system", "stop", "reboot")
    sched.run(function() sched.wait(delay); os.execute("reboot") end)
end

---Shuts down the OperatingSystem
--@param delay number of seconds before shutting down the system.
--Default: 5 seconds.
--@return nothing
function shutdown(delay, reason)
    log("ASSCON", "INFO", "Shutdown requested, reason: %s", tostring(reason))
    delay = ((tonumber(delay) or 5) > 5) and tonumber(delay) or 5
    sched.signal("system", "stop", "shutdown")
    sched.run(function() sched.wait(delay)
    os.execute("halt") end)
end

---Stops the Agent
--@param delay number of seconds before stopping the Agent.
--Default: 1 second.
--@return nothing
function stop(delay)
    delay = ((tonumber(delay) or 1) > 1) and tonumber(delay) or 1
    sched.signal("system", "stop", "stop")
    sched.run(function() sched.wait(delay)
    os.exit() end)
end
