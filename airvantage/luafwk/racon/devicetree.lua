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
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--
--This module provides read/write access to system parameters and settings, as
-- well as the ability to receive notifications when they change.
--
-- System parameters and settings are addressed based on a predefined tree
--(the "Device Tree") that organizes them based on functionality. This tree
-- will continue to be developed and expanded as the Application Framework evolves.
--
-- Description / documentation of the tree is available in the "Device Param
-- Access" technical article.
--
-- @module devicetree
--

local common      = require 'racon.common'
local niltoken    = require 'niltoken'

local M = {
    initialized=false; -- has M.init() been run?
    notifyvarid = { }; -- associate a registration ID returned by treemgr
                       -- through EMP, with the callback that should be run
                       -- upon notification (by treemgr through EMP).
    sem_value = 1
}


local uniqueuserid = 1

local function sem_wait()
   while M.sem_value <= 0 do
      sched.wait("dt.semaphore", "unlock")
   end
   M.sem_value = M.sem_value - 1
end

local function sem_post()
   M.sem_value = M.sem_value + 1
   if M.sem_value >= 1 then
      sched.signal("dt.semaphore", "unlock")
   end
end

--------------------------------------------------------------------------------
-- Sets a variable value into the variable tree.
-- **Example**
--
-- To activate the monitoring, you could send:
--
--     devicetree.set ("config.monitoring.activate", true).
--
-- @function [parent=#devicetree] set
-- @param #string path defining the path of the variable to set.
-- @param value is the value of the variable. It can also be a table of key/value
--   pairs, allowing to set several variables in a single operation.
--   it will actually set a whole sub tree (several variables at once)).
-- @return `"ok"` on success.
-- @return `nil` + error message otherwise.

function M.set(path, value)
    checks("string", "?")
    if not M.initialized then error "Module not initialized" end
    local s, b
    sem_wait()
    s, b = common.sendcmd("SetVariable", { path, value })
    sem_post()

    -- No callbacks acknowledged the changes, returning "not handled"
    if s and type(b) == "string" then return b end
    -- At least one callback returned an error, theses errors have been collected into a table.
    -- Returning this table as errors informations.
    if s and type(b) == "table"  then return nil, b end
    -- An error occured, normal case
    if not s and type(b) == "string" then return nil, b end
    return s
end

--------------------------------------------------------------------------------
-- Retrieves a variable's value from the device tree.
--
-- The retrieval is not recursive: asking for a path prefix will _not_ return
-- every key/value pairs sharing this prefix. Instead, it will return `nil`
-- followed by a list of the absolute paths to the prefix's direct children.
--
-- For instance, if the branch `foo.bar` contains `{ x=1, y={z1=2, z2=3}}`,
-- a `get("foo.bar")` will return `nil, { "foo.bar.x", "foo.bar.y" }`. No value
-- is returned, and the children of `y`, which are the grand-children of
-- `foo.bar`, are not enumerated.
--
-- Several variables can also be retrieved in a single request, by passing a
-- list of paths rather than a single path string. In this case, a record
-- of path/value pairs is returned. If some of the paths are non-leaf nodes,
-- a second list, of all direct children of all non-leaf paths, is also
-- returned.
--
-- @usage `devicetree.get("system.sw_info.fw_ver")` may return `"4.2.5"`.
-- @usage `devicetree.get("system.sw_info")` may return
--   `nil, {"system.sw_info.fw_ver", "system.sw_info.boot_ver"}`.
-- @usage `devicetree.get({"system.sw_info", "system.sw_info.boot_ver"})`
-- may return `{["system.sw_info.boot_ver"]="4.2.5"}, {"system.sw_info.fw_ver", "system.sw_info.boot_ver"}`.
--
-- @function [parent=#devicetree] get
-- @param #string path is a string, or list of strings, defining the path of
--   the variable to retrieve.
-- @return the value associated with the `path` leaf node.
-- @return `nil` + list of direct children paths when `path` is a non-leaf node.
-- @return `nil` + error string otherwise.
--

function M.get(...)
    if not M.initialized then error "Module not initialized" end
    local prefix, lpath_list
    if select("#", ...) == 2 and type(select(1, ...)) == 'string' then
        prefix, lpath_list = ...
    else
        prefix, lpath_list = nil, ...
    end
    assert(type(prefix) == "string" or type(prefix) == "nil")
    assert(type(lpath_list) == "string" or type(lpath_list) == "table")

    local s, b = common.sendcmd("GetVariable", { prefix or "", lpath_list })
    if s~="ok" then return nil, (b or "UNSPECIFIED_ERROR") end
    if b[1] == niltoken then return nil, b[2] end
    return b[1]
end

--------------------------------------------------------------------------------
-- Registers to receive a notification when one or several variables change.
--
-- The callback is triggered everytime one or some of the variables listed in
-- `regvars` changes. It receives as parameter a table of
-- variable-name / variable-value pairs; these variables are all the variables
-- listed in `regvars` which have changed, plus every variables listed in
-- `passivevars`, whether  their values changed or not.
--
-- Please note that the callback can be called with table param
-- containing @{niltoken#niltoken} value(s) to indicate variable(s) deletion.
--
-- Variables listed in `regvars` can be either FQVN names,
-- or a path which denotes every individual variable below this path.
--
-- Variables listed in `passivevars` must be path to variables: non-leaf paths
-- will be silently ignored.
--
-- @function [parent=#devicetree] register
-- @param regvars list of variables which must be monitored for change.
-- @param callback the function triggered everytime a `regvars` variable changes.
--   The callback is called with a table containing all the changes, paths as
--   keys and values as values.
-- @param passivevars optional variables to always pass to `callback`,
--   whether they changed or not.
-- @return a registration id, to be passed to @{devicetree.unregister}
--   in order to unsubscribe.
-- @return `nil` + error message in case of error.
--

-- register(regvars, callback, passivevars)
-- register(regvars, prefix, shortpaths, callback, passivevars)
function M.register(...)
    if not M.initialized then error "Module not initialized" end
    local prefix, regvars, callback, passivevars
    if select("#", ...) >= 3 and type(select(3, ...)) == 'function' then
        prefix, regvars, callback, passivevars = ...
    else
        prefix, regvars, callback, passivevars = nil, ...
    end
    if type(regvars)=='string' then regvars={regvars} end
    assert(type(regvars) == "table" and type(callback) == "function")
    assert(type(passivevars) == "table" or type(passivevars) == "nil")

    sem_wait()
    --possible niltoken value for passivevars is managed in devman.
    -- Don't send a nil prefix directly, it is not handled correctly by EMP (arguments are shift to the left) 
    local status, id = common.sendcmd("RegisterVariable", { prefix or "", regvars, passivevars})
    if status~="ok" then
       sem_post()
       return nil, (id or "UNSPECIFIED_ERROR")
    end

    -- Generate a new unique user ID
    local userId = uniqueuserid
    uniqueuserid = uniqueuserid+1
    
    -- A user identifier is required to avoid gaps in the list (when the callback is unregistered)
    table.insert(M.notifyvarid, {userId=userId, regId=id, cb=callback, prefix=prefix, regvars=regvars, passivevars=passivevars})
    sem_post()
    return userId
end

--------------------------------------------------------------------------------
-- Cancels registration to receive a notification when a variable changes.
--
-- @function [parent=#devicetree] unregister
-- @param userid is the id returned by previous @{devicetree.register}
--        call to be cancelled.
-- @return "ok" on success.
-- @return nil + error string otherwise.
--

function M.unregister(userid)
    checks("string|number")
    if not M.initialized then error "Module not initialized" end

    sem_wait()
    for k, v in pairs(M.notifyvarid) do
       if v.userId == userid then
          local s, msg = common.sendcmd("UnregisterVariable", v.regId)
          if s == "ok" then
             table.remove(M.notifyvarid, k)
          end
          sem_post()
          return s, msg
       end
    end
    sem_post()
    return nil, "BAD_PARAMETER"
end


-- React to a notification by treemgr sent through racon.
local function emp_handler_NotifyVariable(payload)
    local id, varmap = unpack(payload) -- do not filter niltoken here ?
    for path, val in pairs(varmap) do
       if val then varmap[path]=niltoken(val) end
    end
    for _, v in pairs(M.notifyvarid) do
       if v.regId == id then
          local status, res, err = copcall(v.cb, varmap)
          if not status then return 1, res end
          if not res and not err then return 0, "unhandled" end
          if not res and type(err) == "string" then return 1, err end
          return 0
       end
    end
    return 1, "cannot find callback"
end

--MUST NOT BLOCK !!!!
local function emp_ipc_broken_handler()

   --block sem: services using EMP are blocked until sem is released
   -- This avoids data races between reregistration and other threads which send EMP commands
   M.sem_value=0
   sched.run(function()
        for _, reg in pairs(M.notifyvarid) do
           local status, id = common.sendcmd("RegisterVariable", { reg.prefix or "", reg.regvars, reg.passivevars})
           if status ~= "ok" then
              log("DT", "WARNING", "Failed to register back callback notification err=%s", tostring(status))
           else
              reg.regId = id
           end
        end
        sem_post()
         end
        )
end
--------------------------------------------------------------------------------
-- Initializes the module.
-- @function [parent=#devicetree] init
-- @return non-`nil` upon success;
-- @return `nil` + error message upon failure.
--

function M.init()
    if M.initialized then return "already initialized" end
    local a, b = common.init()
    if not a then return a, b end
    common.emphandlers.NotifyVariable = emp_handler_NotifyVariable
    common.emp_ipc_brk_handlers.DeviceTreeIpcBrkHandler = emp_ipc_broken_handler
    M.initialized=true
    return M
end

return M