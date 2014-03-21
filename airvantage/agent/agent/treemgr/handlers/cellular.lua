-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched    = require 'sched'
local at       = require 'at'
local timer    = require 'timer'
local notify   = require 'agent.treemgr'.notify

-- Extract strings, as captured by `regex`, from the 1st AT command response line
-- @return the captures in a list, or nil + error msg
local function at2strings(at_command, regex)
    local response, msg = at (at_command)
    if not response then return nill, msg end
    local captures = { response[1] :match (regex) }
    return captures
end

-- Extract numbers, as captured by `regex`, from the 1st AT command response line
-- @return the captured numbers in a list, or nil + error msg
local function at2numbers(at_command, regex)
    local captures, msg = at2strings(at_command, regex)
    for i, s in ipairs(captures) do captures[i] = tonumber(s) end
    return captures
end

-- timer object which will trigger the next signal level polling
local poll_timer
-- number of seconds between two signal level pollings
local poll_frequency = 10
-- values at last polling
local poll_last_rssi, poll_last_ber

-- Check signal levels, notify changes if applicable.
-- This function is intended to be triggered upon timer `poll_timer`.
local function poll_signal()
    local hmap = { }
    local rssi, ber = unpack(at2numbers("AT+CSQ", "+CSQ: (%d+), (%d+)"))
    if rssi~=poll_last_rssi then
        hmap['signal.rssi'], poll_last_rssi = rssi, rssi
    end
    if ber~=poll_last_ber then
        hmap['signal.ber'], poll_last_ber = ber, ber
    end
    if next(hmap) then notify(hname, hmap) end
end

-- Change polling frequency, notify change if appropriate
local function poll_set_frequency(f)
    if f~=poll_frequency then
        pollf_requency=f
        if poll_timer then timer.cancel(poll_timer); poll_timer=nil end
        if f~=0 then poll_timer = timer.new(-f, poll_signal) end
        notify(hname, {['signal.pollfrequency']=f})
    end
end

-- helper to define getters
local function lift(f, at_command, regex)
    return { get = function() f(at_command, regex) end }
end

local t = {
    imei            = lift(at2strings, 'AT+CGSN', "%d+"),
    imsi            = lift(at2strings, 'AT+CIMI', "%d+"),
    ['signal.rssi'] = lift(at2numbers, 'AT+CSQ', "%+CSQ: (%d+), %d+"),
    ['signal.ber']  = lift(at2numbers, 'AT+CSQ', "%+CSQ: %d+, (%d+)"),
    ['signal.pollfrequency'] = {
        get = function() return poll_frequency end,
        set = poll_set_frequency }
}

return require 'agent.treemgr.handler.functable' (hname, t, false)
