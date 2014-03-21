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

--- Simple handler example.
--
-- This module demonstrates how to implement a handler for the tree
-- manager system. It stores data in RAM, retrieves it from there, and
-- notifies the tree manager whenever a monitored variable is modified.
--

local hname   = ...
local treemgr = require 'agent.treemgr'
local path    = require 'utils.path'

--- Handler metatable
local MT = { }; MT.__index=MT

--- Return either the value stored under this path,
--  or nil followed by the list of this path's children.
function MT :get(hpath)
    local v = self.values[hpath]
    if v then return v end
    return nil, self.nlnodes[hpath]
end

--- Store this set of path/value pairs, notify a change if appropriate.
--  The paths are used as keys in `self.values`. If a path, or one of
--  its prefixes, is registered for monitoring, then a notification is
--  sent to the tree manager.
--
-- @param hmap a flat table of path/value pairs
function MT :set(hmap)
    local changesset = {}
    local function set_hpath(hpath, val)
        if self.values[hpath]==val then return end -- no change, no notification
        local hchild, _
        for hprefix in path.gsplit(hpath) do
            -- keep track of parent->children relationship
            if hchild then
                local c = self.nlnodes[hprefix]
                if c then c[hchild] = true
                else self.nlnodes[hprefix] = { [hchild] = true } end
            end   
            _, hchild = path.split(hprefix, -1) -- for the next iteration

            -- check whether a hook needs to be notified
            if self.registered[hprefix] then
	        table.insert(changesset, hpath)
            end
        end
        self.nlnodes[hpath] = nil
        self.values[hpath] = val
    end
    for hpath, val in pairs(hmap) do set_hpath(hpath, val) end
    return changesset
end

--- Add a path to the set of paths monitored for changes, stored in
--  `self.registered`. These monitored paths will be checked by the
--  `:set()` method to decide whether the tree manager must be
--  notified.
--  @param hpath the path to start monitoring.
function MT :register(hpath)
    self.registered[hpath] = true
end

--- Remove a path from the set of paths monitored for changes, stored in
--  `self.registered`. These monitored paths will be checked by the
--  `:set()` method to decide whether the tree manager must be
--  notified.
--  @param hpath the path to stop monitoring.
function MT :unregister(hpath)
    self.registered[hpath] = nil
end

--- Create a new ram handler.
local function newhandler()
    local instance = {
        name       = hname,
        registered = { },
        values     = { },
        nlnodes    = { }
    }
    return setmetatable(instance, MT)
end

return newhandler()
