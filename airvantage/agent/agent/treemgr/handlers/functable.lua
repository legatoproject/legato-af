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

--- Handler generator: transform a table of functions into a treemgr handler
--  The table is one level deep, i.e. hpath "foo.bar" is hanlded by the
--  cell in `t['foo.bar']`, not in `t.foo.bar`.
--  Each cell can contains a `get()` and a `set(value)` function.
--  which reads and writes on this nested table.
--
--  Functions are left in charge of causing notifications, if/when appropriate.
--  However, an additional

local path  = require 'utils.path'

local MT = { }; MT.__index=MT

local function get_cell(self, hpath)
    local cell = self.leaves[hpath]
    if cell then
        return cell
    else
        for prefix, suffix in path.gsplit(hpath) do
            cell = self.leaves[prefix..".*"]
            if cell then return cell end
        end
        return nil
    end
end

--- Read a variable
function MT :get(hpath)
    local children = self.children[hpath]
    if children then
        local children_set = { }
        for k, _ in pairs(children) do children_set[k] = true end
        return nil, children_set
    else
        local cell = get_cell(self, hpath)
        if not cell then return nil, "no such path" end
        local getter = cell.get
        if not getter then return nil, "non-readable path" end
        return getter(hpath)
    end
end

--- Write a map of variables. If auto-notification upon set is enabled,
--  cause a notification.
function MT :set(hmap)
    local tonotify = self.notify_set and { } -- map of variable changes, or nil

    for hpath, value in pairs(hmap) do
        local cell = get_cell(self, hpath)
        if not cell then return nil, "no such path" end
        local setter = cell.set
        if not setter then return nil, "non-writable path" end
        local previous_value
        if tonotify then
            local getter = cell.get
            if getter then previous_value = getter(hpath) end
        end
        setter(hpath, value)
        if tonotify and previous_value ~= value then
            tonotify[hpath] = value
        end
    end
    if tonotify and next(self.registered) and next(tonotify) then
        require 'agent.treemgr'.notify(self.name, tonotify)
    end
    return true
end

function MT :register(hpath)
    self.registered[hpath] = true
end

function MT :unregister(hpath)
    self.registered[hpath] = nil
end

local function newhandler(name, leaves, notify_set)
    checks('string', 'table', '?')

    -- Build the parent->children relationship
    local children = { }
    for hpath, _ in pairs(table) do
        for hprefix, relpath in path.gsplit(hpath) do
            if relpath~='' then
                local child = path.split(relpath, 1)
                local children = children[hprefix]
                if children then children[child]=true
                else children[hprefix]={[child]=true} end
            end
        end
    end
    return setmetatable({
        name       = name,
        children   = children,
        leaves     = leaves,
        notify_set = notify_set
    }, MT)
end

return newhandler
