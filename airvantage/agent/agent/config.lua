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
--     Laurent Barthelemny for Sierra Wireless - initial API and implementation
--     Cuero Bugot         for Sierra Wireless - initial API and implementation
--     David Francois      for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require = require
local sched = require 'sched'
local persist = require 'persist'
local checks = require 'checks'
local upath = require 'utils.path'
local tableutils = require 'utils.table'
local loader = require 'utils.loader'
local niltoken = require 'niltoken'

local type = type
local pairs = pairs
local ipairs = ipairs
local next = next
local setmetatable = setmetatable
local assert = assert
local _G = _G
local string = string
local tostring = tostring


module(...)


local store = persist.table.new("ConfigStore") -- holds the config data, in a form key/value, where keys are the FQVN (complete path)
local cache -- cache table: holds the table structure of the store data




function loadcache()
    cache = {}
    for k, v in pairs(store) do
        upath.set(cache, k, v)
    end
    if not next(cache) then default() end -- make sure we have a default config !
end



function get(path)
    checks('string')

    local v = upath.get(cache, path)

    -- clone a table to be sure the cache will not be accessed from outside!
    if type(v) == 'table' then v = tableutils.copy(v) end

    return v
end

local function internalset(values)
    local towrite = {}

    for k, v in tableutils.sortedpairs(values) do -- make sure the order is defined
        local prev = upath.get(cache, k)

        -- Only do something if the value is different
        if niltoken(prev) ~= v then

            -- if the previous value was a table we are going to erase all its sub values at once!
            if type(prev) == 'table' then
                for sk in tableutils.recursivepairs(prev, k) do towrite[sk] = niltoken end
            end

            -- check if we are not overwriting a value along the path ! Only do that when the value is not nil.
            local t, p = upath.find(cache, k, "noowr")
            if p and p~=k then towrite[p] = niltoken end

            -- Finally mark the value to be written
            -- only mark it if it is non nil or if we are setting a leaf value
            if v ~= niltoken or type(prev) ~= 'table' then
                towrite[k] = v
            end

        -- make sure this value is not marked as cleared (niltoken)
        else
            towrite[k] = nil
        end
    end

    if next(towrite) then
        for k, v in tableutils.sortedpairs(towrite) do
            if k == "" then
                if v==niltoken then -- empty the whole config table
                    cache={ }
                    persist.table.empty(store)
                else
                    return nil, "cannot set root element of config"
                end
            else
                local tv = niltoken(v)
                upath.set(cache, k, tv)
                store[k] = tv
            end
        end
    end

    return towrite
end

function set(path, value, clear)
    checks('string')

    path = upath.clean(path) -- make sure the path is clean

    value = value==nil and niltoken or value

    if type(value) == 'table' then
        local values = {}
        if clear then values[path]=niltoken end -- erase all sub values
        for k, v in tableutils.recursivepairs(value, path) do values[k] = v end
        return internalset(values)
    else
        return internalset {[path] = value}
    end
end

-- restore the default configuration
function default(path)
    path = path or ""
    local def = upath.get(require 'agent.defaultconfig', path)
    loader.unload 'agent.defaultconfig'
    set(path, def, true)
end

-- returns the diff between current config and the default one
function diff(path)
    path = path or ""
    local def = upath.get(require 'agent.defaultconfig', path)
    loader.unload 'agent.defaultconfig'
    local cur = upath.get(cache, path)
    if type(cur) ~= 'table' then cur={[""] = cur} end
    if type(def) ~= 'table' then def={[""] = def} end

    return tableutils.diff(def, cur)
end

-- Pretty printer for the diff API
function pdiff(path)
    path = path or ""
    local print=_G.print
    local d = diff(path)
    if #d == 0 then
        print("No differences")
    else
        print(string.format("%-40s %-30s %-30s", "Config Key", "Default", "Current"))
        local def = require 'agent.defaultconfig'
        loader.unload 'agent.defaultconfig'
        path = path.."."
        if d then
            for _, v in ipairs(d) do
                v = upath.clean(path..v)
                local dv = upath.get(def, v)
                local cv = upath.get(cache, v)
                print(string.format("%-40s %-30s %-30s", v, tostring(dv), tostring(cv)))
            end
        end
    end
end

-- Excuted at module load time !
-- Load the structure cache
loadcache()


-- Make the module a direct access to the table
local c = require 'agent.treemgr.table'.config
return setmetatable(_M, { __index=c, __newindex=c })
