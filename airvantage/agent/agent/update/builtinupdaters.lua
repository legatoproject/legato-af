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

--
-- This file provides the installation of components from an update package sending the update to the agent (@sys).
-- For now two types of update are accepted:
--  - @sys.appcon: manage (install/uninstall) an application in the ApplicationContainer module
--  - @sys.update: `install script`: run a simple Lua script embedded in the update package
--
-- Error codes returned here will be interpreted as update error codes: see update.status file, or update error codes doc
-- in Software_Update_Module.md doc.
--

local config    = require "agent.config"
local pathutils = require "utils.path"
local lfs       = require "lfs"
local niltoken  = require "niltoken"
local devman    = require "agent.devman"
local upderrnum = require "agent.update.status".tonumber
local devasset  = devman.asset

local M = {}

local installscript
local appconinstall

-- run internal installers
local function dispatch( name, version, path, parameters)
    local res
    --name is the path minus @sys, updatepath is the first part of the remaining path used to determine what type of update
    --is requested.
    local updatepath , _ = pathutils.split(name, 1)

    if not updatepath then
        log("UPDATE", "INFO", "Built-in update for package %s: Update path is not valid", tostring(name))
        return upderrnum 'INVALID_UPDATE_PATH'
    end
    --dispatch the update
    if updatepath=="update" then
        log("UPDATE", "INFO", "Built-in update: installscript is running for component %s, path=%s", tostring(name), tostring(path))
        res = installscript(name, version, path, parameters)
    elseif updatepath=="appcon" then
        log("UPDATE", "INFO", "Built-in update: appcon is running for component %s, path=%s", tostring(name), tostring(path))
        res = appconinstall(name, version, path, parameters)
    else
        log("UPDATE", "INFO", "Built-in update for package %s: Sub path is not valid", tostring(name))
        res = upderrnum 'INVALID_UPDATE_PATH'
    end

    return res
end

function installscript(name, version, path, parameters)
    if not path then
         log("UPDATE", "DETAIL", "Built-in update: running install script with no location given, immediate success")
    return upderrnum 'OK' end

    local scriptpath = path.."/install.lua"
    if not lfs.attributes(scriptpath) then
        log("UPDATE", "ERROR", "Built-in update: installscript for package %s: file install.lua does not exist", name)
        return upderrnum 'MISSING_INSTALL_SCRIPT'
    end
    local f, err = loadfile(scriptpath)
    if not f then
        log("UPDATE", "ERROR", "Built-in update: installscript for package %s: Cannot load install.lua script, err=%s",name,  err or "nil")
        return upderrnum 'CANNOT_LOAD_INSTALL_SCRIPT'
    end

    --parameters given to the install to ease its work, add other parameters in the param table, so that API is kept.
    local runtimeparams = {cwd = lfs.currentdir(),  script_dir = path, name = name, version = version, parameters = parameters}

    local res, err = copcall(f, runtimeparams)
    if not res then
        log("UPDATE", "ERROR", "Built-in update: installscript for package %s: script execution error, err=%s", name, err or "nil");
        return upderrnum 'CANNOT_RUN_INSTALL_SCRIPT'
    end

    return upderrnum 'OK'
end

function appconinstall(name, version, appdata, parameters)
    if not config.get('appcon.activate') then
        log("UPDATE", "ERROR", "Built-in update: ApplicationContainer updater cannot be run because ApplicationContainer is not activated!")
        return upderrnum 'APPLICATION_CONTAINER_NOT_RUNNING' end
    local appcon = require"agent.appcon"
    --name = appcon.some.thing, let's remove "appcon" from name
    local _, appname = pathutils.split(name, 1)
    local res, err
    if version then --filter extra parameters
        local purge = parameters and parameters.purge==true
        local autostart = parameters and parameters.autostart == true
        res, err = appcon.install(appname, appdata, autostart, purge)
    else
        res, err = appcon.uninstall(appname)
    end
    log("UPDATE", "INFO", "Built-in update: ApplicationContainer updater result: res=%s, err=%s", tostring(res), tostring(err))
    return upderrnum (res and 'OK' or 'APPLICATION_CONTAINER_ERROR')
end

M.dispatch = dispatch

return M;