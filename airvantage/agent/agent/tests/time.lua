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

local time = require'agent.time'
local u = require 'unittest'
local config = require 'agent.config'
local os = require"os"
local system = require"agent.system"

local t=u.newtestsuite("time")

function t:setup()
    u.assert(config.time.activate)
    u.assert(config.time.ntpserver)
    u.assert(not config.time.ntppolling or config.time.ntppolling==0)
end



function t:test_time_01_on_demand()
    time.synchronize()
    local res = sched.wait("TIME", "TIME_UPDATED", 20)--30 sec timeout
    u.assert_equal("TIME_UPDATED", res, "cannot update time using synchronize, timeout")
end

function t:test_time_02_before_34years()
    if _OSVERSION:match("^Linux") then
        --timestamp of 01/01/1971 00:00:00
        local time_1971=31536000
        -- current_time is really correct time (depending on precision)
        -- because this test is done after t:test_time_01_on_demand
        local time_start=os.time()
        u.assert(system.settime(time_1971))
        u.assert_lt(time_1971+2, os.time())
        time.synchronize()
        --timeout may be broken by time update!
        u.assert_equal("TIME_UPDATED", sched.wait("TIME", "TIME_UPDATED", 10 ))
        u.assert_equal("TIME_UPDATED", sched.wait("TIME", "TIME_UPDATED", 10 ))
        -- should have received 2 time update notif
        -- 1st one: time update to get rid of 34years limitation
        -- 2nd one: accurate time update
        local time_end=os.time()
        local expected_time_min=time_start
        local expected_time_max=time_start+25
        u.assert_gte(expected_time_min,time_end  )
        u.assert_lte(expected_time_max,time_end  )
    else
        print("test_time_02_before_34years not possible on oatlua: date cannot be set before year 2000")
    end
end


function t:test_time_03_after_34years()

    --timestamp of 01/01/2046 00:00:00
    --this is possible only on 64 OS
    local time_2046
    if _OSVERSION:match("^Linux") then
        if _OSVERSION:match("^Linux.-x86_64.-") then
            --make this file compilable using oatlua compiler!!
            time_2046=tonumber("2398377600")
        else
            print("### WARNING this test is runnable only on x86_64 Linux system")
            return
        end
    else
        -- OAT case
        --'fake' time representing 01/01/2070 00:00:42
        --TO BE CHANGED when API enabling time setting to year<2000.
        time_2046=42
    end
    -- current_time is really correct time (depending on precision)
    -- because this test is done after t:test_time_01_on_demand
    local time_start=os.time()
    u.assert(system.settime(time_2046))
    u.assert_lt(time_2046+2, os.time())
    time.synchronize()
    --timeout may be broken by time update!
    u.assert_equal("TIME_UPDATED", sched.wait("TIME", "TIME_UPDATED", 10 ))
    u.assert_equal("TIME_UPDATED", sched.wait("TIME", "TIME_UPDATED", 10 ))
    -- should have received 2 time update notif
    -- 1st one: time update to get rid of 34years limitation
    -- 2nd one: accurate time update
    local time_end=os.time()
    local expected_time_min=time_start
    local expected_time_max=time_start+25
    u.assert_gte(expected_time_min,time_end  )
    u.assert_lte(expected_time_max,time_end  )
end



function t:teardown()

end
