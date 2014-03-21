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
    if hpath=='' then return self.constant
    else return nil, 'path not found' end
end

--- Write a map of variables
function MT :set(hmap)
    return nil, 'cannot write to path'
end

-- Registration not supported
function MT :register() end


local function newhandler(k)
    return setmetatable({constant=k}, MT)
end

return newhandler
