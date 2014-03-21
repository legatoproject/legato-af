-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local core = require "agent.platform.core"
local sched = require "sched"

local M = {}


-- Lua non-blocking (sched aware) API to request network connection to Legato framework.
-- Can be synchronous or not by using `blocking` parameter:
-- - `blocking` set to nil or false will make the function to return
--   without waiting the result from Legato le_data callback.
-- - `blocking` set to true value will make the function to return
--    after receiving the result from Legato le_data callback
-- - if `blocking` can be evaluated as a number, then the function
--    will return after receiving the result from Legato le_data callback
--    with a timeout in seconds  set to `blocking` number value.
function M.request(blocking)
    local instance, err = core.connectionRequest()
    if instance and blocking then
        local ev, s, p = sched.wait("netman-legato", {"connected", tonumber(blocking)})
        if ev == "timeout" then
            --in case of error, release the connection asynchronously (block = false)
            instance:release(false)
            return nil, "Timeout expired, unable to request connection"
        end
    end
    return instance, err
end

-- Lua non-blocking (sched aware) API to release network connection to Legato framework.
-- Can be synchronous or not by using `blocking` parameter:
-- - `blocking` set to nil or false will make the function to return
--   without waiting the result from Legato le_data callback.
-- - `blocking` set to true value will make the function to return
--    after receiving the result from Legato le_data callback.
-- - if `blocking` can be evaluated as a number, then  the function
--    will return after receiving the result from Legato le_data callback
--    with a timeout in seconds set to `blocking` number value.
function M.release(instance, blocking)
    local res, err = instance:release()
    if res == "ok" and blocking then
        local ev, s, p = sched.wait("netman-legato", {"disconnected", tonumber(blocking)})
        if ev == "timeout" then return nil, "Timeout expired, unable to release connection" end
    end
    return res, err
end

function M.init()
    sched.sigonce("Agent", "InitDone", function(...) core.connectionRegisterHandler() end)
    return "ok"
end

return M
