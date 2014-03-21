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

--- Handler generator: transform a table into a treemgr handler
--  which reads and writes on this nested table.
--  No notification / registration / deregistration is supported.

local path = require 'utils.path'

local MT = { }; MT.__index=MT

--- Read a variable
function MT :get(hpath)
    if hpath=='' then return nil, self.table end
    local parent_path, key = path.split(hpath, -1)
    local parent_table = path.find(self.table, parent_path)
    if not parent_table then
        return nil, "path "..parent_path.." not found"
    end
    local x = parent_table[key]
    if x==nil then return nil
    elseif type(x)~='table' then return x
    else return nil, x end
end

--- Write a map of variables
function MT :set(hmap)
    local t = self.table
    for hpath, val in pairs(hmap) do
        local parent_path, key = path.split(hpath, -1)
        local parent_table = path.find(t, parent_path, 'noowr')
        if type(parent_table) == 'table' then parent_table[key] = val
        else return nil, "cannot write to path "..hpath end
    end
    return true
end

-- Registration not supported
function MT :register() end


local function newhandler(t)
    return setmetatable({table=t}, MT)
end

return newhandler
