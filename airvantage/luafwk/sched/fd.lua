-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

require 'pack'
require 'log'

local fdt = {
    wait_read  = { },
    read_func = { },
    wait_write = { },
    write_func = { },
    wait_except = { },
    except_func = { }
}
proc.fd = fdt

------------------------------------------------------------------------------
-- File descriptor handling functions go in this sub-module;
-- they're used, among others, to make luasocket sched-compatible.
------------------------------------------------------------------------------
sched.fd = { }

local monotonic_time = require 'sched.timer.core'.time
local math_min = math.min

------------------------------------------------------------------------------
-- Add a file descriptor to a watch list (wait_read, wait_write or wait_except)
------------------------------------------------------------------------------
local function add_watch(rw, fd, func)
  local t = fdt["wait_"..rw]
  if t[fd] then return nil, "file descriptor already registered" end
  local i = #t+1
  t[i] = fd
  t[fd] = i
  fdt[rw.."_func"][fd] = func
  return true
end

------------------------------------------------------------------------------
-- Remove a file descriptor from a watch list (wait_read, wait_write or wait_excep)
------------------------------------------------------------------------------
local function remove_watch(rw, fd)
  local t = fdt["wait_"..rw]
  if not t[fd] then return nil, "file descriptor unknown (not registered)" end
  local lasti = #t
  local i = t[fd]

  if lasti ~= i then -- swap fd that are at index i and lasti
    local last = t[lasti]
    t[i] = last
    t[last] = i
  end

  t[lasti] = nil
  t[fd] = nil
  fdt[rw.."_func"][fd] = nil

  return true
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.close(fd)
  remove_watch("read", fd) -- need to keep those remove_watch because some may have been added with when_fd function
  remove_watch("write", fd)
  remove_watch("except", fd)
  sched.signal(fd, "closed")
end

------------------------------------------------------------------------------
-- local helper function
-- Wait for a file descriptor to be readable / writable
-- Block until the readable/writable/exception condition is met or the timeout
-- expires.
-- Return true when readable/writable/have exception or nil followed by the 
-- error message which is "timeout" when the given timeout expired.
------------------------------------------------------------------------------
local function wait_fd(rw, fd, timelimit, timedelay)
  local stat, msg = add_watch(rw, fd)
  if not stat then return nil, msg end

  local timeout -- compute the first due date: either timeout per operation (timedelay) or global timeout (time limit)
  timelimit = timelimit and timelimit-monotonic_time()
  timeout = timedelay and (timelimit and math_min(timelimit, timedelay) or timedelay) or timelimit

  local event, msg = sched.wait(fd, {rw, "error", "closed", timeout})
  if event == rw then stat, msg = true, nil
  elseif event == "timeout" then stat, msg =  nil, "timeout"
  elseif event == "closed" then stat, msg =  nil, "closed"
  else stat, msg =  nil, string.format("Channel error: %s", msg or 'unknown')
  end

  remove_watch(rw, fd)
  return stat, msg
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.wait_readable(fd, timelimit, timedelay)
  return wait_fd("read", fd, timelimit, timedelay)
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.wait_writable(fd, timelimit, timedelay)
  return wait_fd("write", fd, timelimit, timedelay)
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.wait_exception(fd, timelimit, timedelay)
  return wait_fd("except", fd, timelimit, timedelay)
end


------------------------------------------------------------------------------
-- local helper function
-- Register func to be called whenever the file descriptor fd is
-- readable/writable/have exception. 
-- func is a function that can return "again" (or something equivalent to true)
-- in order to be called again if the file descriptor is 
-- readable/writable/have exception again.
-- If func return false or nil then the file descriptor is de-registered 
-- automatically.
-- Calling this function with func=nil de-register the given file descriptor
------------------------------------------------------------------------------
local function when_fd(rw, fd, func)
  if func then
    return add_watch(rw, fd, func)
  else
    return remove_watch(rw, fd)
  end
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.when_readable(fd, func)
  return when_fd("read", fd, func)
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.when_writable(fd, func)
  return when_fd("write", fd, func)
end

------------------------------------------------------------------------------
--
------------------------------------------------------------------------------
function sched.fd.when_exception(fd, func)
  return when_fd("except", fd, func)
end

------------------------------------------------------------------------------
-- Block on file descriptors until some are readable or some are writable or
-- some have exceptions.
--    see wait_read/wait_write/wait_except list
-- wait until timeout is reached.
------------------------------------------------------------------------------
local function notify_fd(rw, notified)
  local t = fdt[rw.."_func"]
  for _, fd in ipairs(notified) do
    if t[fd] then
      local r = t[fd](fd)
      if not r then remove_watch(rw, fd) end
    end
    sched.signal(fd, rw)
  end
end


------------------------------------------------------------------------------
-- the 'require' is performed inside `sched.fd.step`, then the select
-- function is memorized in `fd_select`, in order to avoid a
-- circular dependency.
------------------------------------------------------------------------------
local fd_select
function sched.fd.step(timeout)
  if not fd_select then local psignal = require 'sched.posixsignal'; fd_select = psignal.select; end -- executed once at first call only !

  local can_read, can_write, has_except, msg = fd_select(fdt.wait_read, fdt.wait_write, fdt.wait_except, timeout)

  if msg=='timeout' then return 'timeout' end

  notify_fd("read", can_read)
  notify_fd("write", can_write)
  notify_fd("except", has_except)
end

return sched.fd
