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
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched    = require "sched"
local config   = require "agent.config"
local server   = require "agent.srvcon"
local log      = require "log"
local asscon   = require "agent.asscon"
local treemgr  = require "agent.treemgr"
local niltoken = require "niltoken"
local upath    = require 'utils.path'
local utable   = require 'utils.table'
local racon    = require 'racon'
local errnum   = require 'returncodes'.tonumber

require 'coxpcall'

local M = { asset=false; initialized=false }


-- Enable auto reconnect, when some acknowledgement are sent back.
-- if set to nil(false) no auto reconnect is done
-- when set to a number, wait this amount of time before doing a reconnection => allow to send several message in the same transmission
M.AUTORECONNECT = 1

--------------------------------------------------------------------------------------------------------------------
-- predefined commands
--------------------------------------------------------------------------------------------------------------------
local function tree_cmd_Connect()
    return server.connect()
end

local function tree_cmd_default(asset, data, path, ticket)
    checks('racon.asset', 'table', 'string')
    local status, err
    local err_msg = ""
    for key, val in pairs(data) do
        local fullpath = upath.concat(path, key)
        local prefix, cmdname = upath.split(fullpath, 1)
        assert (prefix=="commands")
        local f = require("agent.devman.cmds."..cmdname)
        status, err = f(asset, val, fullpath, ticket)
        if err then
            err_msg = upath.concat(err_msg, err)
            log('DEVMAN', 'ERROR', "Error while running command %s: %s", cmdname, err_msg)
        else
            log('DEVMAN', 'DEBUG', "Ran command %s successfully (status=%s)", cmdname, tostring(status))
        end
    end
    if err_msg~="" then return status, err_msg else return status end
end

-- Handle access to non-command tree nodes.
local function tree_default(asset, data, path)
    checks('racon.asset', 'table', 'string')
    if log.musttrace('DEVMAN', 'INFO') then
        log('DEVMAN', 'INFO', "Requested to write %s in the device tree under path %q",
            sprint(data), path)
    end
    local status, msg = treemgr.set(path, data)
    if status then return 0 else return errnum 'UNSPECIFIED_ERROR', msg end
end

--------------------------------------------------------------------------------------------------------------------
-- Add EMP commands: GetVariable, SetVariable, RegisterVariable, UnregisterVariable
--------------------------------------------------------------------------------------------------------------------
local function EMPGetVariable(assetid, payload)
    local prefix, lpath_list = unpack(payload)
    local v, l = treemgr.get(prefix ~= "" and prefix or nil, lpath_list)
    if not v and type(l) == "string" then return asscon.formaterr(l) end
    return errnum("OK"), { niltoken(v), l }
end

local function EMPSetVariable(assetid, payload)
    local path, value = unpack(payload)
    local res, err = treemgr.set(path, value)

     -- An error occured
    if not res and type(err) == "string" then return asscon.formaterr(err) end

    -- At least one callback returned an error, theses errors have been collected into a table.
    -- Returning this table.
    if not res and type(err) == "table" then return errnum("OK"), err end
    if res and res ~= "ok" then return errnum("OK"), res end -- no callbacks acknowledged the changes, returning "not handled"
    return errnum("OK")
end

local dt_id_idx = 0
local dt2tm = { } -- tm->id translation is kept in a closure passed to tm.register

local function EMPRegisterVariable(assetid, payload)
    local prefix, regvars, passivevars = unpack(payload)
    local dt_id, tm_id, err
    -- hook function need to be a closure to keep assetid and registerid
    -- needed in hook.
    local function hook(var_map)
       local res, err = asscon.sendcmd(assetid, "NotifyVariable", {dt_id, var_map})
       if not res then
            log("DEVMAN", "ERROR",
                "Trying to send NotifyVariable message failed [%s] to asset(%s), unregistering this registerid",
                err or "unknown error",
                tostring(assetid))
            treemgr.unregister(tm_id)
            dt2tm[dt_id] = nil
       end
       return res, err
    end

    -- ensure we don't give niltoken to treemgr
    tm_id, err = treemgr.register(prefix ~= "" and prefix or nil, regvars, hook, passivevars)
    if not tm_id then return asscon.formaterr(err) end
    dt_id = "DT_ID_"..dt_id_idx
    dt_id_idx = dt_id_idx+1
    dt2tm[dt_id]=tm_id
    return 0, dt_id
end

local function EMPUnregisterVariable(assetid, dt_id)
    if not dt_id then return asscon.formaterr("BAD_PARAMETER:no devicetree id provided to unregister") end
    local tm_id = dt2tm[dt_id]
    dt2tm[dt_id] = nil
    if not tm_id then return asscon.formaterr("no matching treemanager id found") end
    local res, err = treemgr.unregister(tm_id)
    if res then return 0
    else return asscon.formaterr(err) end
end

local function rest_get_handler(env)
   local node = env["suburl"] and env["suburl"]:gsub("/", ".") or ""
   local v, l = treemgr.get(nil, node)
   if not v and type(l) == "string" then
      return v, l
   end
   return { niltoken(v), l }
end

local function rest_set_handler(env)
   if not env["payload"] then
      return nil, "missing value"
   end
   local node = env["suburl"] and env["suburl"]:gsub("/", ".") or ""
   return treemgr.set(node, env["payload"])
end

function M.init(cfg)

    if M.initialized then return "already initialized" end

    -- create asset @sys
    assert(racon.init())
    M.asset = assert(racon.newasset ('@sys'))

    -- configure standard tree handlers
    M.asset.tree.commands.Connect   = tree_cmd_Connect
    M.asset.tree.commands.__default = tree_cmd_default
    M.asset.tree.__default = tree_default
    M.asset.tree.commands.ReadNode  = nil
    -- the "ReadNode" command will be handled by agent.devman.cmds.ReadNode,
    -- through commands.__default's lazy module loader.

    assert(M.asset :start())

    -- register EMP commands
    assert(asscon.registercmd("GetVariable", EMPGetVariable))
    assert(asscon.registercmd("SetVariable", EMPSetVariable))
    assert(asscon.registercmd("RegisterVariable", EMPRegisterVariable))
    assert(asscon.registercmd("UnregisterVariable", EMPUnregisterVariable))

    -- register rest commands
    if type(config.rest) == "table" and config.rest.activate == true  then
       local rest = require 'web.rest'
       rest.register("^devicetree", "GET", rest_get_handler)
       rest.register("^devicetree", "PUT", rest_set_handler)
    else
       rest_get_handler = nil
       rest_set_handler = nil
    end
    M.initialized = true

    return "ok"
end

return M
