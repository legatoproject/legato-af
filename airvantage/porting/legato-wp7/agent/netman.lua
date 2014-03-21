local sched  = require 'sched'
local lock   = require 'sched.lock'
local nm     = require 'agent.platform.nm'
local tutils = require 'utils.table'
local log    = require 'log'
local timer  = require 'timer'

local M = {}

--some modules still access to netman API by global (relics of `module` usage)
--so we need to put netman API as global too.
global 'agent'; agent.netman = M


local requested = 0
local releasetm
local instance

function M.withnetwork(action, ...)
    local res, err
    lock.lock(M)
    --if no connection request in progress
    if requested == 0 and not releasetm then
        --request the network to be mounted and wait for the action to end with 30 sec timeout.
        log("NETMAN", "DEBUG", "Requesting")
        instance, err = nm.request(30)
        if not instance then
            log("NETMAN", "ERROR", "NM request failed: %s ", tostring(err) )
            lock.unlock(M)
            return nil, (err or "network not available")
        end
        sched.signal("NETMAN", "CONNECTED", "Default")
    end
    requested = requested + 1
    lock.unlock(M)

    --always cancel the existing timer to leave time to action to execute
    if releasetm then releasetm:cancel() end

    --capture all action results
    local r = { action(...) }

    requested = requested - 1

    --requested will be set to 0 by the action that finished last
    if requested == 0 then
        local function onrelease()
            log("NETMAN", "DEBUG", "Releasing")
            if not instance then releasetm = nil; log("NETMAN", "ERROR", "Releasing : internal error, no instance") end
            --We release network connection asynchronously (block set to false)
            -- to ensure that the dedicated coroutine executing this function will be collected ASAP.
            -- We could have set the call as synchronous to be able to send DISCONNECTED signal
            -- at the appropriate moment.
            --sched.signal("NETMAN", "DISCONNECTED")
            local res, err = nm.release(instance, false)
            if not res then
                 log("NETMAN", "ERROR", "Releasing network connection failed with [%s]", tostring(err))
            end
            --enable to request new connection whatever the release result was
            releasetm = nil
        end
        -- One shot 60sec timer to release network connection.
        -- It gives a chance to another network related action to run quickly without
        -- re-mounting interface. E.G. When a connection to the server happens, it is very likely a second connection
        -- will happen soon after (to send an ack or ReadNode data etc).
        -- Also it helps handling some network mgmt issue: the socket is kept open in M3DA connection, and M3DA module can have trouble detecting
        -- the interface was down and up again, and that it needs to re-init the socket.
        releasetm = timer.new(60, onrelease)
    end
    
    -- return all action results
    return unpack(r)
end


function M.init()
    return "ok"
end

return M
