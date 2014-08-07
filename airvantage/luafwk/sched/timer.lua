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
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Timer module
--      supports one time timer timer.new(positive number)
--      supports periodic timer timer.new(negative number)
--      cron-compatible syntax (string conforming to cron syntax)
--      support simple timers
------------------------------------------------------------------------------


local os = os
local math = math
local tonumber = tonumber
local assert = assert
local table = table
local pairs = pairs
local next = next
local type = type
local _G=_G
local time = require'sched.timer.core'.time

module (...)

-------------------------------------------------------------------------------------
-- Common scheduling code
-------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------
-- an event is a table with:
--  * a :nextevent() method which returns the next due date, or nil if the timer is dead
--  * a 'nd' (Next Due) field holding the next due date. This field is computed by
--    the method above. It is used as a sorting key internally.
--  * any additional data needed by :nextevent()
--  * optional fields emitter and signal contain the two parameters of the signal
--    to emit when the timer elapses. emitter defaults to the event object itself,
--    signal defaults to "run".
-------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------
-- The list part is the list of dates at which at least one event is due.
-- The hash-part associates timestamps when at least one event is due, to the set of
-- all events due at this date (set => events are stored in keys, not values)
-------------------------------------------------------------------------------------
events = {}

-------------------------------------------------------------------------------------
-- These functions must be attached as method :nextevent() to timer objects,
-- and return the timer's next due date.
-------------------------------------------------------------------------------------
local function stimer_nextevent (timer) return nil end

-------------------------------------------------------------------------------------
-- Take a timer, reference it properly in `events` table
-------------------------------------------------------------------------------------
local function addevent(timer)
    local nd = timer.nd

    if events[nd] then
        events[nd][timer] = true
    else
        local n = #events+1
        for i = 1, n-1 do
            if events[i] > nd then n = i break end
        end
        table.insert(events, n, nd)
        events[nd] = { [timer] = true }

        if update_first_timer and n==1 then
            update_first_timer()
        end -- On some targets, the timer must be rearmed when it changes
    end
end

function addtimer(timer)
    timer.nd = timer:nextevent()
    if not timer.nd then return end -- this event is not to be rescheduled

    return addevent(timer)
end

-------------------------------------------------------------------------------------
-- Take a timer, dereference it properly in `events` table
-------------------------------------------------------------------------------------
function removetimer(timer)
    local entries = events[timer.nd]
    if not entries or not entries[timer] then return nil, "not a registered timer object" end
    entries[timer] = nil
    if not next(entries) then -- when there's no more entry for this date, clean up the list
        -- TODO: the date itself could be removed from the array-part of `events`.
        events[timer.nd] = nil
        if update_first_timer then update_first_timer() end -- On some targets, the timer must be rearmed when it changes
    end
    timer.nd = nil
    return "ok"
end

-------------------------------------------------------------------------------------
-- Signal all elapsed timer events.
-- This must be called by the scheduler every time a due date elapses.
-------------------------------------------------------------------------------------
function step()
    local ev1 = events[1]
    if not ev1 then return end -- if no timer is set just return and prevent further processing

    local now = time()
    while ev1 and now >= ev1 do
        local entries = events[ev1]
        if entries then
            for timer, _ in pairs(entries) do
                local ev = timer.event
                -- trig the timer. If the trigger is a hook, call it, otherwise signal a timer event
                if type(ev) == 'function' then ev(timer)
                else _G.sched.signal(timer.emitter or timer, ev or 'run') end
                addtimer(timer) -- reschedule when necessary
            end
            events[ev1] = nil
        end
        table.remove(events, 1)
        ev1 = events[1]

    end
    if update_first_timer then update_first_timer() end
end

-------------------------------------------------------------------------------------
-- Simple timer API used by the scheduler; cause a signal ('timer', '@<date>') after
-- the delay has elapsed.
-- Return the name of the event that will be sent at expiration, and the timer
-- itself, which can be used e.g. to cancel the event before it occurs.
-------------------------------------------------------------------------------------
function set(t, em, ev)
    assert(t>=0, "parameter must be a positive number")
    local nd = time() + t
    em = em or 'timer'
    ev = ev or "@"..nd
    local timer = { nextevent=stimer_nextevent, nd=nd, emitter=em, event=ev }
    addevent(timer)
    return ev, timer
end

-------------------------------------------------------------------------------------
-- Simple timer API used by the scheduler;
-- returns the next expiration date
-------------------------------------------------------------------------------------
function nextevent()
    return events[1]
end

return _M