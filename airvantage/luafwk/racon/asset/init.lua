-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
--     Minh Giang         for Sierra Wireless - initial API and implementation
--     Romain Perier      for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--
-- Racon Application Services object used to send data to the M2M
-- Application Services server. Instances of this object should be created
-- through @{racon#(racon).newAsset}.
--
-- This module relies on an internal background service known as the Mihini
-- Agent, which is responsible for queuing data, managing the flush timers
-- and sending the data to the remote M2M server. Many of the APIs in
-- this module relay the data to the Agent; the Agent then manages
-- the data as described.
--
-- For information regarding supported policies, see the @{racon#racon} module
-- documentation.
--
--Two methods are supported for sending data to the M2M servers:
--
-- 1. `pushData`:  this is a simple API for managing how to send data,
--    this is the recommended method for most use cases.
-- 2. Tables (via `newTable`): this allows for more advanced control of the
--    transfer of data; it is an experimental API.
--
-- Moreover, methods `setUpdateHook` and `sendUpdateResult` are provided to let application handle
-- software update requests from the M2M servers in a custom way.
--
-- @module racon.asset
--

local common       = require 'racon.common'
local utils_table  = require 'utils.table'
local checks       = require 'checks'
local log          = require 'log'
local upath        = require 'utils.path'
local asset_tree   = require 'racon.asset.tree'
local niltoken     = require "niltoken"
local errnum       = require 'returncodes'.tonumber

local M = { initialized=false; sem_value = 1}

M.assets = { }

local MT_ASSET = { __type  = 'racon.asset' }; MT_ASSET.__index = MT_ASSET

local function sem_wait()
   while M.sem_value <= 0 do
      sched.wait("av.semaphore", "unlock")
   end
   M.sem_value = M.sem_value - 1
end

local function sem_post()
   M.sem_value = M.sem_value + 1
   if M.sem_value >= 1 then
      sched.signal("av.semaphore", "unlock")
   end
end

-- ------------------------------------------------------------------------------
-- Creates and returns a new @{#asset} instance.
--
-- The newly created @{#asset} is not started when returned, it can therefore
-- neither send nor receive messages at this point.
--
-- This intermediate, unstarted set allows to configure message handlers before
-- any message is actually transferred to the #asset.
--
-- See @{racon.asset.start} to start the newly created instance.
--
-- @function [parent=#racon] newAsset
-- @param id string defining the assetid identifying the instance of this new asset.
-- @return @{#asset} instance on success.
-- @return `nil` followed by an error message otherwise.
--
function M.newasset(id)
    checks('string')
    assert(id ~= "")
    if not M.initialized then error "Module not initialized" end

    if M.assets[id] then return nil, "asset id in use" end

    local instance = {
        id   = id,
        tree = { commands = { } } }

    -- configure default datatree
    instance.tree.commands.__default = function() return nil, "unknown command" end
    instance.tree.commands.ReadNode  = asset_tree.ReadNode
    setmetatable(instance, MT_ASSET)

    local a, b = common.init()
    if not a then return a, b end
    a, b = asset_tree.init()
    if not a then return a, b end

    M.assets[id] = instance

    return instance
end

--------------------------------------------------------------------------------
-- Starts a newly created @{#asset}.
--
-- Allows the @{#asset} instance to send and receive messages to/from the servers.
--
-- An @{#asset} instance which has been started, then experiences an unrecoverable
-- error or an explicit call to @{asset:close} emits a `"closed"` event
-- (cf. @{sched#signal}).
--
-- @function [parent=#asset] start
-- @param self
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

function MT_ASSET :start()
    checks("racon.asset")
    if not M.initialized then error "Module not initialized" end

    if self.registered then return nil, "already started" end
    -- send the register command
    self.registered = true
    local r, m = common.sendcmd("Register", self.id)
    self.registered = r and true or false
    return r, m
end

--------------------------------------------------------------------------------
-- Closes an @{#asset} instance.
--
-- Once this destructor method has been called, no more message can be sent
-- nor received by the instance, and the resources it reserved can be reclaimed,
-- either immediately or through the garbage collection cycle.
--
-- @function [parent=#asset] close
-- @param self
-- @return `"ok"` on success.
--

function MT_ASSET :close()
    checks("racon.asset")
    if not self.registered then return nil, "asset not started" end
    sem_wait()
    common.sendcmd('Unregister', self.id)
    M.assets[self.id]=nil
    sem_post()
    return "ok"
end


--------------------------------------------------------------------------------
-- Pushes some unstructured data to the agent.
--
-- Those data are not necessarily moved forward from the agent to the server
-- immediately: agent-to-server data transfers are managed through policies,
-- as described in the _Racon technical article_.
--
-- This API is optimized for ease of use: it will internally try to reformat
-- data in the most sensible, server-compatible way. Applications requiring a
-- tight control over how data are structured, buffered, consolidated and
-- reported should consider the more advanced @{#asset.newTable} API.
--
-- Data send through `pushData` can be a flat set of key/values in a record,
-- or can contain nested sub-records. It can also be a simple value out of a
-- record, if the path is not empty: in this case, the last path segment will be
-- used as a datastore key. You also can add a timestamps in the record to inform
-- to the server side about the sending date.
--
--     --Examples
--     asset :pushData('foo', {x=1, y=2}) -- sends <assetroot>.foo.x=1, <assetroot>.foo.y=2
--     asset :pushData('foo.x', 1, 'midnight') -- sends <assetroot>.foo.x=1 according to the "midnight" policy
--     asset :pushData('foo', { timestamp=os.time(), y={a=2, b=3}}) -- sends <assetroot>.foo.y.a=2 and <assetroot>.foo.y.b=3 with the timestamp
--
-- **Note:** Notice that for the server, all data is timestamped. If there is 
-- no 'timestamp' or 'timestamps' entry on a record, it will be timestamped 
-- at the date of its reception by the server (see more detail [Racon_Lua_library](http://git.eclipse.org/c/mihini/org.eclipse.mihini.git/tree/doc/md/agent_connector_libraries/Racon_Lua_library.md))
--
-- @function [parent=#asset] pushData
-- @param self
-- @param path the datastore path under which data will be stored relative to
--   the asset node (optional).
-- @param data a table of key/value pairs, or a simple non-table serializable
--   value.
-- @param sendpolicy name of the policy controlling when the data must be sent
--   to the server, if omitted then the default policy is used.
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

function MT_ASSET :pushData(path, data, sendpolicy)
    checks("racon.asset", "string|table", "?", "?string")
    if not self.registered then return nil, "asset not started" end

    if path and data==nil then -- data parameter omitted.
        checks("?", "table", "nil", "nil")
        path, data, sendpolicy = '', path, nil
    end

    local arg, msg =  {
        asset  = self.id,
        policy = sendpolicy,
        path   = path,
        data   = data }

    if arg then
       sem_wait()
       local s, b = common.sendcmd("PData", arg)
       sem_post()
       return s, b
    else
       return nil, msg
    end
end


--------------------------------------------------------------------------------
-- Changes the hook function to be called when the asset receives a software
-- update request from the portal.
--
-- **Notes:**
--
-- * There can be only one pending `SoftwareUpdate` request at a time.
-- * Only one hook can be registered for the whole asset
-- * If no user update hook is set, the error code 472 (meaning "not supported /
--   not implemented") will be reported to the server.
-- * Any error coming from this update request means that the whole update
--   process will be considered as failed.
-- * When an update request tries to install a version that is already
--   installed, the application should return success value.
--   Indeed, in some cases the asset instance won't receive and report the hook's
--   result (e.g. because of a poorly timed reboot). As a result, the update
--   request will be sent again, and the hook should report a success immediately.
--
-- @function [parent=#asset] setUpdateHook
-- @param self
-- @param hook the new function to handle software update request.
--   hook signature: <br> `hook(componentname, version, path, parameters)`
--
-- * `componentname` (string) is the identifier of the component to update,
--    the name is defined in update manifest file, here it is provided
--    without the assetid at the beginning.
-- * `version` (string) is the version of the component to install
--    version can be empty string (but not nil!) to specify de-installation
--    request, non empty string for regular update/install of software
--    component.
-- * `path` (string) can be empty when version is empty too, otherwise path
--    will be a non empty string defining the absolute path of the file/folder
--    to use to update the application.
-- * `parameters` (table), can be nil, when set it contains the content
--     of parameters fields from update package, those parameters provide a way to give application specific
--     update parameters.
--    <br><br>
-- * `return value`: an integer, 200 for success, any other value means error.
--     Values from 480 to 499 are reserved for applicative error codes,
--     so it is highly recommended to use one (or more) of those to signify
--     an error coming from this update hook.
--     Non-integer return values will be rejected and be replaced by
--     default value 471.
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

function MT_ASSET :setUpdateHook(hook)
    checks('racon.asset', 'function')
    self.notifyswu = hook
    return 'ok'
end

-- -----------------------------------------------------------------------------
-- Creates and returns a @{racon.table#table} instance.
--
-- @function [parent=#asset] newTable
-- @param self
-- @param path (relative to the asset's root) where the data will be sent.
-- @param columns list of either @{racon.table#columnspec} or column names (to
--  use default values).
-- @param storage either string `"file"` or `"ram"`; how the table must be persisted.
-- @param sendPolicy name of the policy controlling when the table content is
--  sent to the server.
-- @param purge boolean indicating whether an existing table, if any, must be
--  recreated (`true`) or reused (`false`/`nil`). Recreation means the table
--  will be dropped then recreated from scratch (so any data inside table will
--  be lost).
--
-- @return an @{racon.table#table} to store and consolidate structured data.
-- @return `nil` + error message otherwise.
--

MT_ASSET.newTable = require 'racon.table'

--------------------------------------------------------------------------------
-- racon.asset data tree
--
-- An @{#asset} contains a data tree, in its field `tree`.
--
-- When the server sends data to an asset, the way it reacts is determined by
-- the tree's content:
--
-- * if there's a function in the tree under a path which is the prefix of the
--   data's path, this function is called as a handler;
-- * if there's no specific handler, but a handler is found in a `__default`
--   field in a prefix of the data's path, this generic handler is used. If
--   several generic handlers are found, the one corresponding to the longest
--   prefix is chosen.
-- * if no handler is found at all, the value sent by the server is written
--   in the tree, under its path.
--
-- For instance, if the server sends `myAsset.foo.bar=123`:
--
-- * if there's a function in one of `myAsset.tree.foo.bar`, `myAsset.tree.foo`
--    or `myAsset.tree`,  this function is called;
-- * if there's no such function, but at least one default handler is found
--   in either `myAsset.tree.foo.__default` or `myAsset.tree.__default`, the one
--   with the longest path is called;
-- * if no handler is found, the value `123` is written in `myAsset.tree.foo.bar`.
--
-- Handlers are functions which receive as parameters:
--
-- * `self` the asset instance;
-- * `data` the map of keys/values sent by the server; keys are paths relative
--   to the node where the handler is attached;
-- * `path` the path string where the handler is attached; concatenated in front
--   of each key, it allows to retrieve absolute paths.
--
-- Handlers must return:
--
-- * the status code `0` to indicate success;
-- * a non-zero numeric status code to indicate failure, optionally followed
--   by an error message.
--
-- The data tree comes with a pre-installed handler function on the path
-- `myAsset.tree.commands.ReadNode`. It handles standard requests by the server
-- when it wants to read the content of an arbitrary path in the asset's tree.
--
-- Notice that handler functions should be installed in the data tree before
-- the asset is started with @{#asset.start}: otherwise, any server message
-- received between the start and the handler attachment will be lost.
--
-- @field [parent=#asset] tree


-- Handler for software update requests; will be attached by `init()`.
local function emp_handler_SoftwareUpdate(payload)
    local path, version, url, parameters = unpack(payload)

    --payload is serialized as niltoken in command SoftwareUpdate
    --so we have to niltoken again to ensure that we give a nil value to user in case of niltoken value
    if version then version = niltoken(version) end
    if url then url = niltoken(url) end
    if parameters then parameters = niltoken(parameters) end

    local assetname, package = upath.split(path, 1)
    local self = M.assets[assetname]
    if not self then
        log('RACON-ASSET', 'ERROR', "Software update for unknown asset %q", assetname)
        return errnum 'WRONG_PARAMS'
    end
    local status, err, res
    if self.notifyswu then
        --TODO more checks on package, version, url values may be needed.
        res, err = self.notifyswu(package, version, url, parameters)
        status = tonumber(res) or 471 --User update callback failed
        if res ~= "async" then
            common.sendcmd("SoftwareUpdateResult", { path, status })
        end
    else
        status = 472 --No User update callback set
    end
    return 0
end

--MUST NOT BLOCK !!!!
local function emp_ipc_broken_handler()
   --block sem: services using EMP are blocked until sem is released
   -- This avoids data races between reregistration and other threads which send EMP commands
   M.sem_value = 0
   sched.run(function()
        for id, asset in pairs(M.assets) do
           asset.registered = false
           local r, m = asset:start()
           if not r then log('RACON-ASSET', 'WARNING', "Failed to register back asset \"%s\"", tostring(asset.id)) end
        end
         end
        )
end

-- Initialize the module.
function M.init()
    if M.initialized then return "already initialized" end
    local a, b = asset_tree.init()
    if not a then return a, b end
    common.emphandlers.SoftwareUpdate = emp_handler_SoftwareUpdate
    common.emp_ipc_brk_handlers.AssetIpcBrkHandler = emp_ipc_broken_handler
    M.initialized=true
    return M
end

--------------------------------------------------------------------------------
-- Sends the result of the software update request previously received by an asset.
--
-- @function [parent=#asset] sendUpdateResult
-- @param self
-- @param componentName a string, this must be the same value as the one
-- that was given as argument to the update hook (the hook registered with #setUpdateHook). <br>
-- As only one software update is possible for the same component at the same time,
-- the couple asset+componentName fully identifies the software update request.
-- @param updateResult a number, the result of the update, 200 for success,
-- any other value means error. <br>
-- Values from 480 to 499 are reserved for applicative error codes, so it is highly recommended
-- to use one (or more) of those to signify an error coming from an asset update.
--
-- @return: "ok" on success
-- @return `nil` + error message otherwise.
function MT_ASSET :sendUpdateResult(componentName, updateResult)
    checks("racon.asset","string")
    sem_wait()
    updateResult = tonumber(updateResult) or 471 --error ##21
    local r , msg = common.sendcmd("SoftwareUpdateResult", { upath.concat(self.id, componentName), updateResult })
    if r then
       sem_post()
       return "ok"
    else
       sem_post()
       return nil, string.format("Unable to send the update result to the agent, err = [%s]", tostring(msg))
    end
end



-- Create lowercase aliases for CamelCase API entries
require 'utils.loweralias' (MT_ASSET)

return M
