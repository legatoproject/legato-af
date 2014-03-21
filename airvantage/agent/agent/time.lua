-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local log    = require "log"
local sched  = require "sched"
local socket = require "socket"
local config = require "agent.config"
local math   = require "math"
local ntp    = require "ntp.core"
local timer  = require "timer"
local lock   = require "sched.lock"

local os       = os
local table    = table
local tonumber = tonumber
local type     = type
local assert   = assert
local tostring = tostring

--[[
 rfc 4330 is the base used to develop this file
 Simple Network Time Protocol (SNTP) Version 4 for IPv4, IPv6 and OSI

some excerpts:

Timestamp Name          ID   When Generated
------------------------------------------------------------
Originate Timestamp     T1   time request sent by client
Receive Timestamp       T2   time request received by server
Transmit Timestamp      T3   time reply sent by server
Destination Timestamp   T4   time reply received by client

The roundtrip delay d and system clock offset t are defined as:

d = (T4 - T1) - (T3 - T2)
t = ((T2 - T1) + (T3 - T4)) / 2

Read more: http://www.faqs.org/rfcs/rfc4330.html
--]]


module(...)


-- synchronize current time with the ntp server
local function local_synchronize()
    assert(config.time.ntpserver)
    lock.lock(_M)

    local skt,err = socket.udp()
    if not skt then
        log("TIME", "ERROR", "socket.udp reports %s", err or "unknown")
        lock.unlock(_M)
        return
    end
    local res, err = skt:setpeername(config.time.ntpserver, 123);
    if not res then
        log("TIME", "ERROR", "udp:setpeername(%s, 123) reports %s", config.time.ntpserver,  err or "unknown")
        skt:close()
        lock.unlock(_M)
        return
    end
    skt:settimeout(5)

    local best_delay, best_offset
    local status, offset, delay, org, rec

    for i=8,1,-1 do
        repeat
            --first call to buildntppacket will build first NTP packet
            --subsequent calls build response packet to previous correct received packet
            local packet1, err= ntp.buildntppacket(org, rec)
            if not packet1 then
                log("TIME","WARNING", "Error in ntp.buildntppacket: %s", err or "unknown error");
                break
            end

            local res, err = skt:send(packet1)
            if not res then
                log("TIME","WARNING", "Error in skt:send: %s", err or "unknown error")
                break
            end

            local packet2, err= skt:receive()
            if not packet2 then
                log("TIME","WARNING", "Error in skt:receive: %s", err)
                break
            end

            status, offset, delay, org, rec =  ntp.processpackets(packet1, packet2)
            if not status then
                log("TIME","WARNING", "Cannot process NTP packets, err=%s", offset or "unknown error")
            elseif status =="0" then
                log("TIME","DEBUG", "Correct NTP packet received")
                if  not best_delay or (ntp.getbestdelay(best_delay, delay) ==  delay) then
                    log("TIME","DEBUG", "New best NTP packet found")
                    best_delay = delay
                    best_offset = offset
                end
            elseif status =="1" then
                log("TIME","WARNING", "Time set directly to NTP server time (%s) (without using NTP algorithm because of 34 years limitation)", os.date())
                --here (status=2): offset is fixed number:  number of sec in 34 years
                sched.signal("TIME", "TIME_UPDATED", offset);
            elseif status =="2" then  log("TIME","WARNING", "NTP packet sanity checks failed: %s" , offset or "unknown sanity failure");
            end
        until true
    end

    --now update time with best offset
    -- best offset is the one computed with the lowest delay
    if not best_delay then
        log("TIME", "ERROR", "Cannot set time: No valid NTP packet received");
    else
        local status, set_offset = ntp.settime(best_offset)
        if status ~= "ok" then
            log("TIME","ERROR", "Cannot set time, error: %s", set_offset or "unknown error")
        else
            log("TIME", "INFO","Time successfully synchronized to %s, applied offset: %ss", os.date(), tostring(set_offset))
            sched.signal("TIME", "TIME_UPDATED", set_offset);
        end
    end
    skt:close()
    lock.unlock(_M)
end

function synchronize()
    sched.run(local_synchronize)
end

function init()
    sched.sighook("NETMAN", "CONNECTED", synchronize)
    if config.time and config.time.ntppolling and config.time.ntppolling ~= 0 and config.time.ntppolling ~= "0" then
        local period = "string" == type(config.time.ntppolling) and config.time.ntppolling or -(tonumber(config.time.ntppolling)*60)
        timer.new(period, local_synchronize)
    end
    return "ok"
end
