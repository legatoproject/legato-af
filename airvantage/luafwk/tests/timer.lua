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

local timer = require 'timer'
local u = require 'unittest'
local t = u.newtestsuite("timer")
local monotonic_time = require 'sched.timer.core'.time
require 'print'

-- Fix random seed so that the test are reproducible!
math.randomseed(4242)

local function random_timeout()
   local r = math.random() * 4 + 0.5
   return math.floor(r * 100) / 100
end

local function check_timer(timeout, starttime, endtime)
   local delta = 0.02
   u.assert_gte((timeout-delta), (endtime - starttime))
   u.assert_lte((timeout+delta), (endtime - starttime))
end

function t:test_01_oneshot_timer()
   local timeout = random_timeout()
   local starttime = monotonic_time()

   -- Testing oneshot timer with hook
   local _timer = timer.new(timeout,  function()
                                        sched.signal("timertest", "hook_invoked")
                                      end)
   u.assert_not_nil(_timer)
   local ev = sched.wait("timertest", { "hook_invoked", 5 })
   local endtime = monotonic_time()
   u.assert_equal("hook_invoked", ev)
   check_timer(timeout, starttime, endtime)
   u.assert_nil(_timer.nd)

   -- Testing oneshot timer with signal
   timeout = random_timeout()
   starttime = monotonic_time()
   _timer = timer.new(timeout)

   u.assert_not_nil(_timer)
   ev = sched.wait(_timer, { "run", 5 })
   local endtime = monotonic_time()
   u.assert_equal("run", ev)
   check_timer(timeout, starttime, endtime)
   u.assert_nil(_timer.nd)
end

function t:test_02_periodic_timer()

   -- Testing periodic timer with hook
   local timeout = random_timeout()
   local starttime = monotonic_time()
   local _timer = timer.new(-timeout, function()
				   sched.signal("timertest", "hook_invoked")
			        end
			   )
   u.assert_not_nil(_timer)
   local ev = sched.wait("timertest", { "hook_invoked", 5 })
   u.assert_equal("hook_invoked", ev)
   check_timer(timeout, starttime, monotonic_time())

   local saved_nd = _timer.nd
   for i=1,60 do
      local nev = _timer:nextevent()
      check_timer(timeout, _timer.nd, nev)
      _timer.nd = nev
   end
   _timer.nd = saved_nd
   u.assert(timer.cancel(_timer))

   -- Testing periodic timer with signal
   timeout = random_timeout()
   starttime = monotonic_time()
   _timer = timer.new(-timeout)

   u.assert_not_nil(_timer)
   ev = sched.wait(_timer, { "run", 5 })
   local endtime = monotonic_time()
   u.assert_equal("run", ev)
   check_timer(timeout, starttime, endtime)

   saved_nd = _timer.nd
   for i=1,60 do
      local nev = _timer:nextevent()
      check_timer(timeout, _timer.nd, nev)
      _timer.nd = nev
   end
   _timer.nd = saved_nd
   u.assert(timer.cancel(_timer))
end

function t:test_03_cron_timer_table()

   local t = os.date("*t")
   t.hour = (t.min + 1) >= 60 and t.hour + 1 or t.hour
   t.min = (t.min + 1) % 60

   local _timer = timer.new(t)

   u.assert_not_nil(_timer)
   for i=1,60 do
      local time = monotonic_time()
      local nev = _timer:nextevent()
      local nt = os.date("*t", os.time() + (nev - time))
      u.assert_equal(t.min, nt.min)
      t = nt
   end
   u.assert(timer.cancel(_timer))

   t = os.date("*t")
   t.hour = (t.hour + 1) % 24
   _timer = timer.new(t)

   u.assert_not_nil(_timer)
   for i=1,24 do
      local time = monotonic_time()
      local nev = _timer:nextevent()
      local nt = os.date("*t", os.time() + (nev - time))
      u.assert_equal(t.hour, nt.hour)
      t = nt
   end
   u.assert(timer.cancel(_timer))
end

function t:test_04_cron_timer_posix_syntax()

   local t = os.date("*t")
   t.min = (t.min + 1) % 60
   local _timer = timer.new("*/1 * * * *")

   u.assert_not_nil(_timer)
    for i=1,60 do
      local time = monotonic_time()
      local nev = _timer:nextevent()
      local nt = os.date("*t", os.time() + (nev - time))
      u.assert_equal(t.min, nt.min)
      t = nt
   end
   u.assert(timer.cancel(_timer))

   t = os.date("*t")
   t.hour = (t.hour + 1) % 24
   local _timer = timer.new("@hourly")

   u.assert_not_nil(_timer)
    for i=1,24 do
      local time = monotonic_time()
      local nev = _timer:nextevent()
      local nt = os.date("*t", os.time() + (nev - time))
      u.assert_equal(t.hour, nt.hour)
      t = nt
   end
   u.assert(timer.cancel(_timer))
end

function t:test_05_cancel_timer()
   local _timer = timer.new(-0.1)

   u.assert(timer.cancel(_timer))
   u.assert_nil(timer.cancel(_timer))
   u.assert_nil(timer.cancel(_timer))
   _timer.nd = 666
   u.assert_nil(timer.cancel(_timer))
end

function t:test_06_rearm_timer()
   local _timer = timer.new(2)
   
   u.assert(timer.rearm(_timer))
   u.assert(timer.cancel(_timer))
   u.assert(timer.rearm(_timer))

   timer.cancel(_timer)
end
