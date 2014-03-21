-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
--     David Francois     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require = require

require 'print'

local sched = require "sched"
require "pack"
local string = require "string"
local log = require "log"

local type = type
local setmetatable = setmetatable
local pairs = pairs
require "coxpcall"
local copcall = copcall
local base = _G
local assert = assert
local table = table
local sprint = sprint
local next = next
local print = print
local p = p
local tostring = tostring
local errnum = require 'returncodes'.tonumber

module (...)

-- Assets table: holds the association between asset names (given with
-- register command) and ids (the associated emp parser instance)
-- The table has a reverse entry (i.e assets[name] = id and assets[id] = name).
assets = { }

-- Commands table: holds the EMP commands that can be processed by the agent
local commands = { }

-- Allows other modules to register extra EMP commands.
-- The hook must be a function that takes (assetid, payload) as parameters and
-- returns a status number followed by an optional payload that will by sent back.
-- The status number is the status of the EMP reply (0 means OK).
function registercmd(name, hook)
    if commands[name] then
        return nil, "Command name is already used"
    end
    commands[name] = hook
    return "ok"
end

local function getassetids(id)
    if not assets[id] then
        return "Unknown asset id"
    elseif not next(assets[id]) then
        return "{Unregistered asset ["..tostring(id).."]}"
    else
        return sprint(assets[id])
    end
end

local function unregisterasset(id)
    if assets[id] then
        for k, assetname in pairs(assets[id]) do
            assets[assetname]=nil
        end
        assets[id] = nil
    end
end

local function EMPRegisterAsset(assetid, name)
    log("ASSCON", "DETAIL", "Registering [%s] as asset (%s)...", tostring(assetid), name)
    -- check name
    if name:find("%.") or name=="" then
        log("ASSCON", "ERROR", "Asset name %q is not respecting naming policy. Rejecting registration.", name)
        return errnum 'BAD_PARAMETER'
    end
    -- check name availability
    if assets[name] then
        log("ASSCON", "ERROR", "An asset was already registered with name %q ([%s]). Rejecting registration !", name, tostring(assets[name]))
        return errnum 'BAD_PARAMETER'
    end
    -- register new asset
    assets[name] = assetid
    table.insert(assets[assetid], name)
    log("ASSCON", "INFO", "Asset registered, name=%q, id=%s.", name, tostring(assetid))
    sched.signal("ASSCON", "AssetRegistration", name)
    return 0
end

local function EMPUnregisterAsset(assetid, name)
    log("ASSCON", "DETAIL", "Unregistering [%s] as asset (%s)...", tostring(assetid), name)
    if not assets[name] then
        log("ASSCON", "ERROR", "No asset registered with name %q. Rejecting unregistration !", name)
        return errnum 'BAD_PARAMETER'
    end
    unregisterasset(name)
    return 0
end


local function EMPReboot(assetid, reason)
    local asset = (#assets[assetid] ~= 0) and assets[assetid][1] or "Unregistered asset [" .. tostring(assetid) .. "]"
    local _reason = reason and (reason .. ", from Asset (" .. asset .. ")") or nil
    local system = require "agent.system"
    system.reboot(nil, _reason)
    return 0
end

function printassettable()
    for k, v in pairs(assets) do
        if type(k)=="string" then
            base.print(string.format("\tAsset: %s => emp(%s)", k, tostring(v)))
        else
            base.print(string.format("\temp(%s) => %s", tostring(k), sprint(v)))
        end
    end
end

-- Returns the EMP status code followed by the payload of the response (if any)
-- in case of error return nil followed by an error code
function sendcmd(assetid, cmd, payload)
    if not assets[assetid] then
        log("ASSCON", "ERROR", "Cannot send command (%s) to unregistered asset (%s).", tostring(cmd), tostring(assetid))
        return nil, "unknown assetid"
    end
    if type(assetid) == 'string' then assetid = assets[assetid] end
    if not assetid then
        log("ASSCON", "ERROR", "Cannot find EMP parser for asset (%s), the command (%s) is discarded.", tostring(assetid), tostring(cmd))
        return nil, "no emp parser for asset"
    end
    local s, err = assetid:send_emp_cmd_wait(cmd, payload)
    if s ~= 0 then
        log("ASSCON", "ERROR", "Asset (%s) executing (%s) reports error: '%s'", getassetids(assetid), tostring(cmd), err or "unknown")
    end
    return s, err
end


-- This function is called when a local asset makes a connection
-- Usually the connection is closed when the asset does not need to send or receive anymore data.
-- If the asset closes its connection it will not be able to receive any data anymore...
function connectionhandler(skt)
    local function cmdhdl(instance)
        return function(cmdname, payload)
            local cmd = commands[cmdname]
            if cmd then
                local s, c, p = copcall(cmd, instance, payload)
                if not s then
                    log("ASSCON", "ERROR", "Error while executing EMP command (%s) for asset (%s): '%s'.", tostring(cmdname), getassetids(instance), c)
                    return errnum 'BAD_PARAMETER', c
                else
                    return c, p
                end
            else
                log("ASSCON", "ERROR", "Asset (%s) sent an unsupported EMP command (%s)", getassetids(instance), tostring(cmdname))
                return errnum 'BAD_PARAMETER', "unsupported EMP command"
            end
        end
    end
    -- create and configure emp
    local emp = require "racon.empparser"
    local instance = emp.new(skt)
    instance.cmdhook  = cmdhdl(instance)
    log("ASSCON", "INFO", "Connection received from asset [%s] at '%s:%s'", tostring(instance), skt:getpeername())
    --save assetid in assets table (no registered name for now)
    assets[instance] = {}
    -- start emp
    local s, err = instance:run()
    -- emp stopped
    if not s and not err:match("close") then
        log("ASSCON", "ERROR", "Asset (%s), connection abrubtly closed: '%s'.", getassetids(instance), err)
    end
    log("ASSCON", "INFO", "Asset (%s), connection closed.", getassetids(instance))
    unregisterasset(instance)
    skt:close()
end

function init(cfg)
    -- register main EMP commands
    assert(registercmd("Register", EMPRegisterAsset))
    assert(registercmd("Unregister", EMPUnregisterAsset))
    assert(registercmd("Reboot", EMPReboot))
    -- install asset server
    if cfg.assetport then
        local socket = require "socket"
        assert(socket.bind(cfg.assetaddress or "localhost", cfg.assetport, connectionhandler))
    end
    return "ok"
end
