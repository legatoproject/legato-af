#!/usr/bin/lua
 --[[
 /*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
]]


local args={...}


if not args[1] then print("Usage:\n\t rcgen.lua inputfile") return end

local f = assert(io.open(args[1], "r"), "cannot open file: "..args[1])


-- ReturnCode table: ectn[code] = name
local ectn = {}
-- Description table: ectd[code] = description
local ectd = {}
-- Sorted code list: error code are sorted in ascending order
local ects = {}

local ln = 1
local namesize = 0
local nbofcodes = 0



----------------------------------------------------------------------------------------
-- Parse the input error code definition file
-- The Parsing check for duplicates (either names or codes)
-- The name can only be all caps alphanum + "_" char.
while true do
    local line = f:read("*l")
    if not line then break end

    if not line:match("^%s*$") and not line:match("^%s*\#") then -- skip empty line and comments (line starting with '#' char)
        local code, name, desc = line:match("^%s*([\-%d]+)%s+([%u%d_]+)%s+(.+)$")
        code = tonumber(code)
        
        if not code or not name or not desc then error(string.format("Pattern matching error when trying to parse line %d: %s", ln, line)) end
        if code > 0 then error(string.format("Return codes must be negative. Error at line %d: %s", ln, line)) end

        namesize = namesize < #name and #name or namesize -- Compute enum name max size
        nbofcodes = nbofcodes < -code and -code or nbofcodes -- Compute the enum max value

        -- Fill in the error code table, with dual entry.
        -- Check that there is no duplicated codes or names
        assert(not ectn[code], "Error code value already used: "..code)
        ectn[code] = name
        assert(not ectn[name], "Error name already used: "..name)
        ectn[name] = code
        ectd[code] = desc
        ln = ln+1
    end
end
f:close()

-- File successfully parsed. No duplicate found: good job!

-- Sort return codes by value
for k in pairs(ectd) do table.insert(ects, k) end
table.sort(ects, function(a,b) return a>b end)

-- Utility function to copy template file until the generation tag
local function copygen(tf, of)
    while true do
        local line = tf:read("*l")
        if not line then error("Bad  template file: does not contain the // RCGEN tag") end
        if line:match("^%s*//%s*RCGEN") then break end
        of:write(line.."\n")
    end
end



----------------------------------------------------------------------------------------
-- Generate the header file
local tf = assert(io.open("returncodes.h.tpl", "r"))
local of = assert(io.open("returncodes.h", "w"))

copygen(tf, of)

namesize = namesize +3 -- count the RC_ prefix
for _, code in ipairs(ects) do
    local line = string.format("  % -"..namesize.."s = %4d,  ///< %s", "RC_"..ectn[code], code, ectd[code])
    of:write(line.."\n")
end
of:write(tf:read("*a")) -- copy the remaning part of the file...
of:close()
tf:close()



----------------------------------------------------------------------------------------
-- Generate the c file containing the table
local tf = assert(io.open("returncodes.c.tpl", "r"))
local of = assert(io.open("returncodes.c", "w"))

copygen(tf, of)

-- generate the code -> string array
for code = 0, -nbofcodes, -1 do
    local name = ectn[code]
    name = name and '"'..name..'",' or "NULL,"
    local line = string.format('  % -'..namesize..'s  //%4d', name, code)
    of:write(line.."\n")
end

copygen(tf, of)

-- generate the string -> code array
local ects = {} -- create a table sorted by name
for k in pairs(ectd) do table.insert(ects, ectn[k]) end
table.sort(ects)
for _, name in ipairs(ects) do
    local code = ectn[name]
    local line = string.format('  { %4d, "%s" },', code, name)
    of:write(line.."\n")
end

of:write(tf:read("*a")) -- copy the remaning part of the file...
of:close()
tf:close()

print("Return code file generation completed successfully")