-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- This module allows to save and retrieve Lua values in table-like objects, in
-- a non-volatile way. Once stored in a persisted table, these Lua values can
-- be retrieved even after the Lua process and/or the CPU running it have been
-- rebooted.
--
-- Compared to raw files, the `persist` module offers a higher level of
-- abstraction, allowing to save and retrieve Lua objects directly, without
-- explicitly dealing with serialization, deserialization or other filesystem
-- issues. It can also be ported to environmnets which don't have a filesystem.
--
-- This version of the module naively writes data in a file, and keeps whole
-- tables' content in RAM. Its purpose is to avoid using the more efficient QDBM
-- version, whose LGPL license might be problematic to some use cases.
--
-- Persistence services are offered through two public APIs:
--
-- * general purpose persisted tables: they behave mostly as regular tables,
--   except that their content survives across reboots. These tables are
--   created with @{#table.new};
--
-- * To easily save and retrieve isolated objects, @{#persist.save} and
--   @{#persist.load} allow single-line operations with no extra bookkeeping.
--
-- Persisted Tables.
-- -----------------
--
-- Persisted tables behave mostly as regular Lua tables, except that their
-- content survives across reboots. They can hold strings, numbers, booleans,
-- and possibly nested tables thereof, both as their keys and as their values.
--
-- Functions can be persisted if and only if they don't capture any upvalue
-- (see examples below).
--
-- Beware that tables are stored structurally; what's retrieved are copies of
-- the objects store in them, not the objects themselves:
--
--     local persist = require 'persist'
--     t = persist.table.new('test')
--     t[1] = { }
--     assert(t[1] ~= t[1])
--
-- However, loops and shared table parts are preserved within a single
-- item (key or value) stored and retrieved from a persisted table:
--
--     local y  = { }
--     local x1 = { y1=y; y2=y } -- y1 and y2 point to the same object
--     x1.x = x                  -- x1 points to itself through its field x
--     assert(x1.x == x1)
--     assert(x1.y1 == x1.y2)
--     t[2] = x1                 -- save it in a table
--     local x2 = t[2]           -- retrieve a copy from the table
--     assert(x2 ~= x1)          -- it's a copy, not the original
--     assert(x2.x == x2)        -- but it points to itself
--     assert(x2.y1 == x2.y2)    -- and its shared parts are still shared
--
-- Since tables retrieved from persisted tables are actually copies, alterations
-- of these returned tables won't affect the store's content:
--
--     x = { foo = 'bar' }
--     t.x = x
--     x.foo = 42 -- modifying `x`, not the copy in `t`
--     assert (t.x.foo == 'bar') -- `t` remains unchanged
--     t.x = x -- overriding `t.x` with the whole `x` value
--     assert (t.x.foo == 42) -- now `t` reflects the change
--
-- "Simple" function, which don't capture any local variable, can be saved:
--
--     function plus_one(x) return x+1 end
--     t.plus_one = plus_one
--     assert(t.plus_one(1) == 2)
--
-- As for tables, persisted functions don't retain their identity:
--
--     assert(t.plus_one ~= t.plus_one)
--
-- Finally, functions which capture local variables cannot be saved. Below,
-- the alternative implementation of `plus_one` captures a local variable `i`,
-- and won't be properly serialized:
--
--     function make_incrementer (i)
--         return function(x) return x+i end
--     end
--     plus_one = make_incrementer(1)
--     t.plus_one = plus_one
--
-- Persisted Objects.
-- ------------------
--
-- The usual way to persist data with the `persist` module is to create a table
-- object, then fill it with values to persist. However, this is neither
-- practical nor efficient when only one or a couple of small values need to be
-- saved.
--
-- To avoid a needless proliferation of tiny persisted tables, a pair of
-- `persist.save` and `persist.load` functions are provided, to easily store
-- individual objects with a single line of code.
--
-- Objects stored through this API have the same limitations as those stored
-- in full-featured persisted tables: no userdata, no threads, no upvalues in
-- functions, no preservation of table and function identities.
--
--
-- POSIX implementation details.
-- -----------------------------
--
-- All objects are saved in a SQLITE3 database file called
-- persist/persist.sqlite3.
--
-- @module persist
--

local l2b      = require 'luatobin'
local checks   = require 'checks'
local log      = require 'log'

require "pack"

if global then global 'WORKING_DIR' end

if global then global 'LUA_AF_RW_PATH' end
local persist_path = (LUA_AF_RW_PATH or "./").."persist/"
--force using non-sched aware os.execute function to avoid "cross boundaries" issues
local exec = os.execute_orig or os.execute
exec("mkdir -p "..persist_path)

local M = { }

-- recompaction will be triggered when there will be more lost keys than
-- used one in the file store, and the number of lost key is more than 256.
M.MAX_LOSS_RATIO = 1
M.MIN_LOSS = 256

-- Cache to share multiple instances of the same table
local cache = setmetatable({},{__mode = "v"})

-- initialize the whole persist module
function M.init()
    if M.initialized then return 'already initialized' end
    M.initialized = true
    return 'ok'
end

------------------------------------------------------------------------------
-- The table sub-module of persist
-- @type table
--


------------------------------------------------------------------------------
-- The table sub-module of persist
-- @field [parent=#persist] #table table
--

M.table = { } -- sub-module persist.table

-- Remove useless entries from a table
local function recompact(self)
    log('PERSIST-FILE', 'DEBUG',
        "%d entries wasted for %d entries, recompacting table %s",
        self.__overridden, self.__length, self.__id)
    local file = io.open(persist_path..self.__id..'.l2b', 'wb') -- overwrite
    for k, v in pairs(self.__cache) do
        file :write(l2b.serialize(k))
        file :write(l2b.serialize(v))
    end
    self.__file :flush()
    self.__file = file
    self.__overridden = 0
end

local TABLE_MT = { __type = 'persist.file' }

-- Sets or deletes keys/values in DB
function TABLE_MT :__newindex (k, v)
    --printf("Writing table %d (deserialized): [%s]=%s", self.__id, sprint(k), sprint(v))
    if self.__cache[k]
    then self.__overridden = self.__overridden + 1
    else self.__length = self.__length + 1 end
    self.__cache[k] = v
    if self.__overridden > M.MIN_LOSS
    and self.__overridden/self.__length > M.MAX_LOSS_RATIO then
        recompact(self)
    else
        log('PERSIST-FILE', 'DEBUG', "wrote %s=%s", tostring(k), tostring(v))
        local sk = l2b.serialize(k)
        self.__file :write(sk)
        local sv = l2b.serialize(v)
        self.__file :write(sv)
        self.__file :flush()
    end
end

-- Retrieves values from DB
function TABLE_MT :__index(k)
    return self.__cache[k]
end

function TABLE_MT:__pairs()
    return pairs(self.__cache)
end

function TABLE_MT:__ipairs(...)
    return ipairs(self.__cache)
end

------------------------------------------------------------------------------
-- Creates or loads a new persisted table.
--
-- If a table already exists with the provided name, it is loaded; otherwise,
-- a new one is created.
--
-- @function [parent=#table] new
-- @param name persisted table name.
-- @return the persisted table on success.
-- @return `nil` + error message otherwise.
--

function M.table.new(name)
    checks('string')

    M.init()

    local cached = cache[name]
    if cached then return cached end

    local filename = persist_path .. name .. '.l2b'
    local file, errmsg = io.open(filename, 'rb')
    local filecontent
    if file then -- file existed
        log('PERSIST-FILE', 'DEBUG', "Loading %s from file %s",
            name, filename)
        filecontent = file :read '*a'
        file :close()
    end
    file, errmsg = io.open(filename, 'ab') -- append binary
    if not file then
        log('PERSIST-FILE', 'ERROR', "Can't open persistence file for writing: %q", errmsg)
        return nil, errmsg
    end

    local self = {
        __id         = name,
        __cache      = { },
        __overridden = 0,
        __length     = 0,
        __file       = file }

    cache[name] = self

    if filecontent then
        local offset=1
        while true do
            local k, v
            offset, k, v = l2b.deserialize(filecontent, 2, offset)
            if k==nil then break end
            if self.__cache[k] then
                self.__overridden = self.__overridden+1
            else self.__length = self.__length + 1 end
            self.__cache[k] = v
        end
    end
    setmetatable(self, TABLE_MT)
    return self
end

------------------------------------------------------------------------------
-- Empties a table and releases associateed resources.
--
-- @function [parent=#table] empty
-- @param t persited table returned by @{#table.new} call.
--

function M.table.empty(self)
    self.__length = 0
    self.__overridden = 0
    self.__file :close()
    self.__file = assert(io.open(persist_path..self.__id..'.l2b', 'wb')); -- truncate file to 0
    self.__cache = { }
end

local store = assert(M.table.new("PersistStore"))


--- Resets all persist tables:
-- this function is not to be mistaken with persist.table.empty API
-- this function resets all persisted files: all tables explicitly created by user, and the PersistStore used to provide load/save API in persist module.
-- (This data rest only applies to the current Lua framework running persist module).
function M.table.emptyall()
    log("PERSIST", "WARNING", "All persist files will be reset")
    for k, v in pairs(cache) do
        M.table.empty(v)
    end

    M.table.empty(store)
end


------------------------------------------------------------------------------
-- Saves an object for later retrieval.
--
-- If the saving operation cannot be performed successfully, an error is thrown.
-- Objects saved with this function can be retrieved with @{#persist.load}, by
-- giving back the same name, even after a reboot.
--
-- @function [parent=#persist] save
-- @param name the name of the persisted object to save.
-- @param obj the object to persist.
-- @usage
--
-- persist.save ('xxx', 1357) -- Save it in the store as 'xxx'
-- [...] -- reboot
-- x = persist.load 'xxx' -- Retrieve it from the store
-- assert(x == 1357)
--

function M.save(name, obj)
    checks('string', '?')
    store[name] = obj
end

------------------------------------------------------------------------------
-- Retrieve from flash an object saved with @{#persist.save}.
--
-- @function [parent=#persist] load
-- @param name the name of the persisted object to load.
-- @return the object stored under that name, or `nil` if no such object exists.
--

function M.load(name)
    checks('string')
    return store[name]
end

return M