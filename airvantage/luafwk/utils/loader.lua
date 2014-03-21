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
-- Code loading/unloading utils.
--
-- This module provides additional utility functions to handle the lifecycle
-- of Lua files.
--
--@module utils.loader
--

local table = table
local load = load
local type= type
local _G = _G
local checks = require"checks"

local M={}
--------------------------------------------------------------------------------
-- Compiles a buffer (ie a list of strings) into an executable function.
--
-- This function is quite similar to Lua's `loadstring()` core function, except
-- that it takes a list of strings as a parameter rather than a single string.
-- Since it never concatenates the string internally, using this function
-- instead of `loadstring()` limit fragmentation issues when loading large files
-- on a device with very limited RAM resources.
--
-- @function [parent=#utils.loader] loadBuffer
-- @param buffer a list of strings.
-- @param name is an optional chunk name used for debug traces.
-- @param destroy is a boolean. When true, the buffer is read destructively (saves RAM when handling big sources).
-- @return function resulting of the buffer compilation
--

function loadbuffer (buffer, name, destroy)
    checks("table", "?string", "?")
    local remove = table.remove
    local function dest_reader() return remove (buffer, 1) end
    local i = 0
    local function keep_reader() i=i+1; return buffer[i] end
    return load (destroy and dest_reader or keep_reader, name)
end

--------------------------------------------------------------------------------
-- Unloads a module by removing references to it.
--
-- When a module isn't used anymore, one can call this function with the module's
-- name as a parameter. It will remove references to the module in `package`'s
-- internal tables, thus allowing the module's resources to be reclaimed by the
-- garbage collector if no deangling references remain in the application.
--
-- 1. Call function `package.loaded[name].__unload()` if it exists
-- 2. Clear `package.loaded[name]`
-- 3. Clear `_G[name]`
--
-- @function [parent=#utils.loader] unload
-- @param name the name of the module to unload.
--

function unload(name)
    checks("string")
    local l = _G.package.loaded
    local p = l[name]
    local u = type(p) == 'table' and p.__unload
    if u then u() end
    l[name] = nil
    _G.rawset(_G, name, nil)
end

M.unload=unload;
M.loadbuffer=loadbuffer;M.loadBuffer=loadbuffer
return M
