-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Romain Perier      for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local checks = require 'checks'
local system = require 'agent.system'
local persist = require 'persist'

local function ResetToFactoryDefault(asset, params, command, ticket)
    checks("?", "?table", "?", "?")

    local restart = params and (params['restart'] or params[1])

    --ResetToFactoryDefault is done with a list a directories to remove
    --Removing the whole LUA_AF_RW_PATH directory is not possible for
    --default installation where LUA_AF_RW_PATH and LUA_AF_RO_PATH are the same
    --folder, so it would end up removing RA binaries too.

    --directories to delete
    local directories={
        --Lua Fwk:
        --persist file are not deleted but reset.

        --Agent modules:
        updatedir   =(LUA_AF_RW_PATH or "./").."update/",
        appcondir   =(LUA_AF_RW_PATH or lfs.currentdir().."/").."apps/",
        --config is using persist dir
        treemgrdir= (LUA_AF_RW_PATH or "./").."persist/treemgr"
        --security credentials are *not* cleared
    }

    persist.table.emptyall()  -- reset all persist files

    local st, output, status
    for k,v in pairs(directories) do
        st, output = require "utils.system".pexec("rm -rf "..v)
        if st ~= 0 then
            log("DEVMAN", "WARNING", "ResetToFactoryDefault: operation failed for=%s", tostring(v))
            return nil, 'ResetToFactoryDefault command failed'
        end
    end

    log("DEVMAN", "INFO", "Agent settings have been reset to factory")
    if (type(restart) == "boolean" and restart) or (type(restart) == "number" and restart) then
        local delay = (type(restart) == "number" and restart >= 6 and restart) or 6
        if ticket and ticket ~= 0 then
            require'racon'.acknowledge(ticket, 0, "ResetToFactoryDefault command succeeded", "on_boot", true) --ack to server after rebooting device
            status = "async"
        else status = "ok" end

        --this also depends on agent integration:
        -- if it is started using AppmonDaemon, making the agent exit with code !=0 may be enough
        -- (however applications installed in ApplicationContainer are likely to be still running)
        -- anyway, rebooting the whole device operating system is recommended.
        log("DEVMAN", "INFO", "Requesting device to be restarted in "..tostring(delay).." seconds to complete settings reset")
        system.reboot(delay, "request from server")
    else
        --if user doesn't want to reboot device, the ack is left to tree handler
        status = "ok"
    end

    return status
end

return ResetToFactoryDefault



