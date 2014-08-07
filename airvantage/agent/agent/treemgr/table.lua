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

--- Treemgr proxy tables: access treemgr content as nested tables.
--  This module allows to read, write and iterate on treemgr values as if they
--  were nestes  tables. This is done through proxy objects with metatables
--  implementing `__index`, `__newindex`, `__pairs` and `__ipairs`.
--
--  @module treemgr.table

require 'sched'
local upath    = require 'utils.path'
local treemgr = require 'agent.treemgr'


--require 'print'

--local printf=function() end

local MT = {} -- proxy metatable
local pathtoken = newproxy()

--- Create a new proxy object, representing the treemgr path passed as parameter.
local function newtable(path)
    --printf("atmt: new proxy for %q", path)
    return setmetatable({[pathtoken]=path}, MT)
end

--- Retrieve a leaf node's value, or create and return a proxy for a non-leaf
--  node.
function MT  :__index(key)
    --printf("atmt: get key %s from proxy(%s)", sprint(key), sprint(self[pathtoken]))
    local path = upath.concat(self[pathtoken], key)
    local v, l = treemgr.get(nil, path)
    if v==nil and type(l)=='table' then
        return newtable(path) -- non-leaf path, return a proxy
    else return v end         -- leaf path, return value
end

--- Write a value in treemgr.
--- Attention: the write is asynchronous (because of the yield across metamethod boundaries limitation)
function MT  :__newindex(key, value)
    local path = upath.concat(self[pathtoken], key)
    sched.run(treemgr.set, path, value)
end

--- Iterate on every direct child of the proxy.
function MT  :__pairs()
    local path = self[pathtoken]
    local v, l = treemgr.get(nil, path)
    --printf("atmt: treemgr.get(%s) returns %s, %s",
    --    sprint(path), sprint(v), sprint(l))
    assert(v==nil and type(l)=='table', path.." is not a table")
    local i = 0
    return function()
        i = i+1
        local path = l[i] -- absolute path
        if path==nil then return nil end
        local _, key = upath.split(path, -1) -- relative path
        --printf("atmt: __pairs path=%s, key=%s", sprint(path), sprint(key))
        local val = self[key]
        return tonumber(key) or key, val
    end
end

--- Iterate over every numeric child of the proxy.
function MT  :__ipairs()
    local path = self[pathtoken]
    local v, l = treemgr.get(nil, path)
    assert(v==nil and type(l)=='table', path.." is not a table")
    local i = 0
    return function()
        i = i+1
        local v = self[i]
        if v==nil then return nil else return i, v end
    end
end

--- Return, as the resulting module, a proxy for treemgr's root node.
return newtable('')
