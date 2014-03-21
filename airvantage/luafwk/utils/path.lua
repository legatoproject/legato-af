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
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- Path utils.
--
-- **Definition of path** used by this file:
--
-- - it is **not Lua path**, Lua path meaning the way Lua provide table indexing.
--     - especially, **'table[ ]'** notation is not supported
-- - **element separator is  '.'** (dot)
-- - provided **path is always cleaned** before being processed:
--     - if the path starts and/or ends with one or several dots, there are automatically removed.<br />
--       e.g.: `'...toto.tutu....'` means `'toto.tutu'`
--     - internal repetitions of successives dots are replaced by a single dot.<br />
--       e.g.: `'toto...tutu.tata..foo.bar'` means `'toto.tutu.tata.foo.bar'`
-- - if a path element can be **converted to a number**, then it is returned as a number,
--   otherwise all elements or paths are returned as strings.<br />
--   e.g.: `split('a.2', 1)` -> `('a', 2)`   (where 2 is returned as a number) <br />
--   e.g.: `split('a.b.2e3.c', 1)` -> `('a', 'b.2000.c')`
--
-- @module utils.path
--

local checks = require"checks"




local M = { }
--public API
local split,clean,segments, get, set, gsplit, concat, find

local function pathconcat(pt, starti, endi)
    local t = {}
    local prev
    local empties = 0
    starti = starti or 1
    endi = endi or #pt

    for i = starti, endi do
        local v = pt[i]
        if not v then break
        elseif v == '' then
            empties = empties+1
        else
            table.insert(t, prev)
            prev = v
        end
    end
    table.insert(t, prev)
    --log('PATH', 'INFO', "pathconcat(%s, %d, %d) generates table %s, wants indexes %d->%d",
    --    sprint(pt), starti, endi, sprint(t), 1, endi-starti+1-empties)
    return table.concat(t, '.', 1, endi-starti+1-empties)
end

--------------------------------------------------------------------------------
-- Concatenates a sequence of path strings together.
--
-- @function [parent=#utils.path] concat
-- @param varargs list of strings to concatenate into a valid path
-- @return resulting path as a string
--

function concat(...)
    return pathconcat({...})
end

--------------------------------------------------------------------------------
-- Cleans a path.
--
-- Removes trailing/preceding/doubling '.'.
--
-- @function [parent=#utils.path] clean
-- @param path string containing the path to clean.
-- @return cleaned path as a string.
--

function clean(path)
    checks('string')
    local p = segments(path)
    return pathconcat(p)
end

--------------------------------------------------------------------------------
-- Sets a value in a tree-like table structure.
--
-- The value to set is indicated by the path relative to the table.
-- This function creates the table structure to store the value, unless the value to set is nil.
-- If the value to set is nil and the table structure already exists then the value is set to nil.
-- If the value is not nil, then the table structure is always created/overwritten and the value set.
--
-- @function [parent=#utils.path] set
-- @param t table where to set the value.
-- @param path can be either a string (see @{#(utils.path).split})
--  or an array where path[1] is the root and path[n] is the leaf.
-- @param value the value to set.
--

function set(t, path, value)
    checks('table', 'string|table', '?')
    local p = type(path)=='string' and segments(path) or path
    local k = table.remove(p)
    local t = find(t, p, value~=nil) -- only create the table structure if the value to set is non nil!
--    local t = findtable(t, p, true) -- for the creation of the table structure, even if the value to set is non nil!
    if t then t[k] = value end
end

--------------------------------------------------------------------------------
-- Gets the value of a table field. The field can be in a sub table.
--
-- The field to get is indicated by a path relative to the table.
--
-- @function [parent=#utils.path] get
-- @param t table where to get the value.
-- @param path can be either a string (see @{split}) or an array where path[1] is the root and path[n] is the leaf.
-- @return value if the field is found.
-- @return nil otherwise.
--
function get(t, path)
    checks('table', 'string|table')
    local p = type(path)=='string' and segments(path) or path
    local k = table.remove(p)
    if not k then return t end
    local t = find(t, p)
    return t and t[k]
end


--------------------------------------------------------------------------------
-- Enumerates path partitions in a for-loop generator, starting from the right.
--
-- For instance, `gsplit "a.b.c"` will generate successively
-- `("a.b.c", ""), ("a.b", "c"), ("a", "b.c"), ("", "a.b.c")`.
--
-- @function [parent=#utils.path] gsplit
-- @param path the path as a string
-- @return the for-loop iterator function
--

function gsplit (path)
    checks ('string')
    local segs  = segments(path)
    local nsegs = #segs
    local limit = nsegs
    local function f()
        if limit == -1 then return nil, nil end
        local a, b = pathconcat(segs, 1, limit), pathconcat(segs, limit+1, nsegs)
        limit = limit - 1
        return a, b
    end
    return f
end

--------------------------------------------------------------------------------
-- Splits a path into two halves, can be used to get path root, tail etc.
--
-- The content of the two halves depends on 'n' param: there will be `n` segments in the first half if `n>0`,
-- `-n` segments in the second half if `n<0`.
--
-- If there are less then `n` segments, returns the path argument followed by
-- an empty path.
--
-- If there are less then `-n` segments, returns an empty path followed by the
-- path argument.
--
-- Note that if a half is a single element and that this element can be converted into a number,
-- it is returned as a number.
--
-- @function [parent=#utils.path] split
-- @param path the path as a string
-- @param n number defining how the path is splitted (see above description).
-- @return the two halves
-- @usage local root, tail = split('a.b.c', 1)
-- ->root contains 'a', tail contains 'b.c'
--

function split(path, n)
    checks('string', 'number')
    local segments = segments(path)
    if      n>#segments then return path, ''
    elseif -n>#segments then return '', path
    else
        if n<0 then n=#segments+n end
        return pathconcat(segments, 1, n), pathconcat(segments, n+1, #segments)
    end
end

--------------------------------------------------------------------------------
-- Splits a path into segments.
--
-- Each segment is delimited by '.' pattern.
--
-- @function [parent=#utils.path] segments
-- @param path string containing the path to split.
-- @return list of split path elements.
--

function segments(path)
    checks('string')
    local t = {}
    local index, newindex, elt = 1
    repeat
        newindex = path:find(".", index, true) or #path+1 --last round
        elt = path:sub(index, newindex-1)
        elt = tonumber(elt) or elt
        if elt and elt ~= "" then table.insert(t, elt) end
        index = newindex+1
    until newindex==#path+1
    return t
end

--------------------------------------------------------------------------------
-- Retrieves the element in a sub-table corresponding to the path.
--
-- @function [parent=#utils.path] find
-- @param t is the table to look into.
-- @param path can be either a string (see @{segments}) or an array where
-- `path[1]` is the root and `path[n]` is the leaf.
-- @param force parameter allows to create intermediate tables as specified
--  by the path, if necessary.
-- @return returned values depend on force value:
--
-- * if force is false (or nil), find returns the table if it finds one,
--   or it returns nil followed by the subpath that points to non table value
--
-- * if force is true, find overwrites or create tables as necessary so
--   it always returns a table.
--
-- * if force is 'noowr', find creates tables as necessary but does not
--   overwrite non-table values. So as with `force=false`, it only returns a
--   table if possible, and nil followed by the path that points to the first
--   neither-table-nor-nil value otherwise.
--
-- @usage config = {toto={titi={tutu = 5}}}
--     find(config, "toto.titi") -- will return the table titi
--
function find(t, path, force)
    checks('table', 'string|table', '?')
    path = type(path)=="string" and segments(path) or path
    for i, n in ipairs(path) do
        local v  = t[n]
        if type(v) ~= "table" then
            if not force or (force=="noowr" and v~=nil) then return nil, pathconcat(path, 1, i)
            else v = {} t[n] = v end
        end
        t = v
    end
    return t
end

--public API
M.split=split; M.clean=clean; M.segments=segments; M.get=get; M.set=set;
M.gsplit=gsplit; M.concat=concat; M.find=find

return M
