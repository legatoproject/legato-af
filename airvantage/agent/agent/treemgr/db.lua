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

--------------------------------------------------------------------------------
-- Interface to the CDB databases generated from the .map file.
--
-- Database format
-- ---------------
--
-- The database must allow to easily retrieve `lpath->hpath`
-- translations, as well as `hpath->lpath_list` ones (handler nodes
-- can be mounted in more than one place).
--
-- CDB essentially associate an arbitrary value string to an arbitrary
-- key string. We will exploit the fact that node names don't include
-- punctuations, and concatenate paths by separating them with
-- punctuation characters.
--
-- The first DB `l2h` describes mountpoints: it associates the mountpoint's
-- `lpath` to the handler and the mounted `hpath`, separated by a
-- colon character `":"`.
--
-- The second DB `h2l` describes the inverse relation: it associates a
-- handler and a `hpath`, separated by a colon, to a sequence of every
-- `lpath` which mount it. This allows to translate handler
-- notifications into logical ones.
--
-- The third DB `l2c` allows children enumeration. It associates
-- each `lpath` to the list of every child node directly below it.
--
-- The fourth DB `l2m` lists every handler mounted under every node, directly
-- or not. It has the form `lpath -> handler_name`. This means that
-- mount points below the logical tree root are indexed more than once.
-- For instance, if `"handler1"` is mounted under logical path
-- `"a.b"`, then it will be listed under keys `""`, `"a"` and `"a.b"`.
--
-- @module treequery.db

local cdb = require 'cdb'
local lfs = require 'lfs'

local treemgr_path = (LUA_AF_RW_PATH or "./").."persist/treemgr/"
os.execute("mkdir -p "..treemgr_path)

local resources_path = (LUA_AF_RO_PATH or "./").."resources/"

local M = { }

M.databases = { }

local h2l_db
local l2h_db
local l2c_db
local l2m_db

-- Returns a list of all filenames containing map definitions.
local function get_all_map_names()
    local maps = { }
    local gdir, msg = lfs.dir (resources_path)
    if not gdir then
        log('TREEMGR-DB', 'ERROR', "Cannot find map folder %s", treemgr_path)
        return nil, msg
    end
    for m in gdir do
        if m :sub(-4, -1) == '.map' then
            table.insert (maps, resources_path..m)
        end
    end
    return maps
end

-- Returns true iff the maps are more recent than the compiled DB
local function check_map_dates()
    local att = lfs.attributes(treemgr_path.."load.lua")

    if not att then
        log('TREEMGR-DB', 'DEBUG', "Need to create the treemgr databases, no compiled file")
        return true
    end

    local compilation_date = att.modification

    for _, name in ipairs(get_all_map_names()) do
        local map_date = lfs.attributes(name).modification
        if not map_date or map_date > compilation_date then
            log('TREEMGR-DB', 'INFO', "Need to rebuild the treemgr databases, %s (%d) has changed since last compilation (%d)",
                name, map_date, compilation_date)
            return true
        end
    end
    log('TREEMGR-DB', 'DETAIL', "DB files already compiled from maps")
    return false
end

-- rebuild DB files if map files are more recent
local function rebuild_db_if_needed()
    if check_map_dates() then -- need to rebuild
        local b = require 'agent.treemgr.build' (treemgr_path)
        for _, filename in ipairs(get_all_map_names()) do
            b :add (filename)
        end
        b :close()
    end
end

local initialized = false

--------------------------------------------------------------------------------
-- Load every handler referenced by the mapping databases
function M.init()

    rebuild_db_if_needed()

    -- Open databases
    h2l_db = cdb.init (treemgr_path..'treemgr.h2l.cdb'); M.databases.h2l = h2l_db
    l2h_db = cdb.init (treemgr_path..'treemgr.l2h.cdb'); M.databases.l2h = l2h_db
    l2c_db = cdb.init (treemgr_path..'treemgr.l2c.cdb'); M.databases.l2c = l2c_db
    l2m_db = cdb.init (treemgr_path..'treemgr.l2m.cdb'); M.databases.l2m = l2m_db
    initialized = true
end

--------------------------------------------------------------------------------
-- Translate a handler path into a list of logical paths.
-- The handler path must be a mount point; for a translation working
-- on arbitrary nodes, @see treemgr.hpath2lpath.
-- @param handler_name
-- @param hpath
-- @return a for-loop iterator listing every lpath associated to this hpath
--
function M.h2l(handler_name, hpath)
    if not initialized then M.init() end
    return h2l_db :values(handler_name..":"..hpath)
end

--------------------------------------------------------------------------------
-- Translate a logical path into the handler path mounted on it;
-- return nil if no handler is mounted there.
--
-- @return handler_name, hpath
--
function M.l2h(lpath)
    if not initialized then M.init() end
    local line = l2h_db :values (lpath) ()
    if line then return line :match "^([^:]+):(.*)$" end
end

--------------------------------------------------------------------------------
-- Translate a logical node into an enumeration of it direct children's path
--
-- @return a for-loop iterator listing the absolute logical path to
-- each direct child of `lpath`.
--
function M.l2c(lpath)
    if not initialized then M.init() end
    return  l2c_db :values (lpath)
end


--------------------------------------------------------------------------------
-- Translate a logical node into an enumeration of every handler/hpath combo
-- mounted below it, directly or indirectly.
--
-- @return a for-loop iterator listing the names of every handler
-- mounted below this logical node.
--
function M.l2m(lpath)
    if not initialized then M.init() end
    local raw_enumerator = l2m_db :values (lpath)
    return function()
        local line = raw_enumerator()
        if not line then return nil
        else return line:match "^([^:]+):(.*)$" end
    end
end

return M

