-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--- Agent port to Legato : porting and wrappers functions

local log   = require"agent.platform.log"
local event = require"agent.platform.event"
local nm    = require"agent.platform.nm"
local core  = require"agent.platform.core"
local sched = require "sched"

local M = { }

function M.init()
    log.init()
    event.init()
    nm.init()
    -- Signal to Legato Supervisor that agent init is done.
    sched.sigonce("Agent", "InitDone", core.syncWithSupervisor)
    return 'ok'
end

function M.getupdateplatformcomponent()
        return {
            Agent    = _MIHINI_AGENT_RELEASE,
        }
    end

function M.getdeviceid()
    local treemgr = require "agent.treemgr"
    local deviceId = treemgr.get("system.cellular.hw_info.imei")
    local log = require "log"
    log("LEGATO", "INFO", "getdeviceid: deviceId set [%s]", deviceId or "unknown");
    return deviceId or "012345678901234"
end

return M

