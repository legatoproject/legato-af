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
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require"sched"
local lfs = require"lfs"
local os = require "os"

local M = {}
local APPS_ROOT = "/opt/legato/apps"
local SANDBOX_ROOT = "/tmp/legato/sandboxes"

local function isAppInstalled(id)
    local installed = false
    for _, app in pairs(M.list()) do
        if app == id then
            installed = true
        end
    end
    return installed
end


local function basename(path)
    return string.gsub(path, "(.*/)(.*)", "%2")
end

local function appCmd(id, action)
    if not isAppInstalled(id) then return nil, "Application is not installed" end
    local res, err = os.execute("app ".. action .. " ".. id)
    if not res then return nil, err end
    return "ok" 
end

--- Start an application if it is runnable.
-- If the application is not runnable, or the application is already started, it has no effect.
-- @param id the string used as unique id when application was installed
-- @return "ok" string in case of success, nil+error message otherwise
function M.start(id)
   return appCmd(id, "start")
end

--- Stop an application if it is runnable.
-- If the application is not runnable, or the application is already stopped, it has no effect.
-- @param id the string used as unique id when application was installed
-- @return "ok" string in case of success, nil+error message otherwise
function M.stop(id)
   return appCmd(id, "stop")
end

--- Install an application in appcon.
-- The application can be runnable or not, it is decided dynamically checking for run file.
-- @param id string to use a unique id for the app.
-- Note that if the application id is already used, the application will be overwritten/reinstalled
-- by the last call to install.
-- @param data_path string representing absolute path to application folder
-- @param autostart boolean value to set autostart mode of the application
-- @param purge boolean value to purge application runtime folder before reinstallation
-- @return "ok" string in case of success, nil+error message otherwise
function M.install(id, data_path, autostart, purge)
    log("APPCON", "DETAIL", "id=%s, data_path=%s", tostring(id), tostring(data_path)) 
    if not data_path or "file" ~= lfs.attributes(data_path, "mode") then return nil, "Invalid data" end

    local res, err = os.execute("app install " .. id .. " < " .. data_path)
    if tostring(res) ~= "0" then return nil, string.format("app install failed with res=[%s], err[%s]", tostring(res), tostring(err)) end
    return "ok"
end

--- Get installed applications list.
-- @return table listing the installed application, nil+error message otherwise
function M.list()
    local list = {}
    for app in lfs.dir(APPS_ROOT) do
        if app ~= "." and app ~= ".." then
            table.insert(list, app)
        end
    end
    return list
end

--- Return the status of an application.
-- if the application is runnable, then it returns the status sent by status command of appmon_daemon (see appmon_daemon doc)
-- else only returns "Not runnable"
-- @param id the string used as unique id when application was installed
-- @return status string in case of success, nil+error message otherwise
function M.status(id)
    if not isAppInstalled(id) then return nil, "Application is not installed" end
    for app in lfs.dir(SANDBOX_ROOT) do
        if app == id then
            return "running"
        end
    end
    return "not running"
end

--- Uninstall an application.
-- First: stop the application if running, then remove permanently the whole folder where the application was installed.
-- @param id the string used as unique id when application was installed
-- @return "ok" string in case of success, nil+error message otherwise
function M.uninstall(id)
    return appCmd(id, "remove")
end

--- Configure an application.
-- Set autostart parameter of an runnable application.
-- if the application is not runnable, or the application is already stopped, it has no effect.
-- @param id the string used as unique id when application was installed
-- @param autostart boolean value to set autostart mode of the application
-- @return "ok" string in case of success, nil+error message otherwise
function M.configure(id, autostart)
    -- In Legato user applications are automatically started after the sys components.
    -- autostart is always assumed to be "true" and is not configurable
    return nil, "Unsupported for Legato porting"
end

--- get application info from daemon.
--used in treemgr handler to expose appcon data.
--@param id the string used as unique id when application was installed
--@return table containing application attributes when operation is successful,
--nil+error message otherwise
function M.attributes(id)
    -- In Legato this kind of informations are not available
    return nil, "Unsupported for Legato porting"
end

--- Init function to be called by the agent's initializer module.
-- Loads the installed application list, inits each application and starts all applications
--  with autostart parameter set to true.
-- @return "ok" string in case of success, nil+error of message otherwise
function M.init()
    return "ok"
end

return M
