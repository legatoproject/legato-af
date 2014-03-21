-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------
local sched = require 'sched'
local setmetatable = setmetatable
local string = string
local table = table
local type = type
local tonumber = tonumber
local monotonic_time = require 'sched.timer.core'.time
--local print = print

module(...)

local wrapper = {}
-- the following function allow to call seamlessly the userdata function if it is not present into the wrapper table
local function wrap(table, key)
    if wrapper[key] then
        return wrapper[key]
    elseif table.file[key] then
        return function(self, ...) return table.file[key](table.file, ...) end
    else
        return nil
    end
end

function new(file)
    return setmetatable({buffer = "", file=file}, {__index=wrap})
end

local function read(file, timelimit, timedelay)
    local s, err
    while true do
        s, err = file:read()
        if not s then
            return nil, err
        end
        if #s > 0 then return s end
        s, err = sched.fd.wait_readable(file, timelimit, timedelay)
        if not s then
            return nil, err
        end
    end

end

-------------------------------------------------------
-- file:read(pattern)
-- Read bytes from the file according to the given pattern.
-- Default pattern is '*l'
--      pattern can be
--      '*l' to read a line
--      '*a' read some bytes until timeout
--      n <number> to read n bytes
-- return data read or nil followed by an error
--------------------------------------------------------
function wrapper:read(pattern)
    pattern = pattern or "*l"
    local timelimit = self.totaltimeout and monotonic_time() + self.totaltimeout
    local timedelay = self.blocktimeout
    local r, s, err
    if type(pattern) == 'number' then
        while true do
            if #self.buffer >= pattern then
                r = self.buffer:sub(1, pattern)
                self.buffer = self.buffer:sub(pattern+1)
                return r
            end
            s, err = read(self.file, timelimit, timedelay)
            if not s then
                r = self.buffer
                self.buffer = ""
                if #r == 0 then r = nil end
                return nil, err, r
            end
            self.buffer = self.buffer..s
        end

    elseif pattern == '*l' then
        while true do
            local line, next = self.buffer:match("^(.-)\r*\n(.*)$")
            if line then
                r = line
                self.buffer = next
                return r
            end
            s, err = read(self.file, timelimit, timedelay)
            if not s then
                r = self.buffer
                self.buffer = ""
                if #r == 0 then r = nil end
                return nil, err, r
            end
            self.buffer = self.buffer..s
        end

    elseif pattern == '*a' then
        local b = {}
        table.insert(b, self.buffer)
        self.buffer = ""
        while true do
            s, err = read(self.file, timelimit, timedelay)
            if not s then
                r = table.concat(b)
                if #r == 0 then r = nil end
                if (err ~= "timeout" and err ~= "closed" and err ~= "eof") or not r then
                    return nil, err, r
                else -- for '*a' it is not an error to get timeout or end of file event...
                    return r
                end
            end
            table.insert(b, s)
        end
    else
        return nil, "unknown pattern"
    end
end

function wrapper:write(data)
    local towrite = #data
    local written = 0
    local timelimit = self.totaltimeout and monotonic_time() + self.totaltimeout
    local timedelay = self.blocktimeout
    while written < towrite do
        local n, err = self.file:write(data)
        if not n then return nil, err, written end
        written = written + n

        if written == towrite then return written end

        data = data:sub(n+1)

        n, err = sched.fd.wait_writable(self.file, timelimit, timedelay)
        if not n then return nil, err, written end
    end
end

function wrapper:close()
   sched.fd.close(self.file)
   return self.file:close()
end

function wrapper:settimeout(timeout, mode)
    timeout = tonumber(timeout)
    timeout = timeout and (timeout>=0 or nil) and timeout -- negative timeout is equivalent to no timeout
    if not mode or mode:find("b") then self.blocktimeout = timeout end
    if mode and mode:find("t") then self.totaltimeout = timeout end
    return 1
end
