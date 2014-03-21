-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot for Sierra Wireless - initial API and implementation
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Timer module that supports one-time timer, periodic timer and Cron-compatible syntax.
-- @module timer
--

local sched = require 'sched'
local checks = require 'checks'
local sched_timer = require 'sched.timer'
local os_time = os.time
local os_date = os.date
local monotonic_time = require'sched.timer.core'.time
local math = math
local tonumber = tonumber
local tostring = tostring
local error = error
local unpack = unpack
local table = table
local _G=_G
local setmetatable = setmetatable
local type = type
local ipairs=ipairs
local assert = assert

require 'utils.table' -- needed for table.pack. To be removed when we switch to Lua5.2


module(...)

-- The cron random jitter needs to give different sequenaces on different devices.
-- WARNING: this might cause nasty surprizes to people who fiddle by themselves
-- this the random seed.
math.randomseed(os_time())

local TIMER_MT = { __type='timer' }; TIMER_MT.__index=TIMER_MT

-- ---------------------------------------------------------------------------
-- CRON name->number conversions
local conv = {
        sun = 0, mon = 1, tue = 2, wed = 3, thu = 4, fri = 5, sat = 6,
        jan = 1, feb = 2, mar = 3, apr = 4,  may = 5,  jun = 6,
        jul = 7, aug = 8, sep = 9, oct = 10, nov = 11, dec = 12 }

-- ---------------------------------------------------------------------------
-- CRON alias->config conversions
local cron_aliases = {
    ["@yearly"]   = "0 0 1 1 *",
    ["@annually"] = "0 0 1 1 *",
    ["@monthly"]  = "0 0 1 * *",
    ["@weekly"]   = "0 0 * * 0",
    ["@daily"]    = "0 0 * * *",
    ["@hourly"]   = "0 * * * *" }

-- ----------------------------------------------------------------------------
-- Converts a CRON element, already extracted from the string, into a number
-- representing the next occurrence. This number's unit is the same as the
-- string's, hence can be in minutes, hours etc.
--
-- @function [parent=#timer] nextval
-- @param p the string extract to convert
-- @param curr value of current time (in the same units as `p`)
-- @param period period of `p`'s units, e.g. 60 for minutes, 24 for hours etc.
-- @param minval minimal acceptable value for the result
-- @return time of next occurrence of the event, in the same units as `p`.
--

local function nextval(p, curr, period, minval)

    local next
    if p:match(",") then
        next = math.huge
        -- we have a list of values, we need to test the list one by one
        for e in p:gmatch("[^,]+") do
            local l = nextval(e, curr, period, minval)
            if l < next then next = l end
        end
        return next
    else
        local step = tonumber(p:match("/(%d+)")) or 1 -- get the step value (number behind the '/' char)
        if step>period/2 then error("step too big in "..p) end

        if p:match("%-") then
            -- we have a range
            local min, max = p:match("(%d+)%-(%d+)")
            min = tonumber(min)
            max = tonumber(max)
            if not min or not max then error("error in range pattern: "..p) end
            max = math.min(period, max)
            min = math.min(math.max(minval, min), max)
            max = min + math.floor((max-min)/step)*step
            if curr < min then next = min
            elseif curr > max then next = period + min
            else next = min + math.floor((curr-min+step-1)/step)*step end
        elseif p:match("%*") then
            if step > 1 then next = math.floor((curr+step-1)/step)*step -- round up the value so it corresponds to the step
            else next = curr end
        else
            local v = tonumber(p) or conv[p:lower()]
            if not v or v>period or v<minval then error("error in pattern "..p) end
            next = curr <= v and v or v + period
        end
    end

    return next
end

-- ----------------------------------------------------------------------------
-- Retrieves the number of days in the month including a given date.
--
-- @function [parent=#timer] nbofdaysinmonth
-- @param date in `os.time` format included in the considered month
-- @return number of days in the month: 28,29,30,31
--

local function nbofdaysinmonth(now)
    local cd = os_date("*t", now)
    cd.day = 0
    cd.month = cd.month+1
    return tonumber(os_date("%d", os_time(cd)))
end

-- ----------------------------------------------------------------------------
-- Converts a CRON string into the next date at which the corresponding event
-- shall be triggered. To be stored as a timer object's `nextevent` field.
--
-- @function [parent=#timer] cron_nextevent
-- @param timer the CRON string
-- @return next occurrence's date, as an `os.time()` number.
--

local function cron_nextevent(timer)
    local entry = timer.cron
    local now = os_time()
    -- minutes, hours, day of month, month, day of week, random jitter
    local mm, hh, jj, MMM, JJJ , rnd = entry:match("^(%S+) (%S+) (%S+) (%S+) (%S+) ?(%S*)")
    local d = os_date("*t", now)
    if not mm then error("error in entry syntax: "..tostring(entry)) end

    local nmin = nextval(mm, d.min+1, 60, 0)

    if nmin >= 60 then nmin = nmin-60; d.hour = d.hour+1 end
    d.min = nmin

    local nh = nextval(hh, d.hour, 24, 0)
    if nh >= 24 then nh = nh-24; d.day = d.day+1; d.wday = d.wday+1 end
    d.hour = nh

    local nd
    if JJJ ~= "*" then
        local wday = d.wday-1 -- 0 is sunday in cron syntax
        nd = nextval(JJJ, wday, 7, 0) - (wday) + d.day
    end

    local daysinmonth = nbofdaysinmonth(now)
    if not nd or jj ~= "*" then
        local nmd = nextval(jj, d.day, daysinmonth, 1)
        if nd then nd = math.min(nmd, nd) else nd = nmd end
    end

    if nd > daysinmonth then nd = nd-daysinmonth; d.month = d.month+1 end
    d.day = nd

    if #rnd>0 and not timer.jitter then
        local jitter_boundary = tonumber(rnd) or error "Invalid cron jitter"
        timer.jitter = math.random(0, jitter_boundary)
    end

    local nm = nextval(MMM, d.month, 12, 1)
    d.month = nm -- date sytem will auto increment year if month > 12
    d.yday = nil
    d.wday = nil
    d.sec = 0
    d = monotonic_time() + (os_time(d) - now)

    if timer.jitter then d=d+timer.jitter end

    return d
end

-- ----------------------------------------------------------------------------
-- Computes next due date for periodic timers.
-- To be stored as a timer object's `nextevent` field.
local function periodic_nextevent(timer)
    local n, period = timer.nd, timer.period
    local now = monotonic_time()
    n = (n or now) + period
    if n<now then  -- make sure the next expiration date is in the future
        n = (1 + math.floor(now/period)) * period
    end
    return n
end

-- ----------------------------------------------------------------------------
-- Computes next due date for 1-shot timers
--  To be stored as a timer object's `nextevent` field.
local function oneshot_nextevent(timer)
   return not timer.nd and monotonic_time()+timer.delay or nil
end


-- ----------------------------------------------------------------------------
-- Converts a table into a standard cron string.
-- @param t the table
-- @return the corresponding string
local function table2cron(t)
    local conv = {
        { "minute", "minutes", 1 },
        { "hour", "hours", 2},
        { "dayofmonth", "day", 3 },
        { "month", "months", 4 },
        { "dayofweek", 5},
        { "jitter", 6} }
    local cron = { "*", "*", "*", "*", "*" }
    for i, names in ipairs(conv) do
        for _, name in ipairs(names) do
            local v=t[name]
            if v then
                if type(v)=='table' then v=table.concat(v,',') end
                cron[i]=v; break
            end
        end
    end
    return table.concat(cron, " ")
end

-------------------------------------------------------------------------------------
-- Setups and returns a new timer object.
-- This function can be called directly instead of @{timer.once}, @{timer.periodic}
-- and @{timer.cron}.
--
-- The event will cause a `signal([returned timer], 'run')` whenever its due
-- date(s) is/are reached. The date(s) is/are determined by the `expiry` argument:
--
-- * If `expiry` is a positive number, causes a 1-shot event after that delay,
--   in seconds, has elapsed.
--
-- * If `expiry` is a negative number, causes a periodic event whose period is
--   the (positive) opposite of `expiry`, in seconds.
--
-- * If `expiry` is a string or a table, causes events at the dates described
--   under the POSIX CRON format. Cf. @{timer.cron} for a detailed description
--   of the CRON format.
--assert
-- @function [parent=#timer] new
-- @param expiry string or number specifying the timer's due date(s).
--   See explanations above.
-- @param hook an optional function which will be called whenever the timer is due.
--   The hook is called in a new thread (thus blocking functions are allowed in it).
-- @param varargs optional additional parameters, which will be passed to the `hook`
--   function when called.
-- @return created timer on success.
-- @return `nil` followed by an error message otherwise.
--

function new(expiry, hook, ...)
    checks("string|number|table", "?function")

    local timer
    local t = tonumber(expiry)
    if not t then
        if type(expiry)=='table' then expiry=table2cron(expiry) end
        timer = { nextevent=cron_nextevent, cron=cron_aliases[expiry] or expiry }
    elseif t>=0 then
        timer = { nextevent=oneshot_nextevent,  delay=t }
    else
        timer = { nextevent=periodic_nextevent, period=-t }
    end

    if hook then
        local p = table.pack(...)
        timer.event = function() sched.run(hook, unpack(p, 1, p.n)) end
    end

    sched_timer.addtimer(timer)
    if not timer.nd then return nil, "failed to create timer" end
    return setmetatable(timer, TIMER_MT)
end

--------------------------------------------------------------------------------
-- Creates a new timer object, which will elapse every `period` seconds.
-- If a hook function is provided, it is run whenever the timer elapses.
--
-- @function [parent=#timer] periodic
-- @param period the duration between two timer triggerrings, in seconds
-- @param hook optional function, run everytime the timer elapses
-- @param varargs optional parameters for `hook`
--

function periodic(period, hook, ...)
    checks('number', '?function')
    assert(period>0)
    return new(-period, hook, ...)
end

--------------------------------------------------------------------------------
-- Creates a new timer object, which will elapse after `delay` seconds.
-- If a hook function is provided, it is run when the timer elapses.
--
-- @function [parent=#timer] once
-- @param delay the duration before the timer elapses, in seconds
-- @param hook optional function, run when the timer elapses
-- @param varargs optional parameters for `hook`
--
function once(delay, hook, ...)
    checks('number', '?function')
    assert(delay>0)
    return new(delay, hook, ...)
end

--------------------------------------------------------------------------------
-- Creates a new timer object, which will elapse at every date described by
-- the `cron` parameter.
--
-- If a hook function is provided, it is run whenever the timer elapses.
-- This is an alternative to @{#timer.new}.
--
-- CRON specification strings
-- --------------------------
-- Cron entries are strings of 5 elements, separated by single spaces.
--
--          .---------------- minute (0 - 59)
--          |  .------------- hour (0 - 23)
--          |  |  .---------- day of month (1 - 31)
--          |  |  |  .------- month (1 - 12) OR jan,feb,mar,apr ...
--          |  |  |  |  .---- day of week (0 - 7) (Sunday=0 or 7)  OR sun,mon,tue,wed,thu,fri,sat
--          |  |  |  |  |  .- optional random jitter
--          |  |  |  |  |  |
--          *  *  *  *  *  *
--
-- The corresponding event is triggered every minute at most, if and only if
-- every field matches the current date. Asterisks (`"*"`) can be used as
-- placeholderst in unused fields. For instance, `"30 8 1 * * *"` describes an
-- event which takes place at 8:30AM every first day of the month.
--
-- Beside numbers, some operators are allowed in fields:
--
-- - As mentionned, the asterisk (`'*'`) placeholder represents all possible
--   values for a field. For example, an asterisk in the hour time field
--   would be equivalent to 'every hour' (subject to matching other specified
--   fields).
--
-- - The comma (`','`) is a binary operator which specifies a list of possible
--   values, e.g. `"1,3,4,7,8"` (there must be no space around commas)
--
-- - The dash (`'-'`) is a binary operator which specifies a range of values.
--   For instance, `"1-6"` is equivalent to `"1,2,3,4,5,6"`
--
-- - The slash (`'/'`) is a binary operator, called "step", which allows to skip
--   a given number of values. For instance, `"*/3"` in the hour field is
--   equivalent to `"0,3,6,9,12,15,18,21"`.
--
-- For instance, `"0 * * * *"` denotes an event at every hour (whenever the
-- number of minutes reaches 0), but `"0 */6 * * *"` denotes only the hours
-- divisible by 6 (i.e. 0:00, 6:00, 12:00 and 18:00). Beware that the formal
-- meaning of the `'/'` operator is "whenever the modulo is 0".
-- As a consequence, `"*/61"` in the minutes slot will be triggered when the
-- number of minutes reaches 0, because 0/61==0.
--
-- Cron also accepts some aliases for common periodicities. `"@hourly"`,
-- "@daily"`, "@weekly"`, "@monthly"` and "@annually"` represent the
-- corresponding expected periodic events.
--
-- CRON jitter extension
-- ---------------------
-- `timer.cron()` supports an addition to the cron standard: if a sixth number
-- element is appended to the string, this number is taken as a random jitter,
-- in seconds. For instance, if a jitter of 120 is given, a random shifting
-- between 0 and 120 is chosen when the timer is created; every triggerring of
-- the event will be time-shifted by this amount of time.
--
-- The jitter doesn't change during the lifetime of a given timer:
-- if a minutely test `"* * * * * 59"` is triggered at 12:03:28, it will be
-- triggered next at 12:04:28, 12:05:28 etc.: jitters don't modify the time
-- which elapses between the triggerings of a given timer.
--
-- Jitter allows to spread the triggering dates of resource-intensive
-- operations. For instance, if a communication to the M2M operating portal is
-- scheduled at midnight with `"0 0 * * *"` on a large fleet of devices,
-- congestion might ensue on the communication networks and/or on the servers.
-- By adding a 60*60*4 = 14400 jitter, communications will be uniformly
-- spread between 0:00AM and 04:00AM: `"0 0 * * * 14400"`.
--
-- CRON specified with a table
-- ---------------------------
-- Cron specifications can also be entered as tables, with keys `minute, hour,
-- dayofmonth, dayofweek, jitter` respectively; missing keys are treated as
-- `"*"` (except for `jitter`, which is treated as missing); lists of values
-- are passed as tables; steps and ranges are not supported in any special ways,
-- but they can be passed as strings, e.g. `{ hour='9-12,14-17', minute=0 }`.
--
-- @function [parent=#timer] cron
-- @param cron specification of the cron dates, as a cron string or a table
--   (see @{#timer.new})
-- @param hook optional function, run everytime the timer elapses
-- @param varargs optional parameters for `hook`
-- @return a timer object
--
function cron(cron, hook, ...)
    checks('string|table', '?function')
    return new(cron, hook, ...)
end

-------------------------------------------------------------------------------------
-- Cancels a running timer.
-- No signal will be triggered with this timer object.
-- A canceled timer can be rearmed using the @{rearm} function.
--
-- @function [parent=#timer] cancel
-- @param timer as returned by @{#timer.new} function.
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--
function cancel(timer)
    checks('timer')
    return sched_timer.removetimer(timer)
end

-------------------------------------------------------------------------------------
-- Rearms a canceled or expired timer.
-- Same as new function. A signal will be emitted on timer expiration.
-- Rearming a timer that have not expired, reset it to its inital value.
--
-- @function [parent=#timer] rearm
-- @param timer as returned by @{#timer.new} function.
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--
function rearm(timer)
    checks('timer')
    if timer.nd then timer:cancel() end
    sched_timer.addtimer(timer)
    if not timer.nd then return nil, "unable to rearm timer" end
    return "ok"
end

TIMER_MT.cancel = cancel
TIMER_MT.rearm  = rearm

local latencyjobs = { }

-------------------------------------------------------------------------------------
-- Calls a function within a maximum delay, while trying to avoid multiple calls.
--
-- In some cases, a task must be performed within the next `n` seconds; if the task
-- is performed by calling function `f`, then a call to `timer.latencyexec(f, n)`
-- will guarantee that. If `f` hasn't been run after the `n` seconds delay expired
-- (neither by this thread nor any other one), then the execution of `f` is triggered.
-- However, if `f` has already been triggered by another call to `timer.latencyexec`
-- before the delay expired, then it isn't triggered a second time.
--
-- When a function can treat data by batches and is idempotent (think for instance
-- flushing buffered data to a server), calling it with a delay through `latencyexec`
-- leaves a chance to group multiple invocations into a single one, thus presumably
-- saving resources.
--
-- @function [parent=#timer] latencyExec
-- @param func function to run.
-- @param latency optional number. Time to wait before running the function (in seconds),
-- if set to O then the function is run asynchronously but as soon as possible.
-- if set to nil, then the function is run synchronously.
-- @return "ok" on success.
-- @return nil followed by an error message otherwise.
--
-- @usage
--
-- timer.latencyExec examples
--
-- -- t1: `f` must have run within the next 10s
-- timer.latencyexec(f, 10)
-- -- t2: `f` must have run within the next 5s. This supersedes t1
-- timer.latencyexec(f, 5)
-- -- after 3s elapsed, `f` must run within the next 5s-3s=2s
-- sched.wait(3)
-- -- t3: `f` must have run within the next 1s. This supersedes the 2s left in t2
-- t3 = timer.latencyexec(f, 1)
-- -- `f` will run in the middle of this 2s delay, due to t3
-- sched.wait(2)
-- -- This has been scheduled after t3 elapsed, so `f` will be run again in 1s at most.
-- t4 = timer.latencyexec(f, 1)
--
function latencyExec(func, latency)
    checks('function', '?number')

    latency = tonumber(latency)
    if latency and latency < 0 then return nil, "latency must be positive integer" end

    local function dofunc()
        latencyjobs[func] = nil
        return func()
    end
    -- execute `func` synchronously only if giving a nil latency
    if latency == nil then
        if latencyjobs[func] then
            sched.kill(latencyjobs[func].hook)
        end
        return dofunc()
    end
    -- calculate exec time
    local next = monotonic_time() + latency
    -- if not already scheduled, schedule
    local entry = latencyjobs[func]
    if latency == 0 then
        if entry then sched.kill(entry.hook) end
        sched.run(dofunc)
    elseif not entry then
        latencyjobs[func] = {hook = sched.sigrunonce("timer", latency, dofunc), date = next}
    -- if already schedule and new execution is early
    elseif entry.date > (next + 1) then
        sched.kill(entry.hook)
        latencyjobs[func] = {hook = sched.sigrunonce("timer", latency, dofunc), date = next}
    end
    return "ok"
end

latencyexec=latencyExec

