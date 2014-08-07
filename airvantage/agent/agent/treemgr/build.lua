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
-- Compile .map files, describing a mapping from treemgr handlers to
--  the global logical tree, into the databases used by treemgr.
--
-- File format
-- -----------
--
-- A map file consists of a first line
-- `"treemgr <module_name>\n"`, where `module_name` is both the name used in the
-- databases, and the name of the Lua module which implements this handler.
--
-- Each subsequent line if of the form
-- `"<logical_path>=<handler_path>\n"`.
--
-- Finally, comments starting with `"#"` characters as well as empty
-- lines are allowed, except on the first line.
--
-- Usage
-- -----
--
-- The databases are erased and set ready for writing with `dbset =
-- init(file_prefix)`.  Each map file is then processed with
-- `add(dbset, filename)`, and the writing operation is finalized by
-- called `close(dbset)

local cdb_make = require 'cdb_make'
local path     = require 'utils.path' -- path.split, path.gsplit
local log      = require 'log'


local MT = { }; MT.__index=MT

local DB_NAMES = { 'h2l', 'l2h', 'l2c', 'l2m' }

--------------------------------------------------------------------------------
-- Create a new databases set. Any preexisting database will be destroyed.
--
-- @param file_prefix the path and the stem of the database files. For
--        instance, a prefix of `"/tmp/treemgr"` will lead to the
--        creation of `/tmp/treemgr.h2l.cdb, /tmp/treemgr.l2h.cdb`,
--        etc.
--
-- @return a dbset table, to be passed as argument to `add` and `close`.
--
local function newmapper (file_path)
    if file_path :sub(-1, -1) ~= '/' then file_path = file_path .. '/' end
    local db_prefix = 'treemgr'
    local instance  = { cache = { l2c={ }; l2m={ }; lpaths={ } } }
    for _, db_name in ipairs(DB_NAMES) do
           local stem = file_path..db_prefix..'.'..db_name
        local db = cdb_make.start(stem..'.cdb', stem..'.tmp.cdb')
        assert (db, "Cannot create tree manager databases")
        instance[db_name] = db
    end

    local b = assert(io.open(file_path.."rebuild.lua", 'w'))
    instance.builder = b
    b :write ("-- This generated script rebuilds the databases from the .map file.\n\n")
    b :write ("local B = require('agent.treemgr.build')('"..file_path.."')\n")

    local l = assert(io.open(file_path.."load.lua", 'w'))
    instance.loader = l
    l :write ("-- This generated script loads all handlers needed by treemgr accordin to the map.\n\n")
    l :write 'local H = { }\n'

    log('TREEMGR-BUILD', 'DETAIL', "Compiling databases in %s.*.cdb", file_path)
    return setmetatable(instance, MT)
end

--------------------------------------------------------------------------------
-- Add the mappings described in the file to the databases.
-- @param map_filename mappings to add to the databases.
function MT :add (map_filename)
    local input = assert(io.open(map_filename, 'r'), "map file "..
        map_filename.." not found")
    local first_line = input :read '*l'
    local name = first_line :match "^treemgr%s+([%S]+)$"

    assert (name, "Bad map file format")

    log('TREEMGR-BUILD', 'DETAIL', "Begin of mappings from %s to databases", map_filename)
    local i=2
    for line in input :lines() do
        if not line :match "^%s*#" -- skip comment lines
        and not line :match "^%s*$" then -- skip empty lines
            local lpath, hpath = line :match ("([^=%s]+)%s*=%s*([^#]*)")
            --printf ("parsing %s->%s", lpath, hpath)
            if not lpath then
                input :close()
                error("Bad line #"..i.." in file "..map_filename)
            end
            log('TREEMGR-BUILD', 'DEBUG', "add line to l/h: L '%s' <-> H '%s:%s'.", lpath, name, hpath)
            local name_hpath = name..":"..hpath
            self.l2h :add (lpath, name_hpath)
            self.h2l :add (name_hpath, lpath)
            if self.cache.lpaths[lpath] then
                error("Attempt to mount lpath "..lpath.." several times: as "..
                    name_hpath.." and "..self.cache.lpaths[lpath])
            end
            self.cache.lpaths[lpath] = name_hpath
            for child, _ in path.gsplit(lpath) do
                local parent, _ = path.split(child, -1)
                local key_l2c = parent .. '->' .. child
                local key_l2m = child  .. '->' .. name_hpath
                if child ~= '' and not self.cache.l2c[key_l2c] then
                    log('TREEMGR-BUILD', 'DEBUG', "    add to l2c: Parent '%s' -> Child '%s'.", parent, child)
                    self.l2c :add (parent, child)
                    self.cache.l2c[key_l2c] = true
                end
                if not self.cache.l2m[key_l2m] then
                    log('TREEMGR-BUILD', 'DEBUG', "    add to l2m: L '%s' -> Module '%s'.", child, name_hpath)
                    self.l2m :add (child, name_hpath)
                    self.cache.l2m[key_l2m] = true
                end
            end
        end
        i=i+1
    end
    log('TREEMGR-BUILD', 'DETAIL', "End of mappings from %s to databases", map_filename)
    input :close()
    self.loader :write ("H['"..name.."'] = require '"..name.."'\n")
    self.builder :write("B :add '"..map_filename.."'\n")
   return self
end

--------------------------------------------------------------------------------
-- Finalize the database writing operation.
function MT :close()
    self.loader  :write 'return H\n'
    self.builder :write 'B :close()\n'
    self.loader  :close()
    self.builder :close()
    for _, db_name in ipairs(DB_NAMES) do self[db_name] :finish() end
    log('TREEMGR-BUILD', 'DETAIL', "Database compilation finished.")
end

return newmapper
