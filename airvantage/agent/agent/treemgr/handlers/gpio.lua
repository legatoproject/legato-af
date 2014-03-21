-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
--GPIO device tree handler.
--This handler highly relies on [GPIO module](http://download.eclipse.org/mihini/api/lua/gpio.html),
--please read its documentation before.
--
-- Especially, have a look on:
--
-- * Linux kernel GPIO userspace API: [Kernel GPIO doc](https://www.kernel.org/doc/Documentation/gpio.txt),
-- section "Sysfs Interface for Userspace
-- * user right access to /sys/class/gpio for agent process
-- * automatic and required GPIO configuration for each GPIO action (read/write/register)
--
-- *Device Tree Mapping*:
--
--The root path of GPIO is defined in gpio.map file,
--in this documentation let's suppose it's set to `system.gpio`
--Here is the device mapping provided by this tree handler:
--
--| Parameter Path                    | R/W | Data Type | Data Range                  | Description |
--| system.gpio.id                    | RW  | Integer   | 0-1                         | The current value of the GPIO pin, 1 high, 0 low.
--                                                                                      This path can be registered by embedded application to be notified for changes. |
--| system.gpio.settings.id.direction | RW  | String    | "in", "out"                 | The direction of the GPIO; input or output
--| system.gpio.settings.id.edge      | RW  | String    | "none", "rising", "falling" | This setting describes when embedded application will be notified for changes:
--                                                                                      never, on rising edge (from inactive to active) or falling edge (from active to inactive).|
--| system.gpio.settings.id.activelow | RW  | String    | "1", "0"                    | This setting describes how to interpret the GPIO:
--                                                                                      Is high voltage value (e.g.: 3.3V or 5V) to be interpreted as "active"/"on" (the default behavior), or as "inactive"/"off".
--                                                                                      This setting also modifies the way "rising" / "falling" edge are interpreted. |
--| system.gpio.available             | R   | String    | N/A                         | This is the list of exiting GPIOs on the device, not necessary activated/enabled GPIO. |
--
--
--Notes:
--
-- * When listing system.gpio and system.gpio.settings, only GPIO already enabled are returned.
-- * When reading/writing/configuring a GPIO that was never used before on the system (since last reboot),
--    the GPIO is automatically configured with the most appropriate values.
-- * When reading/writing/configuring a GPIO that was already used on the system (since last reboot),
--    the GPIO is left with its current configuration even though it can't fulfill the current operation.
--
--
--
--Usage:
-- -- Read / write GPIOs
-- local dt = require "devicetree"
-- dt.init()
--
-- dt.set("system.gpio.settings.10.direction", "out")
-- dt.set("system.gpio.settings.20.direction", "in")
--
-- local value = dt.get("system.gpio.20")
-- dt.set("system.gpio.10", 1)
--
--Usage:
-- -- Get notified of every change on GPIO 4 and copy GPIO 4 value on GPIO 7.
--
-- local dt = require "devicetree"
-- dt.init()
-- dt.set("system.gpio.settings.4.direction", "in")
-- dt.set("system.gpio.settings.4.edge", "both")
-- dt.set("system.gpio.settings.7.direction", "out")
--
-- local function hook(notif)
--   dt.set("system.gpio.7", notif["system.gpio.4"])
-- end
--
-- dt.register("system.gpio.4", hook)
--
--



local log      = require"log"
local gpio     = require"gpio"
local upath    = require"utils.path"
local treemgr  = require"agent.treemgr"
local niltoken = require"niltoken"

--the name used to load this tree handler, to be used for notification
local handlername = ...

local logmod= "DT-GPIO"

local M = { }

function M :get (path)
    local res, err
    if path == nil or path == "" then --list root gpio path
        res, err = gpio.enabledlist() -- list of enabled GPIOs.
        if not res or type(res)~= "table" then return nil, err or "error while loading GPIO list"
        else
            local res2 = {}
            for k,v in pairs(res) do res2[v]=true end
            --no need to set real value, available GPIOs will be return when explicitly requested
            res2.available = true
            res2.settings = true
            return nil, res2
        end
    elseif path == "available" then
        local res, err = gpio.availablelist()
        if not res then return nil, err or "Can't get availablelist"
            --return it as a string so that no subvalue will be requested.
        else return sprint(res) end
    elseif path:match("settings") then --reading GPIO configuration
        local _, subpath = upath.split(path,1)
        if not subpath or subpath=="" then
            res, err = gpio.enabledlist() -- list of available GPIOs.
            if not res or type(res)~= "table" then return nil, err or "error while loading GPIO list"
            else
                local res2 = {}
                for k,v in pairs(res) do res2[v]=true end
                --no need to set real value, available GPIOs will be return when explicitly requested
                return nil, res2
            end
        else
            local id, setting = upath.split(subpath,1)
            id = tonumber(id)
            if not id then return nil, "invalid GPIO id" end
            --listing config parameters for each GPIO ( it's supposed to be the same for each GPIO)
            if not setting or setting == "" then return nil, { direction = true, edge = true, activelow=true} end
            if setting == "direction" or setting == "edge" or setting == "activelow" then
                -- those subpaths are only for configuration purpose
                res, err = gpio.getconfig(id)
                if not res or type(res)~= "table" then return nil, err or "failed to get GPIO config" end
                if not res[setting] then
                    return nil, string.format("failed to get GPIO config field %s", tostring(setting))
                else
                    return res[setting]
                end
            else
                return nil, string.format("invalid GPIO parameter: %s", tostring(setting))
            end
        end
    elseif tonumber(path) then
        -- reading GPIO value
        local id = tonumber(path)
        return gpio.read(id)
    else
        return nil, string.format("invalid GPIO parameter: %s", tostring(path))
    end
end

function M :set(hmap)
    local res, err, changesset = nil, nil, {}
    for hpath, val in pairs(hmap) do --set can be done on several values in the same call.
        val = niltoken(val) --filter niltoken value (no specific behavior for niltoken/nil)
        if not val then return nil, string.format("invalid nil value for path %s", tostring(hpath)) end
        local root, subpath = upath.split(hpath,1)
        local subpath1, subpath2 = upath.split(subpath,1)
        if root == "settings" then --configuring GPIO, path is like: `settings.id.direction` etc
            local id = tonumber(subpath1)
            if not id then return nil, "invalid GPIO id" end
            if subpath2 == "direction" or subpath2 == "edge" or subpath2 == "activelow" then
                val = tostring(val)
                if not val then return nil, string.format("invalid value %s for parameter %s", val, subpath2) end
                res, err = gpio.configure(id, { [subpath2]=val})
                if not res then return nil, string.format("error when configuring GPIO %d: err: %s", id, tostring(err)) end
                --TODO: improvement: bufferize cfg actions?
            else
                return nil, string.format("invalid GPIO parameter: %s", tostring(subpath2))
            end
        elseif tonumber(root) and (subpath1=="" or not subpath1) then --writing GPIO value, the path is like `id`
            local id = tonumber(root)
            val = tostring(val)
            if val ~= "0" and val~="1" then return nil, string.format("invalid value to write %s", val) end
            res,err = gpio.write(id, val)
            if not res then return nil, string.format("error when writing to %d: err:%s", id, err) end
            table.insert(changesset, id)
        else
            return nil, string.format("invalid GPIO parameter: %s", tostring(hpath))
        end
    end
    return changesset
end


--keep track of registered GPIOs
--to prevent useless multi registrations.
local registered = {}


local function gpio_hook(id, val)
    id = tonumber(id)
    if not id then
        log(logmod, "WARNING", "Receive notification from gpio module with invalid id ")
    elseif not registered[id] then
        log(logmod, "WARNING", "Receive notification from gpio module for 'unknown' id.")
    else
        local path = string.format("%d", id)
        treemgr.notify(handlername, { [path]=val});
    end
end


function M :register(hpath)
    local id = tonumber(hpath)
    --can only register on `id` path
    if not id then return nil, "only GPIO value can be monitored" end
    if not registered[id] then
        registered[id] = true
        return gpio.register(id, gpio_hook)
    else
        return "ok"
    end
end


function M :unregister(hpath)
    local id = tonumber(hpath)
    --can only register on `id` path
    if not id then return nil, "only GPIO value can be monitored" end
    if registered[id] then
        registered[id] = nil
        return gpio.register(id, nil)
    else
        return "ok"
    end
end

return M
