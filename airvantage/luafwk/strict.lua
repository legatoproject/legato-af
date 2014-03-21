-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--
-- strict.lua
--
-- checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
--
-- This is a variant of the strict.lua file distributed with Lua 5.1.4
-- (additional support for a global() declaration function)

local getinfo, error, rawset, rawget = debug.getinfo, error, rawset, rawget

local mt = getmetatable(_G)
if mt == nil then
    mt = {}
    setmetatable(_G, mt)
end

mt.__declared = {}

local function what ()
    local d = getinfo(3, "S")
    return d and d.what or "C"
end

mt.__newindex = function (t, n, v)
    if not mt.__declared[n] then
        local w = what()
        if w ~= "main" and w ~= "C" then
            error("assign to undeclared variable '"..n.."'", 2)
        end
        mt.__declared[n] = true
    end
    rawset(t, n, v)
end

mt.__index = function (t, n)
    if not mt.__declared[n] and what() ~= "C" then
        error("variable '"..n.."' is not declared", 2)
    end
    return rawget(t, n)
end

function global(...)
    local vals = { }
    for i, v in ipairs{...} do
        mt.__declared[v] = true
        vals[i] = rawget(_G, v)
    end
    return unpack(vals)
end
