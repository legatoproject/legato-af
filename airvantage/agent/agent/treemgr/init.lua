-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- Tree Manager
-- ============
--
-- Purpose
-- -------
--
-- Offer a generic mechanism to present a device's data as a set
-- of variables, organized in a hierarchical tree, which can be
-- read, written, and monitored for changes by user applications.
--
-- The architecture consists of 3 parts:
--
-- * A logical tree, whose hierarchical organization is user-friendly
--   and portable, is presented to user applications.
--
-- * Actual data manipulation is performed by device-specific
--   handlers, which work on handler trees. The handler trees' layout
--   is organized in a way which eases implementation, and doesn't
--   have to match the logical tree's layout. Handler API is exposed
--   to developers who want to contribute new variables to the tree
--   manager, but not to normal applications.
--
-- * a mapping definition, allowing the core tree engine to translate
--   logical paths into handler paths and the other way around.
--   This mapping is compiled into CDB databases, so that large
--   mappings don't have to fit entirely in RAM.
--
-- Sub-modules
-- -----------
--
-- The tree manager is organized in several sub-modules:
--
-- * `treemgr` is the core engine, which coordinates handlers with the
--   logical view of the tree.
-- * `treemgr.db` interfaces with the CDB databases which describe
--   the mapping.
-- * `treemgr.build` compiles `"*.map"` files into a set of `"*.cdb"`
--   database files, organized for quick access to translation info.
-- * `treemgr.handlers.*` are handler implementations.
--
-- Concepts
-- --------
--
-- The API works with paths (sequences of identifiers separated by
-- dots), which denote nodes in trees. The node can be leaf nodes or
-- non-leaf nodes. The non-leaf nodes have children, but carry no data
-- to read or write. Leaf nodes carry some data to read/write/monitor,
-- but have no children nodes.
--
-- In the example below, `a` and `a.b` are non-leaf nodes, with
-- children `{"a.b", "a.d"}` and `{"a.b.c"}` respectively. Leaf nodes
-- `"a.b.c"` and `"a.d"` have no children, but carry a value each.
--
--     -a
--      +-b
--      | +-c = 123
--      +-d = 234
--
-- The mapping between the logical tree and handler trees consists of
-- a set of mountpoints. A mountpoint associates a path in the logical
-- tree to a handler, and a path within this handler. Mountpoints can
-- map leaf nodes as well as non-leaf nodes.
--
-- Here is an example of mapping between a logical tree and two handlers:
--
-- Logical tree:
--
--     -system
--      +-position
--      | +-latitude
--      | +-longitude
--      | +-elevation
--      +-time
--      +-agentconfig
--        +- server
--        +- agent
--        +- modem
--        +- ...
--
-- Handler trees:
--
--      aleos
--      +-GPS_LATITUDE
--      +-GPS_LONGITUDE
--      +-GPS_ELEVATION
--      +-TIME
--
--      config
--      +- server
--      +- agent
--      +- modem
--      +- ...

-- In the example above, we want to map:
-- * `system.position.latitude`  with `aleos:GPS_LATITUDE`;
-- * `system.position.longitude` with `aleos:GPS_LONGITUDE`;
-- * `system.position.elevation` with `aleos:GPS_ELEVATION`;
-- * `system.time` with `aleos:TIME`.
-- * `agentconfig` with `config:<root>`
--
-- Each aleos variable above is mapped individually, but the whole
-- `config` tree is mapped recursively: by mapping
-- `config:<root>=agentconfig`, one gets for instance
-- `config:server.url=agentconfig.url`,
-- `config:mediation.pollingperiod.GPRS=`agentconfig.mediation.pollingperiod.GPRS`,
-- etc.
--
-- Naming conventions
-- ------------------
--
-- In this module, the following variable naming conventions are
-- chosen:
--
-- * `hpath` stands for a path relative to a handler, which
--   might denote a leaf node as well as a non-leaf node
--
-- * `hlpath` stands for a handler's leaf node.
--
-- * `lpath` is an absolute path in the logical tree (leaf or non-leaf).
--
-- * `llpath` is an absolute path to a leaf node in the logical tree.
--
-- * `nlnpath` is a non-leaf logical path.
--
-- * `hmap` is an `hpath->value` table.

-- * `lmap` is an `lpath->value` table.
--
-- Handlers
-- --------
--
-- Handlers work with paths relative to themselves; handler paths are
-- not shown directly to user applications, they need to be mapped
-- into the logical tree first.
--
-- The features needed from handlers are provided through methods:
--
-- * `handler:get(hpath)` allows to retrieve the value
--   associated with a leaf-node path, or the set of children under
--   a non-leaf node path. Get can return:
--
-- ** a value, followed by nil
--
-- ** nil, followed by a string error message
--
-- ** nil, followed by a children table. The children must be in the table's
--    keys, not in its values, e.g. `{ x=true, y=true }` is correct, but
--    `{ 'x', 'y' }` is not. The path toward the children must be relative
--    to the `hpath` argument.
--
-- * `handler:set(hmap)` allows to write a map of `hlpath->value` pairs in the
--   handler. It is not expected to return anything meaningful.
--
-- * `handler:register(hpath)` signals that any change of a variable's value,
--   or a value change of any variable under a non-leaf node, must
--   be notified to the tree management system. Notification must
--   be performed by the handler, by calling `treemgr.notify()`
--   everytime it modifies a registered variable.
--
-- * `handler:unregister(hpath)` signals that the logical tree doesn't need
--   to be notified about changes to the `hpath` variable anymore.
--
-- * `treemgr.notify(handler_name, hmap)` must be called by handlers to signal
--   that a set of variables has changed. The engine will take care of converting
--   hpaths into lpaths, retrieving the hooks to notify, sort out the variables
--   (remove the irrelevant ones, add the missing associated ones) for each hook.
--
-- Logical tree
-- ------------
--
-- This is the API accessible to user applications. It offers get /
-- set / notification services, on variables organized according to
-- a map which is independent from the organization by handlers or
-- within handlers.
--
-- Applications can read values with `get()`, write values with `set()`,
-- register hook functions to be triggered everytime a variable changes
-- with `register()`:
--
-- * `treemgr.get(lpath)` returns a value or a list of children node
--   names, depending on whether the path denotes a leaf node or a
--   non-leaf node.
--
-- * `treemgr.get(lpath_list)` performs a batch reading, returns an lmap of
--   values and/or a list of children node lpaths.
--
-- * `treemgr.set(llpath, value)` sets the value of a leaf node.
--
-- * `treemgr.set(lpath, map)` where map keys `k_n` are strings such that
--   `lpath .. "." .. k_n` are logical leaf paths sets a set of
--   leaf node values in a single operation.
--
-- * `treemgr.set(lmap)`, where map keys are logical leaf paths, sets a set
--   of leaf node values in a single operation.
--
-- * `treemgr.register(lpath_list, hook, associated_lpath_list)` registers a
--   hook to be triggered eveytime one of the variables denoted by
--   lpath_list changes. The hook receives an `llpath->value` map
--   argument, which lists the union of every variable in `lpath_list`
--   which changed, plus every vaiable in `associated_lpath_list`.
--   If a variable must be monitored, and its value is needed by the
--   hook even if it didn't change, then it needs to be listed both
--   in `monitored_lpath_list` and in `associated_lpath_list`.
--
-- Mapping and handler loading
-- ===========================
--
-- A treemgr configuration consists of handlers and a mapping. Handlers are
-- Lua objects which implement the `:get()`, `:set()`, `:register()` and
-- `:unregister()` methods. The mapping is a set of bidirectional
-- correspondances between logical tree nodes and handler nodes.
--
-- The mapping is stored in a CDB (Constant DataBase): it ensures conversions
-- between the user view (logical paths) and the implementation view (handler
-- path) in constant memory and time. It is built from `"*.map"` files, but
-- once the DB is built, map files are not needed anymore.
--
-- The handlers are loaded lazily, when they are needed to fulfill a user
-- request. They are identified by the name of the Lua module which implements
-- them: if a handler is called `'agent.treemgr.handlers.ramstore'`, then a
-- call to require `'agent.treemgr.handlers.ramstore'` must return the handler
-- object. By sitting directly above Lua's module management system, the
-- handler loading system benefits from its flexibility and its various
-- predefined loaders.
--
-- To facilitate build and deployment, treemgr is able to recompile its CDB
-- from map files on target, if they are present and more recent than the DB.
-- By convention, all the map files in `persist/treemgr` are compiled into the
-- DB. Each map file describes the mappings of one handler.
--
-- A given treemgr configuration is described through a set of `"*.map"` files:
-- each map file lists one handler, and a list of mappings, between nodes in
-- this handler and nodes on the global, logical tree. The link between the
-- handler's map and its code is maintained through Lua's `require` module
-- system: the handler's name must be a valid Lua module name, and the result
-- of loading this module must be the handler object, ready to run.
--
-- Map files are precompiled into CDB databases, for faster access in constant
-- memory. CDB results could be cached in RAM if necessary, although it is not
-- currently implemented.
--
-- Each architecture might have different ways to provide the same service,
-- and might not provide the exact same set of services as others, depending
-- e.g. on available hardware. By assembling the correct set of specific
-- handlers, together with the map files which put variables at a standard
-- place in the logical tree, one builds the target-specific implementation
-- of the portable treemgr interface.
--
--
-- Implementation
-- ==============
--
-- One strong implementation constraint is that the logical tree can
-- be big, and must not be required to fit entirely in RAM. It is
-- therefore built as a read-only database, based on
-- [cdb](http://tbd).
--
-- There are four dictionaries to be kept in the database:
--
-- * `lpath -> handler_name:hpath` allows `get` and `set` to translate
--   logical paths into handler paths, to which the actual read / write
--   operations are delegated.
--
-- * `handler_name:hpath -> lpath` allows `notify` to translate handler
--   notifications into logical ones, which will be presented to the
--   relevant apllicative hook. A given handler + path can be mounted
--   in more than one place, and therefore have several values
--   associated with it in the database.
--
-- * `lpath -> direct_children_lpaths_list` is used by `get` to retrieve
--   the children of non-leaf nodes. Parent and children lpaths are both
--   absolute.
--
-- * `lpath -> mounted_handlers_below_node` is used by `register`: when a
--   hook is registered on a non-leaf node, it must register every handler
--   mounted below it. This database lists these, indexed by ancestor nodes
--   (this means that handlers mounted below the root node are indexed more
--   than once).
--
--
-- get
-- ---
--
-- The get operation on leaf node is straightforward: `llpath` is
-- translated to `handler_id, hlpath`, then the handler is retrieved
-- and its get method is called with the proper argument.
--
-- A get on a non-leaf node must return the list of every direct child
-- of the node. If the node is under a handler, then this handler's
-- `get` method is in charge of providing this list. Hence, the `get` of a
-- non-leaf node depends on the handlers mounted above it ("above"
-- being understood inclusively, i.e. a node is considered to be above
-- itself. For instance, if there's a non-leaf mountpoint on lpath
-- `"a.b"`, then a get request on `"a.b"` depends on this mountpoint,
-- not on any mountpoint on `"a"` nor on the root node).
--
-- However, it also depends on handlers mounted below it. For
-- instance, if there's a handler mounted on path `"a.b.c"`, and the
-- application gets the list of `"a"`'s children, then `"b"` must be
-- included in this list, whether there's also a handler mounted on
-- `"a"` or not.
--
-- If a get operation covers several paths mapping several handlers,
-- a logical get can trigger several handler get operations. However,
-- it makes sure to perform at most one get operation per handler, thus
-- giving the handler an opportunity to optimize retrieval operations.
--
--
-- set
-- ---
--
-- Set operations are mostly the same as leaf-node get requests.
-- The differing part (writing hpaths rather than reading them) is
-- specific to each handler.
--
--
-- register
-- --------
--
-- Hook registrations call the `register` method(s) of the
-- corresponding handler(s), so that they will know they must provide
-- notifications. Those notifications are produced by calling
-- `notify`, which will:
--
--  * convert `hlpath`s into `llpaths`;
--  * request and add the variables in `associated_lpath_list`;
--  * call the appropriate hook with the resulting map.
--
-- The logical registration on a non-leaf node must translate into
-- registrations:
--
--  * on the first mountpoint node above it;
--  * on the root of every mountpoint below it.
--
--
-- unregister
-- ----------
--
-- The application can unregister from logical paths it's not interested in
-- anymore. Before unregistering from the corresponding hpaths, though,
-- one must first ensure that no other hook needs this hpath. This requires
-- to check whether the hpath is mapped to other lpaths, as it can be mapped
-- more than once, either directly or through an ancestor node.
--
--
--
-- @module treemgr

local M = { }

local path        = require 'utils.path'
local db          = require 'agent.treemgr.db'; M.db = db
local utils_table = require 'utils.table'
local niltoken    = require 'niltoken'
local lfs         = require 'lfs'

require 'print'

local function cleanpath(t)
    if type(t) == "table" then
        for i, p in ipairs(t) do
            t[i]=path.clean(p)
        end
    else
        t = path.clean(t)
    end
    return t
end

--- Helper which associates a handler name with the handler itself.
--  It is currently just wrapping Lua's `require`, as handler names
--  are the names of the Lua modules returning them.
local function get_handler(name)
    local h = package.loaded[name]
    if h then return h end

    if name:match '%.extvars%.' or name:match '^extvars%.' or name:match '%.extvars$' then
       local extvars = require 'agent.treemgr.handlers.extvars'
       local apath = (LUA_AF_RO_PATH or lfs.currentdir()) .. "/lua/" .. name:gsub("%.", "/") .. ".so"
       local err
       h, err = extvars.load(name, apath)
       if not h then error (err) end
       package.loaded[name] = h
    else
       -- Not loaded yet: require it and check that the module works as specified.
       h = require(name)
       if not pcall(function() return h.get end) then
	  error ("Module "..name.." returns "..sprint(h)..
		 " instead of a valid handler")
       end
    end
    return h
end

--- Translates an handler_name + hpath into the list of all lpaths which map
--  it onto the logical tree.
--
--  @param handler_name
--  @param hpath
--  @return a list lpaths
local function hpath2lpath(handler_name, hpath)
    local results = { }
    -- printf("h2l: translate  %s:%s", handler_name, hpath)
    for hprefix, relpath in path.gsplit(hpath) do
        -- printf("h2l: trying to retrieve lprefix %q from DB", hprefix)
        for lprefix in db.h2l (handler_name, hprefix) do
            local lpath = path.concat(lprefix, relpath)
            -- printf("h2l: hit: lprefix = %s, relpath = %s , lpath = %s", lprefix, relpath, lpath)
            table.insert(results, lpath)
        end
    end
    return results
end

--- Retrieve the handler controlling a given logical node, and describe
--  how they relate in an `l2h` record.
--
--  @param lpath the logical path
--  @return TO BE DESCRIBED
-- @return nil, error_msg
local function lpath2hpath(lpath)
    for lprefix, relpath in  path.gsplit (lpath) do
        local handler_name, hpath = db.l2h (lprefix)
        if handler_name then
            local handler = get_handler(handler_name)
            local l2h = {
            handler_name = handler_name,
            handler = handler,-- handler controlling the `lpath` arg node
            lpath   = lprefix,-- logical node on which `handler` is mounted
            hpath   = hpath,  -- handler path mounted on `l2h.lpath`
            relpath = relpath -- path between `l2h.lpath` and the `lpath` arg
            }
            --print ("Converting lpath "..lpath); p(l2h)
            return l2h
        end
    end
    return nil, "no handler found"
end

--------------------------------------------------------------------------------
-- From a logical path or a list of paths, retrieves the values of leaf nodes,
-- and / or the children of non-leaf nodes.
--
-- Example
-- -------
--
-- Consider the following tree:
--
--     a
--     +-b
--     | +-c = 123
--     +-d = 234
--
-- * `get("a")` will return `nil, { "a.b", "a.d" }` (nil because there
--   is no value attached to `a`).
--
-- * `get("a.d")` will return `234, { }` (234 is the value, there are
--   no children)
--
-- * `get{ "a.d" }` will return `{["a.d"] = 234} , { }` (in case of
--   path lists, the values are indexed by path)
--
-- * `get{ "a.d", "a.b" }` will return `{["a.d"] = 234} , { "a.b.c" }`
--   (the former path adds a value, the latter adds a child).
--
--
-- @param lpath_list a logical path, or a list of logical paths
--
-- @param result_lmap an optional table; if provided, the values
--        are written in it rather than in a purpose-created table.
--
-- @return `values_map, children_paths` if `lpath_list` is a list: the
--         values are then presented as a path->value map;
--         `children_paths` is a list of paths to children of non-leaf
--         nodes.
--
-- @return `value, empty_list` if `lpath_list` is a path string, and
--         the corresponding node is a leaf node.
--
-- @return `nil, children_paths` if `lpath_list` is a path string, and
--         the corresponding node is a non-leaf node;
--         `children_paths` is a list of paths its children.
--
--function M.get(lpath_list, shortpaths, result_lmap)
--function M.get(prefix, lpath_list, result_lmap)
function M.get(...)
    local children_set -- will be initialized by first intermediate node
    local prefix, lpath_list, result_lmap

    if select("#", ...) >= 2 and type(select(1, ...)) == "string" then
        prefix, lpath_list, result_lmap = ...
    elseif type(select(1, ...)) ~= "nil" then
        prefix, lpath_list, result_lmap = nil, ...
    else
        prefix, lpath_list, result_lmap = ...
    end
    assert(type(prefix) == "string" or type(prefix) == "nil")
    assert(type(lpath_list) == "string" or type(lpath_list) == "table")
    assert(type(result_lmap) == "table" or type(result_lmap) == "nil")
    --remove unecessary chars (like "foo..bar")
    prefix = prefix and cleanpath(prefix)
    lpath_list = cleanpath(lpath_list)

    -- Perform a get for a single lpath.
    -- If children paths are found, they're added in `children_set`
    -- If a leaf value is found, it's returned
    -- If no leaf value is found, `nil` is returned
    -- If and only if an error occurs, an error string is returned as 2nd value
    -- So, contrary to `handler:get`, this local function won't return a table;
    -- it might only fill `children_set`
    local function get_lpath(lpath)
        checks('string')
        local handler_found = false
        local l2h = lpath2hpath (lpath)
        if l2h then
            -- delegate to the handler above
            handler_found = true
            local hpath = path.concat(l2h.hpath, l2h.relpath) -- TODO: cache this, indexed by lpath
            log('TREEMGR', 'DEBUG', "Get: trying to get lpath %q as hpath %q", lpath, hpath)
            local a, b = l2h.handler :get (hpath)
            if b==nil then return a
            elseif type(b)=='table' then
                log('TREEMGR', 'DEBUG', "Get: according to handler, %q is an intermediate node", l2h.lpath)
                children_set = children_set or { }
                -- convert relative hpaths into absolute lpaths
                for relpath, _ in pairs(b) do
                    --printf("tm: get: add %q + %q = %q to children set", lpath, relpath, path.concat(lpath,relpath))
                    children_set[path.concat(lpath, relpath)] = true
                end
            elseif type(b)=='string' then return a, b
            else error ("Invalid treemgr handler result: "..sprint(b)) end
        else log('TREEMGR', 'DEBUG', "Get: no mapping above lpath %q", lpath) end

        -- either no handler above, or the handler didn't return a leaf value:
        -- list children handlers, add their paths to children list
        for child_lpath in db.l2c (lpath) do
            children_set = children_set or { }
            handler_found = true
            log('TREEMGR', 'DEBUG', "Adding child leading to mapping point %q", child_lpath)
            children_set [child_lpath] = true
        end
        if handler_found then return nil
        else return nil, "NOT_FOUND" end
    end

    -- use `get_lpath` to act on multiple paths, to fill the optional `result_lmap`,
    -- to put children sets in form before returning it if applicable.
    local result, err_msg
    if type(lpath_list)=="string" then
        local path_result, err_msg = get_lpath(prefix and prefix .. "." .. lpath_list or lpath_list)
        if err_msg then return nil, err_msg end
        if prefix and lpath_list == "" then
           result = {}
           for key, value in pairs(children_set) do
               assert(key:sub(1, #prefix) == prefix)
               key = key:sub(#prefix+3, -1)
               result[key] = value
           end
           children_set = result
           result = nil
        elseif prefix and path_result then
           result = {}
           result[lpath_list] = path_result
	elseif path_result ~= nil then
           result = path_result
        end
        if result_lmap then result_lmap[lpath_list] = result end
    else -- actual lpath list, present the result(s) as an lmap.
        result = result_lmap or { }
        children_set = children_set or { } -- always return a list of children
        local has_results = false
        for k, v in ipairs(lpath_list) do
            local lpath = type(k)=='string' and k or v -- take sets as well as lists
            local path_result
            local path_result, err_msg = get_lpath(prefix and prefix .. "." .. lpath or lpath)
            if err_msg then return nil, lpath..": "..err_msg end
            if path_result then has_results = true end
            result[lpath] = path_result
        end
        if not has_results then result = nil end
        if prefix then
           result = {}
           for key, value in pairs(children_set) do
               assert(key:sub(1, #prefix) == prefix)
               key = key:sub(#prefix+2, -1)
               result[key] = value
           end
           children_set = result
           result = nil
        end
    end

    if result==nil and children_set then
        return nil, utils_table.keys(children_set)
    elseif result==nil then -- no children, no error (would have been caught by now)
        return nil, nil
    elseif children_set then -- result (map) and children
        return result, utils_table.keys(children_set)
    else -- result without children
        return result
    end
end

--------------------------------------------------------------------------------
-- Set values in logical tree leaf nodes.
--
-- The values are passed in `lmap`, a table mapping logical paths to
-- values.  Optionally, a prefix logical path can be passed as first
-- argument; the paths in the map are then relative to this node.
-- Finally, if a prefix path is given, the map can be replaced by a
-- single value, which will then be set under the prefix path.
--
-- Example
-- -------
--
-- The following lines are all equivalent:
--     set{ ['a.b.c'] = 123 }
--     set('', { ['a.b.c'] = 123 })
--     set('a', { ['b.c'] = 123 })
--     set('a.b', { c=123 })
--     set('a.b.c', { ['']=123 })
--     set('a.b.c', 123)
--
-- @param prefix_lpath an optional path prefix to add to every key in the map
-- @param lmap a path -> value map; can be a simple value if a non-empty prefix path
--        has been given.
-- @return true upon success
-- @return nil, error_message in case of failure
--
function M.set(prefix_lpath, lmap)
    -- make first arg optional
    if lmap==nil then
        local t=type(prefix_lpath)
        if t=='string' then lmap={['']=niltoken}
        elseif t=='table' then prefix_lpath, lmap = '', prefix_lpath
        else checks('string', 'table') end -- will cause a "bad arg" error msg
    elseif type(lmap) ~= 'table' then
        lmap = { [''] = lmap } -- allow single values out of maps.
    end

    checks('string', 'table') -- prefix lpath, key_suffix/value pairs

    local handler_maps = { } -- argument maps for handlers, indexed by handler.

    -- sort path/value pairs by handler
    for k, v in utils_table.recursivepairs(lmap) do
        local lpath = path.concat (prefix_lpath, k)
        local l2h   = lpath2hpath (lpath)
        if not l2h then return nil, "NOT_FOUND" end
        local hmap  = handler_maps[l2h.handler] and handler_maps[l2h.handler].hwmap or nil
        if not hmap then hmap={ }; handler_maps[l2h.handler]= { handler_name = l2h.handler_name, hwmap = hmap } end
        local hpath = path.concat(l2h.hpath, l2h.relpath)
        hmap[hpath] = v
    end

    -- call each handler with its map
    for handler, hmap in pairs(handler_maps) do
        if log.musttrace('TREEMGR', 'DEBUG') then
            log('TREEMGR', 'DEBUG', "Set: handler:set%s", sprint(hmap.hwmap))
        end
        local r, msg = handler :set (hmap.hwmap)
        if not r then return r, (msg or "UNSPECIFIED_ERROR") end
        if type(r) == "table" then
            local n = 0; for _,__ in pairs(r) do n = n + 1 end
            if n > 0 then return M.notify(hmap.handler_name, hmap.hwmap) end
        end
    end

    return "ok"
end


--------------------------------------------------------------------------------
-- Register the `hook` function, so that everytime a change
-- notification affects one of the logical leaf nodes in `lpath_list`,
-- `hook` is called with a logical map argument.  This map will describe the
-- value of:
--
--  * every node in `monitored_lpath_list` which changed;
--  * every node in `associated_lpath_list`, whether it changed or not.
--
-- Some paths can appear in both `monitored_lpath_list` and
-- `associated_lpath_list`.
--
-- **Beware that the later list must only contain leaf node paths!**
--
-- @param monitored_lpath_list lpaths which must trigger `hook`
--        whenever they change. Non-leaf nodes are allowed in this
--        list.
--
-- @param hook function called everytime a variable in `lpath_list`
--        changes.
--
-- @param associate_lpath_list leaf lpaths which must be reported to
--        `hook` everytime it's called, whether they changed or not,
--        even if they're not monitored.
--
--function M.register(prefix, monitored_lpath_list, hook, associated_lpath_list)
function M.register(...)
    local prefix, monitored_lpath_list, hook, associated_lpath_list

    if select("#", ...) >= 2 and type(select(1, ...)) == "string" and type(select(2, ...)) == "string" or type(select(2, ...)) == "table" then
        prefix, monitored_lpath_list, hook, associated_lpath_list = ...
    elseif type(select(1, ...)) == "nil" then
        prefix, monitored_lpath_list, hook, associated_lpath_list = ...
    else
        prefix, monitored_lpath_list, hook, associated_lpath_list = nil, ...
    end
    assert(type(prefix) == "string" or type(prefix) == "nil")
    assert(type(monitored_lpath_list) == "string" or type(monitored_lpath_list) == "table")
    assert(type(hook) == "function")
    assert(type(associated_lpath_list) == "string" or type(associated_lpath_list) == "table" or type(associated_lpath_list) == "nil")

    if type(monitored_lpath_list)=='string'
    then monitored_lpath_list = {monitored_lpath_list} end
    if type(associated_lpath_list)=='string'
    then associated_lpath_list = {associated_lpath_list} end
    associated_lpath_list = associated_lpath_list or { }

    if prefix then
        local root = prefix
        local monitored_spath_list = {}
        for _, path in pairs(monitored_lpath_list) do
            table.insert(monitored_spath_list, root .. "." .. path)
        end
        monitored_lpath_list = monitored_spath_list

        if associated_lpath_list then
            local associated_spath_list = {}
            for _, path in pairs(associated_lpath_list) do
                table.insert(associated_spath_list, root .. "." .. path)
            end
            associated_lpath_list = associated_spath_list
        end
    end

    -- TODO: ought to belong to utils.table
    local function list2set(list)
        local s = { }
        for _, x in pairs(list) do s[x] = true end
        return s
    end

    cleanpath(monitored_lpath_list)
    cleanpath(associated_lpath_list)

    local hook = {
        monitored_lpath_set = list2set(monitored_lpath_list),
        f = hook,
        prefix = prefix,
        associated_lpath_set = list2set(associated_lpath_list or { }) }

    -- Register the hook, so that we know when it must be triggered
    for _, lpath in ipairs (monitored_lpath_list) do
        local hooks_set = M.hooks[lpath]
        if hooks_set then hooks_set[hook] = true
        else M.hooks[lpath] = { [hook] = true } end
    end

    -- Register on handlers, so that they actually call M.notify upon changes.

    for _, lpath in pairs(monitored_lpath_list) do
        -- handlers above each lpath: only register the path toward this node
        log("TREEMGR", "DEBUG", "Register: search handlers affecting lpath %q", lpath)
        for lprefix, relpath in path.gsplit(lpath) do
            local handler_name, hprefix = db.l2h (lprefix)
            if handler_name then
                local handler = get_handler(handler_name)
                local hpath = path.concat(hprefix, relpath)
                log("TREEMGR", "DEBUG", "Register: Handler %s mounted at "..
                    "lpath %q above lpath %q registers hpath %q",
                    handler_name, lprefix, lpath, hpath)
                handler :register (hpath)
            end
        end
        -- handlers below each lpath: register for everything
        for handler_name, mount_path in db.l2m (lpath) do
            log("TREEMGR", "DEBUG", "Register: Handler %s mounted below "..
                "lpath %q registers hpath %s in handler %s", handler_name, lpath, mount_path, handler_name)
            get_handler(handler_name) :register (mount_path)
        end
    end
    return hook
end

--------------------------------------------------------------------------------
-- Cancel the registration of a hook.
-- Following a call to this function, the hook won't be notified of anything
-- anymore; if some handler registration can be cancelled as a result of this
-- logical deregistration, those deregistrations will be performed.
--
-- @param hook the hook to cancel, as returned by `register`.
--
function M.unregister(hook)

    checks('table') -- TODO: declare agent.treemgr.hook type?

    -- 1/ remove the hook's lpaths from `M.hooks`
    for lpath, _ in pairs(hook.monitored_lpath_set) do
        local hooks_set = M.hooks[lpath]
        hooks_set[hook] = nil
        if not next(hooks_set) then M.hooks[lpath] = nil end
    end

    -- 2/ look for other hooks registered to synonyms of the same lpath
    local collectable_hpaths = { }
    for lpath, _ in pairs(hook.monitored_lpath_set) do
        local l2h = lpath2hpath(lpath)
        --print("unregister: checking lpath "..lpath.."\nh2l = "..siprint(2,l2h))
        if l2h and l2h.handler.unregister then -- no use going further if we can't unregister
            local hpath = path.concat(l2h.hpath, l2h.relpath) -- hpath associated to lpath
            --print("unregister: checking if hpath is still monitored: "..hpath)
            local still_monitored = false
            for _, equiv_lpath in ipairs(hpath2lpath (l2h.handler_name, hpath)) do
                --print("unregister: mounted as lpath "..equiv_lpath)
                -- look whether lpath synonyms are monitored
                if M.hooks[equiv_lpath] then still_monitored=true; break end
            end
            if not still_monitored then
                collectable_hpaths[{ l2h.handler, hpath }] = true
            end
        end
    end

    -- 3/ perform the corresponding handler deregistrations
    for handler_hpath, _ in pairs (collectable_hpaths) do
        local handler, hpath = unpack (handler_hpath)
        handler :unregister (hpath)
    end

    return "ok"
end

--------------------------------------------------------------------------------
-- lpath -> hook -> true.
-- each hook in the set has the following fields:
--
--  * `monitored_lpath_set`:  set of lpaths which trigger this hook;
--  * `associated_lpath_set`: set of lpaths mandatory in the hook's argument;
--  * `f`: function to run at each triggering.
--
-- Hooks monitoring more than one lpath will be referenced more than once.
--
M.hooks = { }

--------------------------------------------------------------------------------
-- Take a variable change notification from a handler, trigger the
-- proper logical notifications on user-provided hooks.
--
-- @param handler the handler which triggers the notification
-- @param lmap the lpath->value map of changed variables.
--
function M.notify (handler_name, hmap)

    checks('string', 'table')
    local errors = nil
    local unhandled = false

    local lmap = { }
    for hpath, val in pairs (hmap) do
        local lpath_list = hpath2lpath(handler_name, hpath) -- TODO: cache these
        for _, lpath in ipairs(lpath_list) do lmap[lpath] = val end
    end
    
    -- List every hook which must be notified
    local notified_hooks = { } -- hook -> true
    
    for lpath, value in pairs(lmap) do
        for lprefix, _ in path.gsplit(lpath) do
            local hooks_set = M.hooks [lprefix]
            if hooks_set then
                -- TODO: cache lpath->monitored_lprefixes
                for hook, _ in pairs(hooks_set) do notified_hooks[hook]=true end
            end
        end 
    end

    for hook, _ in pairs(notified_hooks) do

        -- build the map #1: sort `lmap` subset relevant to this hook
        local hook_lmap = { }
        for llpath, val in pairs(lmap) do
            for lprefix, _ in path.gsplit(llpath) do -- listed as registered?
                if hook.monitored_lpath_set[lprefix] or hook.associated_lpath_set[lprefix] then
                    log('TREEMGR', 'DEBUG', "Notify: llpath %q needed because of lprefix %q", llpath, lprefix)
                    hook_lmap[llpath] = val
                    break -- to the next llpath/value pair
                end
            end
        end

        -- build the map #2: add missing lpaths for this hook
        -- Beware that `M.get` will ignore non-leaf associated paths.
        local associated_lmap = { }
        for lpath, _ in pairs (hook.associated_lpath_set) do
            if hook_lmap[lpath]==nil then
                log('TREEMGR', 'DEBUG', "Need to retrieve associated lpath %q", lpath)
                table.insert(associated_lmap, lpath)
            end
        end

        local _, children = M.get(hook.prefix, associated_lmap, hook_lmap)

        -- Check for erroneous non-leaf associated paths
        if children and next(children) then
            -- Some associated vars were non-leaf, log an error
            local non_llpaths = { }
            for _, lpath in ipairs(children) do
                local non_llpath, _ = path.split(lpath, -1)
                non_llpaths [non_llpath] = true
            end
            local list = table.concat(utils_table.keys(non_llpaths), ", ")
            log("TREEMGR", "ERROR", "Non-leaf associated path %s", list)
        end

        if log.musttrace('TREEMGR', 'DEBUG') then
            log('TREEMGR', 'DEBUG', "Notify hook with lmap %s", sprint(hook_lmap))
        end

        -- remove prefix path
        if hook.prefix then
	   local new_map = { }
            local keys = {}
            for key, value in pairs(hook_lmap) do
                assert(key:sub(1, #hook.prefix) == hook.prefix)
                local k = key:sub(#hook.prefix+2, -1)
                if k ~= key then 
                    new_map[k] = value
                end
            end
	    hook_lmap = new_map
        end
        local res, err = hook.f(hook_lmap)
        if res == 0 and err == "unhandled" then
            unhandled = true
        elseif res == 1 and type(err) == "string" then
	    if not errors then errors = {} end
            table.insert(errors, err)
        end
    end
    if errors then return nil, errors end
    if unhandled then return "unhandled" end
    return "ok"
end

return M
