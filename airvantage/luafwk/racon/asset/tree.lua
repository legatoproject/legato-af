-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local utils_path  = require 'utils.path'
local utils_table = require 'utils.table'
local errnum      = require 'returncodes'.tonumber

require 'coxpcall'
require 'print'

local M = { initialized = false }

-- Retrieve a registered asset from its name
local function get_asset (asset_name)
    return require 'racon.asset' .assets [asset_name]
end

--------------------------------------------------------------------------------
-- Match pieces of data sent by the server with the appropriate handlers in
-- the asset's tree.
--
-- There are 3 ways to handle server data, by decreasing order of preference:
--
-- * if there's a function under the same path in the asset tree, call it
--   with these data; for instance, if there is a function in
--   `asset_tree.foo.bar`, it will handle `data_tree.foo.bar` and all of its
--   suffixes such as `data_tree.foo.bar.and.any.subpath.below`.
--
-- * if there's a default function, registered under a `"__default"` key,
--   in a path prefix to the data path, it will handle the data. If several
--   default handlers appear along the path, the deepest one (with the longest
--   common prefix) is chosen. For instance, if there are handlers
--   `asset_tree.foo.__default` and `asset_tree.foo.bar.__default`, and some
--   data is received under path `data_tree.foo.bar.gnat`, the second default
--   handler will handle it.
--
-- * if no handler is provided, then the value is copied from `data_tree` to
--   `asset_tree` under the same path. If one wants to prevent this behavior,
--   one needs to provide a `__default` handler somewhere up in `asset_tree`.
--
-- `data_tree` might contain many variables, handled by several different
-- handlers: each of them will be called with the appropriate data.
--
-- This function also handles acknowledgment:
--
-- * it sends an "Acknowledge" message with status 0 to the server if every
--   data has been handled successfully;
--
-- * it sends an "Acknowledge" message with a non-zero status to the server if
--   one handler at least failed. In case of failure, the number of handlers
--   applied is unspecified. If an application needs finer-grained
--   acknowledgments, it is the server's responsibility to send data in
--   separate messages with different ticket ids.
--
-- * if one at least of the handlers returned `"async"`, it doesn't acknowledge
--   any ticket. It is the handler's responsibility to do so. If several handlers
--   return `"async"`, there is no protection against each of them acknowledging
--   its data, resulting in several acknowledge messages with the same ticket id.
--   It is the application's responsibility--both on the server and embedded
--   sides--not to let such situations occur.
--
-- Handlers can return:
--
-- * a numeric value, which is interpreted as a status code
-- * the `"async"` string, which means that ticket acknowledgment will be handled
--   by the handler.
-- * any other value, which indicates success if true, failure if false.
--   (the doc states that only `"ok"` is acceptable, but all true-like non-number
--   values are actually supported).
--
-- They're called with parameters:
-- * `asset` (racon asset isntance);
-- * `data_tree`, containing only the data relevant to them;
-- * path prefix (prepended to `data_tree` keys, it gives their absolute paths);
-- * ticket_id if applicable.
--
-- Implementation
-- --------------
--
-- This function travels over `data_tree` recursively. It keeps track of the
-- current matching position in `asset_tree` by narrowing the `asset_tree`
-- parameter and appending segment to the `path` parameter when it goes
-- deeper into the tree. It also keeps the best `__default` handler found so
-- far.
--
-- data are erased from `data_tree` as they are handled. They are only left
-- (or more accurately written back) if no handler has been found, and they
-- will have to be passed to a default handler.

-- Finds, recursively, the deepest handler in the device's tree which can
-- handle data in the specified path. Once found, the appropriate handler
-- is called.
--
-- Only one handler, __default or otherwise, will be executed.
-- If no specific handler is found, the __default the closest to the data
-- (i.e. the deepest in the path) is triggered.
--
-- @param asset the @{racon.asset} instance
--
-- @param asset_tree the part of the device tree matching the curent part
-- of the data tree. At the top-level call, this will be equal to `asset.tree`.
--
-- @param data_tree part of the data sent by the server, which need to be
--   handled. At the top-level call, this will be the content of the full
--   "SendData" message, while in recursive calls it will be a sub-tree thereof.
--
-- @param path the path prefix, relative to the top-level. At the first call
--   it will be equal to `""`, and keys will be added to it as the algorithm
--   descends into `asset_tree` and `data_tree`. This path prefix is common
--   to both trees, i.e. if we're in path `"foo.bar"` in `asset_tree`, we're
--   also in path `"foo.bar"` of `data_tree`.
--
-- @param ticket_id the optional acknowledgement id, to be passed to the handler
--   function if available. This parameter is only useful for `"async"` returning
--   handlers.

-- @param best_default_handler the `__default` handler with the longest matching
--   path prefix found so far. Will be called if no more specific handler is
--   found.
--
-- @return status code, plus a message in case of failure.
--
--------------------------------------------------------------------------------
local function match_asset_data_trees(asset, asset_tree, data_tree, path, ticket_id, best_default_handler)
    checks('racon.asset', '?', 'table', 'string', '?string|number', '?function')

    local state = { code="ok", msg=nil }

    -- Return true iff a handler's result indicates a failure; update state
    local function is_handler_error(code, msg)
        if code=='async' then state.code='async'; return false end
        if code=='ok' or tonumber(code)==0 then return false end
        local num_code = tonumber(code)
        if num_code or (not code) then
            state.code, state.msg = num_code or -1, msg
            log("RACON-ASSET-TREE", "WARNING", "handler error detected %s", tostring(state.msg))
        else -- code is neither false nor numeric -> it's invalid
            log('RACON-ASSET-TREE', 'ERROR', "Handler didn't return a status code (%s)", sprint(code))
            state.code, state.msg = -999, "Invalid return from handler at path "..path
        end
        return true -- error
    end

    -- Update if a better default handler is found
    best_default_handler = asset_tree.__default or best_default_handler

    -- Loop on each data piece: find a specific handler, leave alone (actually
    -- write back) for a later call to a default handler, or write into the
    -- asset tree if no suitable handler is found.
    for k, data_val in utils_table.sortedpairs(data_tree) do
        data_tree[k] = nil -- Remove it. Will be pushed back if a default handler needs it
        local asset_subtree = asset_tree[k]
        local subpath = utils_path.concat(path, k)
        local asset_handler_type = type(asset_subtree)

        -- Specific handler found: run it and collect the returned status
        if asset_handler_type == 'function' then
            local tmpstatus , tmpmsg =  asset_subtree(asset, data_val, subpath, ticket_id)
            log("RACON-ASSET-TREE", "DEBUG", "data handling: specific handler at path %q returns %s",
                tostring(subpath), tostring(tmpstatus))
            if is_handler_error(tmpstatus , tmpmsg) then
                return state.code, state.msg -- stop at first error, report it to caller
            end

        -- Handler is a table of more precise handlers: go down recursively or overwrite
        elseif asset_handler_type == 'table' then
            if type(data_val) == 'table' then -- go down and treat each part of `data_tree` separately
                local tmpstatus , tmpmsg = match_asset_data_trees(
                    asset, asset_subtree, data_val, subpath, ticket_id, best_default_handler)
                log("RACON-ASSET-TREE", "DEBUG",
                    "data handling: recursive call for path %s returned %s",
                    tostring(subpath), tostring(tmpstatus))
                if is_handler_error(tmpstatus, tmpmsg) then
                    return state.code, state.msg  -- propagate stop at first error
                end
            else -- if data isn't a table, just overwrite `asset_tree`
                -- TODO: why are we sure that no __default handler could have handled it?
                asset_tree[k] = data_val
            end

        -- `asset_subtree` is not a table: call `__default` or write data in `asset_tree`
        else
            if asset_subtree == nil and best_default_handler then
                -- No more handler, but a __default was found. Write data
                -- back into __default's future parameter.
                data_tree[k] = data_val
            else -- No handler, no default: write data into `asset_tree`.
                asset_tree[k] = data_val
            end
        end
    end

    -- If some data hasn't been erased, it's in order to be passed to a __default
    -- handler. Call it and handle its returned status.
    if next(data_tree) then
        local tmpstatus , tmpmsg = best_default_handler(asset, data_tree, path, ticket_id)
        log("RACON-ASSET-TREE", "DEBUG", "data handling: __default at %s returned %s",
            tostring(path), tostring(tmpstatus))
        if is_handler_error(tmpstatus , tmpmsg) then
            return state.code, state.msg -- stop at first error, report it to caller
        end
    end

    log("RACON-ASSET-TREE", "DETAIL", "match_asset_data_trees: processed path %s, returning code '%s', msg '%s'",
        tostring(path), tostring(state.code), tostring(state.msg))
    return state.code, state.msg
end

-- Handle data sent by the portal to the device through the agent.
local function emp_handler_SendData(data)
    local id, path  = utils_path.split(data.path, 1)
    local asset     = get_asset(id)

    if not asset then
        log('RACON-ASSET-TREE', 'ERROR', "Received data from server to unknown/closed asset %q", id)
        return errnum 'WRONG_PARAMS', "unknown or closed asset"
    end
    local tree = asset.tree

    if not tree then return 0 end -- no data tree from this asset
    local record, ticket = data.body, (tonumber(data.ticketid) or 0)
    local treeimg = { } --will hold input data transformed into tree representation
    if record.__class == "Response" then -- Receive an acknowledgement from server
        log("RACON-ASSET-TREE", "INFO", "Server acknowledged ticket %d (path:'%s', status:'%s')", record.ticketid, path, record.status)
        return 0
    elseif log.musttrace('RACON-ASSET-TREE', 'DETAIL') then
        log('RACON-ASSET-TREE', 'DETAIL', "Data: '%s' received (path:'%s')", sprint(record), path)
    end
    if type(record) ~= 'table' then
        log('RACON-ASSET-TREE', 'ERROR', "Bad SendData record coming from server: %s", sprint(record))
        error 'bad m3da format'
    end

    for k, v in pairs(record) do
        utils_path.set(treeimg, utils_path.concat(path, tostring(k)), v)
    end
    --treeimg is now filled with arborescent data to be given to the asset tree handlers.
    if log.musttrace('RACON-ASSET-TREE', 'DEBUG') then
        log("RACON-ASSET-TREE", "DEBUG", "Reorganized received data: '%s'", sprint(treeimg))
    end
    local co_status, hdl_status, hdl_msg
    if type(tree) == "function" then -- Case not handled properly by `match_asset_data_trees`
        --the whole tree can be handled by a single function: then call it here
        co_status, hdl_status, hdl_msg = copcall(tree, asset, treeimg, "", ticket~=0 and ticket)
    else
        co_status, hdl_status, hdl_msg = copcall(match_asset_data_trees, asset, tree, treeimg, "", ticket~=0 and ticket or nil)
    end
    --note that treeimg has been modified (mostly emptied) by match_asset_data_trees

	local status, msg
	if co_status then status, msg = hdl_status, hdl_msg -- status/msg returned by handler
	else status, msg = -1, tostring(hdl_status or "unspecified error") end -- copcall error msg; TODO:
	 
    if not status then
        log("RACON-ASSET-TREE", "ERROR", "While writing for path '%s' in the tree: msg: '%s'", tostring(path), tostring(msg))
    end
    if ticket~=0 and status~='async' then -- Immediate acknowledgement, can be OK or KO
        log("RACON-ASSET-TREE", "INFO", "Asset tree acknowledges ticket %d (path:'%s', status:'%s')",
        tostring(ticket), tostring(path), tostring(status))
        --racon.acknowledge deals with status value conversion if needed
        require 'racon'.acknowledge(ticket, status, msg)
        status = tonumber(status) or (status and 0 or -1) --emp status, we may want to force 0 or -1 only
    elseif status=='async' then -- Handler will take care of acknowledgement itself
        log("RACON-ASSET-TREE", "INFO", "Asset acknowledgement left to the handler")
        status = 0 --emp status
    elseif not status then -- Command failed, but the server didn't ask for acknowledgement
        log('RACON-ASSET-TREE', 'ERROR', "Error while running asset datawriting handler: %s", tostring(msg))
        status = -1 --emp status
    elseif not tonumber(status) then -- convert the likes of `"ok"` into `0` status
        status = 0 --emp status
    end
    assert(type(status)=='number')
    return status, msg
end

-- This command reads the value associated with a node in the tree, and
-- returns it as a result.
function M.ReadNode(asset, values)
    local path, policy = unpack(values)
    path = path and tostring(path) or ""
    log("RACON-ASSET-TREE", "INFO", "ReadNode: read path %q from asset %s's tree",
        path, asset.id)
    local store = asset.tree
    for i,segment in ipairs(utils_path.segments(path)) do
        if type(store) ~= "table" then return nil, "path not found" end
        store = store[segment]
    end
    if type(store)=='table' then
        local function treefilter(k, v)
            local t = type(v)
            if t=="function" or t=="userdata" or t=="thread" then return nil end
            return v
        end
        store = utils_table.map(store, treefilter, true, false)
    end
    if log.musttrace('RACON-ASSET-TREE', 'DETAIL') then
        log('RACON-ASSET-TREE', 'DETAIL', "path: '%s' = '%s'", path, sprint(store))
    end
    assert(asset :pushdata(path, store, policy or 'now'))
    return 0
end

function M.init()
    if M.initialized then return "already initialized" end
    local common = require 'racon.common'
    common.emphandlers.SendData = emp_handler_SendData
    M.initialized=true
    return M
end

return M
