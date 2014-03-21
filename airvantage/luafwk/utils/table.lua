-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- Table utils.
--
-- @module utils.table
--


local coroutine = require"coroutine"
local cleanpath = require"utils.path".clean
local table = table

local M = { }
--public API
local pack, copy, diff, isarray, keys, map, sortedpairs, recursivepairs, multipairs


assert(not table.pack, "This was temporary, waiting for Lua5.2. Remove table.pack from utils module!")

--------------------------------------------------------------------------------
-- Packs a variable number of arguments into a table.
-- This is a temporary function until we switch to Lua 5.2.
--
-- Returns a new table with all parameters stored into keys 1, 2, etc. and with a field "n" with
-- the total number of parameters. Also returns, as a second result, the total number of parameters.
--
-- Note that the resulting table may not be a sequence.
--
-- Note that when loading utils.table module, this pack function is also added to global/std table module.
--
-- @function [parent=#utils.table] pack
-- @param varargs arguments to pack
-- @return a table that contains all the arguments followed by the number of arguments.
--

function pack(...)
    local n = select("#", ...)
    local t = {...}
    t.n = n
    return t, n
end
table.pack = pack --until we switch to 5.2.

--------------------------------------------------------------------------------
-- Copies a table from the source to destination table.
--
-- The copy is recursive.
--
-- @function [parent=#utils.table] copy
-- @param src a table to be used as the source of the copy.
-- @param dst a table to be used as the destination of the copy.
-- @param overwrite boolean value, to enable overwrite of existing field in dst table.
-- @return the resulting dst table.
--

function copy(src, dst, overwrite)
    dst = dst or {}
    for k, v in pairs(src) do
        if type(v) == 'table' then -- recursively go into tables
            if type(dst[k]) == 'table' then
                copy(v, dst[k], overwrite)
            elseif overwrite or dst[k]==nil then
                dst[k] = {}
                copy(v, dst[k], overwrite)
            end
        elseif overwrite or dst[k]==nil then dst[k] = v end
    end

    return dst
end

--------------------------------------------------------------------------------
-- Produces a diff of two tables.
--
-- Recursive diff by default.
--
-- @function [parent=#utils.table] diff
-- @param t1 first table to compare.
-- @param t2 second table to compare.
-- @param norecurse boolean value to disable recursive diff.
-- @return the diff result as a table.
-- @usage
--> t1 = { a = 1, b=3, c = "foo"}
--> t2 = { a = 1, b=3, c = "foo"}
--> :diff(t1, t2)
--= {  }
--> t1 = { a = 1, b=3, c = "foo"}
--> t2 = { a = 1, b=4, c = "foo"}
--> :diff(t1, t2)
--= { "b" }
--> t1 = { a = 1, b=3, c = "foo"}
--> t2 = { a = 1, b=3}
--> :diff(t1, t2)
--= { "c" }
--> t1 = { b=3, c = "foo", d="bar"}
--> t2 = { a = 1, b=3, c = "foo"}
--> :diff(t1, t2)
--= {
--   "a",
--   "d" }
--

function diff(t1, t2, norecurse)
    local d = {}
    local t3 = {}

    local rpairs = norecurse and pairs or recursivepairs
    for k, v in rpairs(t1) do t3[k] = v end
    for k, v in rpairs(t2) do
        if v ~= t3[k] then table.insert(d, k) end
        t3[k] = nil
    end

    for k, v in pairs(t3) do table.insert(d, k) end

    return d
end

--------------------------------------------------------------------------------
-- Checks if the table is regular array.
--
-- Regular array is a table with only conscutive integer keys (no holes).
--
-- @function [parent=#utils.table] isArray
-- @param T table to check.
-- @return true or false.
--

function isarray(T)
    local n = 1
    for k, v in pairs(T) do
        if T[n] == nil then return false end
        n = n + 1
    end
    return true
end

--------------------------------------------------------------------------------
-- Extracts the keys of a table into a list.
--
-- @function [parent=#utils.table] keys
-- @param T the table to extract keys from.
-- @return a list of keys of T table.
--

function keys(T)
    local list = {}
    for k in pairs(T) do table.insert(list, k) end
    return list
end


--------------------------------------------------------------------------------
-- Executes a function on each element of a table.
--
-- If recursive is non false the map function will be called recursively.
--
-- If inplace is set to true, then the table is modified in place.
--
-- The function is called with key and value as a parameter.
--
-- @function [parent=#utils.table] map
-- @param T the table to apply map on.
-- @param func the function to call for each value of the map.
-- @param recursive if non false, the map is executed recusively.
-- @param inplace if non false, the table is modified in place.
-- @return the table with each value mapped by the function
-- @usage local data = { key1 = "val1", key2 = "val2"}
-- local function foo(key, value) print("foo:" key, value) end
-- map(data, foo)
-- -> will print:
-- foo:    key1        val1
-- foo:    key2        val2
--

function map(T, func, recursive, inplace)
    local newtbl = inplace and T or {}
    for k,v in pairs(T) do
        if recursive and type(v) == 'table' then newtbl[k] = map(v, func, true, inplace)
        else newtbl[k] = func(k, v) end
    end
    return newtbl
end


--- Iterators
-- @section It

--------------------------------------------------------------------------------
-- Iterator that iterates on the table in key ascending order.
--
-- @function [parent=#utils.table] sortedPairs
-- @param t table to iterate.
-- @return iterator function.
--

function sortedpairs(t)
    local a = {}
    local insert = table.insert
    for n in pairs(t) do insert(a, n) end
    table.sort(a)
    local i = 0
    return function()
        i = i + 1
        return a[i], t[a[i]]
    end
end

--------------------------------------------------------------------------------
-- Iterator that returns key/value pairs except walking through sub tables and concatenating the path in the key.
--
-- This iterator break cycles: it does not recur. If it detects a cycle, the entry causing a cycle is ignored.
-- However, if a table is repeated, its contents will be repeated by this function.
--
-- @function [parent=#utils.table] recursivePairs
-- @param t table to iterate.
-- @param prefix path prefix to prepend to the key path returned.
-- @return iterator function.
-- @usage {toto={titi=1, tutu=2}, tata = 3, tonton={4, 5}} will iterate through
-- ("toto.titi",1), ("toto.tutu",2), ("tata",3) ("tonton.1", 4), ("tonton.2"=5)
--

function recursivepairs(t, prefix)
    checks('table', '?string')
    local function it(t, prefix, cp)
        cp[t] = true
        local pp = prefix == "" and prefix or "."
        for k, v in pairs(t) do
            k = pp..tostring(k)
            if type(v) == 'table' then
                if not cp[v] then it(v, prefix..k, cp) end
            else
                coroutine.yield(prefix..k, v)
            end
        end
        cp[t] = nil
    end

    prefix = prefix or ""
    return coroutine.wrap(function() it(t, cleanpath(prefix), {}) end)
end

--------------------------------------------------------------------------------
-- Iterator that returns key/value on multiple tables.
--
-- @function [parent=#utils.map] multiPairs
-- Tables are traversed in the same order as they are passed as parameters.
-- @param t1 first table to traverse.
-- @param ... additional tables to traverse.
-- @return iterator function.
--

function multipairs(t1, ...)
    local state = { tables = {t1, ...}, i=1, k=next(t1) }
    local function f()
        if not state.i then return end
        local current_table = state.tables[state.i]
        local k, v = state.k, current_table[state.k]
        local next_k = next(current_table, k)
        while next_k==nil do
            state.i = state.i+1
            local next_table = state.tables [state.i]
            if not next_table then state.i=false; break
            else next_k = next (next_table) end
        end
        state.k = next_k
        return k, current_table[k]
    end
    return f, nil, nil
end

--public API
M.pack=pack; M.copy=copy; M.diff=diff; M.keys=keys; M.map=map;
M.isarray=isarray; M.isArray=isarray;
M.sortedpairs=sortedpairs; M.sortedPairs=sortedpairs;
M.recursivepairs=recursivepairs; M.recursivePairs=recursivepairs;
M.multipairs=multipairs; M.multiPairs=multipairs;

return M
