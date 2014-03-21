-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- LTN12 source utils.
--
-- LNT12 stands for Lua Technical Note #12. It deals with sources, sinks, filters, etc.
--
-- You can read the official technical note at [Filters Sources And Sinks](http://lua-users.org/wiki/FiltersSourcesAndSinks) article page.
--
-- LTN12 official module is shipped with LuaSocket. Official LTN12 API documentation can be found at [LTN12 Official module](http://w3.impa.br/~diego/software/luasocket/ltn12.html) page.
--
-- This module provides extra LTN12 sources.
--
-- @module utils.ltn12.source
--

local checks = require 'checks'
local ltn12 = require 'ltn12'
local table = table

local M = {}

--------------------------------------------------------------------------------
-- Transforms a LTN12 source into a string.
--
-- @function [parent=#utils.ltn12.source] toString
-- @param src LTN12 source.
-- @return string.
--

local function tostring(src)
    checks('function|table')
    local snk, dump = ltn12.sink.table()
    ltn12.pump.all(src, snk)
    return table.concat(dump)
end

--------------------------------------------------------------------------------
-- Returns a LTN12 source on table.
--
-- @function [parent=#utils.ltn12.source] table
-- @param t the table, must be a list (indexed by integer).
-- @param empty optional boolean, true to remove the table's values.
-- @return LTN12 source function on the table.
--

local function table(t, empty)
    checks('table', '?boolean')
    local index = 0
    return function()
        index = index + 1
        local chunk = t[index]
        if empty then t[index] = nil end
        return chunk
    end
end


M.tostring=tostring; M.toString=tostring
M.table=table

return M
