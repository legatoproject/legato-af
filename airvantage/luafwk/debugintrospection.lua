-------------------------------------------------------------------------------
-- Copyright (c) 2011 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Julien Desgats for Sierra Wireless - initial API and implementation
--     Simon Bernard  for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- This module defines a single "class" which allows to dump arbitrary values by gathering as munch data as
-- possible on it for debugging purposes.
--
-- Recursions are avoided by "flatten" all tables in the structure and referencing them by their ID.
-- The dump format is documented in Confluence Wiki:
--    https://confluence.anyware-tech.com/display/ECLIPSE/Lua+IDE+Internship#LuaIDEInternship-Datadumpformat
--
-- It has two major parts :
--   dump : Result data structure: keys are numbers which are IDs for all dumped tables.
--          Values are table structures.
--   tables : Table dictionary, used to register all dumped tables, keys are tables,
--            values are their ID. Functions upvalues are also stored here in the same way.
--
-- The dump itself is done by type functions, one for each Lua type. For example a complete dump of the VM is done with:
--   local introspection = require"debugintrospection"
--   local dump = introspection:new()
--   dump.dump.root = dump:table(_G) -- so dump.root will be a number which point to the result of _G introspection
--
-- WARNING: do never keep any direct reference to internal fields (dump or tables): a dump object is not re-dumped to
-- avoid making huge data structures (and potentially overflow l2b)

-- Utility function (from Mihini's utils, copied here to avoid this dependency for a single function)
local function isregulararray(T)
    local n = 1
    for k, v in pairs(T) do
        if rawget(T, n) == nil then return false end
        n = n + 1
    end
    return true
end

local dump_pool = {}
dump_pool.__index = dump_pool

local all_dumps = setmetatable({ }, { __mode = "k" }) -- register all dumps to avoid to re-dump them (see above warning)

--- Creates a new dump pool with specified options
-- @param dump_locales (boolean) whether local values are dumped
-- @param dump_upvalues (boolean) whether function upvalues are dumped
-- @param dump_metatables (boolean) whether metatables (for tables and userdata) are dumped
-- @param dump_stacks (boolean) whether thread stacks are dumped
-- @param dump_fenv (boolean) whether function environments are dumped
function dump_pool:new(dump_locales, dump_upvalues, dump_metatables, dump_stacks, dump_fenv, keep_reference)
    local dump = setmetatable({
        current_id      = 1,
        tables          = { },
        dump            = { },
        -- set switches, force a boolean value because nil would mess with __index metamethod
        dump_locales    = dump_locales and true or false,
        dump_upvalues   = dump_upvalues and true or false,
        dump_metatables = dump_metatables and true or false,
        dump_stacks     = dump_stacks and true or false,
        dump_fenv       = dump_fenv and true or false,
        keep_reference  = keep_reference and true or false,
    }, self)
    all_dumps[dump] = true
    return dump
end

function dump_pool:_next_id()
    local id = self.current_id
    self.current_id = id + 1
    return id
end

function dump_pool:_register_new(value)
    local id = self.current_id
    self.current_id = id + 1
    self.tables[value] = id
    return id
end

--- Utility function to factorize all metatable handling
function dump_pool:_metatable(value, result, depth)
    --TODO: add support for __pairs and __ipairs ?
    if self.dump_metatables then
        local mt = getmetatable(value)
        if mt then
            result.metatable = self[type(mt)](self, mt, depth-1)
            if mt.__len then result.length = #value end
        end
    end
    return result
end

--- Adds a field into destination table, if both key and value has been successfully dumped
function dump_pool:_field(dest, key, value, depth)
    local dkey, dvalue = self[type(key)](self, key, depth-1), self[type(value)](self, value, depth-1)
    if dkey and dvalue then dest[#dest + 1] = { dkey, dvalue } end
end

--- Functions used to extract debug informations from different data types.
-- each function takes the value to debug as parameter and returns its
-- debugging structure (or an id, for tables), modifying the pool if needed.

function dump_pool:table(value, depth)
    depth = depth or math.huge
    if depth < 0 then return nil end

    if all_dumps[value] then return nil end
    local id = self.tables[value]
    if not id then
        -- this is a new table: register it
        id = self:_register_new(value)
        local t = { type = "table", repr = tostring(value), ref = self.keep_reference and value or nil }
        --Detect "arrays" (tables with 1..n keys). Empty tables are really tables
        t.array = (not not next(value)) and isregulararray(value)

        -- For arrays, make sure that keys are given in 1..n order
        for k,v in (t.array and ipairs or pairs)(value) do
            self:_field(t, k, v, depth)
        end

        -- The registered length refers to # result because if actual element count
        -- can be known with dumped values
        t.length = #value
        self:_metatable(value, t, depth)
        self.dump[id] = t
    end
    return id
end

function dump_pool:userdata(value, depth)
    depth = depth or math.huge
    if depth < 0 then return nil end

    return self:_metatable(value, { type = "userdata", repr = tostring(value), ref = self.keep_reference and value or nil }, depth)
end

function dump_pool:thread(value, depth)
    depth = depth or math.huge
    if depth < 0 then return nil end

    local result = { type = "thread", repr = tostring(value), status = coroutine.status(value), ref = self.keep_reference and value or nil }
    local stack = self.tables[value]
    if self.dump_stacks and not stack then
        stack = self:_register_new(value)
        local stack_table = { type="special" }

        for i=1, math.huge do
            if not debug.getinfo(value, i, "f") then break end
            -- _filed is not used here because i is not a function and there is no risk to get a nil from number or function
            stack_table[#stack_table+1] = { self:number(i, depth - 1), self["function"](self, i, depth - 1, value) }
        end

        stack_table.repr = tostring(#stack_table).." levels"
        self.dump[stack] = stack_table
    end
    result.stack = stack
    return result
end

dump_pool["function"] = function(self, value, depth, thread) -- function is a keyword...
    depth = depth or math.huge
    if depth < 0 then return nil end

    local info = thread and debug.getinfo(thread, value, "nSfl") or debug.getinfo(value, "nSfl")
    local func = info.func -- in case of value is a stack index
    local result = { type = "function", ref = self.keep_reference and func or nil }
    result.kind = info.what

    if info.name and #info.name > 0 then result.repr = "function: "..info.name -- put natural name, if available
    elseif func  then                    result.repr = tostring(func)          -- raw tostring otherwise
    else                                 result.repr = "<tail call>" end       -- nothing is available for tail calls

    if not func then return result end -- there is no more info to gather for tail calls

    if info.what ~= "C" then
        --TODO: do something if function is not defined in a file
        if info.source:sub(1,1) == "@" then
            result.file = info.source:sub(2)
        end
        result.line_from = info.linedefined
        result.line_to = info.lastlinedefined
        if info.currentline >= 0 then
            result.line_current = info.currentline
        end
    end

    -- Dump function env (if different from _G)
    local env = getfenv(func)
    if self.dump_fenv and env ~= getfenv(0) then
        result.environment = self:table(env, depth - 1)
    end

    -- Dump function upvalues (if any), trated as a table (recursion is handled in the same way)
    local upvalues = self.tables[func]
    if self.dump_upvalues and not upvalues and func and debug.getupvalue(func, 1) then
        -- Register upvalues table into result
        local ups_table = { type="special" }
        upvalues = self:_register_new(func)

        for i=1, math.huge do
            local name, val = debug.getupvalue(func, i)
            if not name then break end
            self:_field(ups_table, name, val, depth)
        end

        ups_table.repr = tostring(#ups_table)
        self.dump[upvalues] = ups_table
    end
    result.upvalues = upvalues

    -- Dump function locales (only for running function, recursion not handled)
    if self.dump_locales and type(value) == "number" then
        local getlocal = thread and function(...) return debug.getlocal(thread, ...) end or debug.getlocal
        if getlocal(value, 1) then
            local locales = { type="special" }
            local locales_id = self:_next_id()

            for i=1, math.huge do
                local name, val = getlocal(value, i)
                if not name then break
                elseif name:sub(1,1) ~= "(" and val ~= self then -- internal values are ignored
                    self:_field(locales, name, val, depth)
                end
            end

            locales.repr = tostring(#locales)
            self.dump[locales_id] = locales
            result.locales = locales_id
        end
    end
    return result
end

function dump_pool:string(value, depth)
    depth = depth or math.huge
    if depth < 0 then return nil end

    -- make the string printable (%q pattern keeps real newlines and adds quotes)
    return { type = "string", repr = string.format("%q", value):gsub("\\\n", "\\n"), length = #value,
             ref = self.keep_reference and value or nil }
end

-- default debug function for other types
setmetatable(dump_pool, {
    __index = function(cls, vtype)
        return function(self, value, depth)
            return (depth == nil or depth >= 0) and { repr = tostring(value), type=vtype, ref = self.keep_reference and value or nil } or nil
        end
    end
})

return dump_pool
