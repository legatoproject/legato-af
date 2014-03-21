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
local serial = require 'serial'
local string = string
local table = table
require 'coxpcall'
local copcall = copcall
local log = require 'log'
local _G = _G


module(...)

local serial_port
local serial_port_name
local cmd_nb_of_lines
local wait_for_prompt
local prompt_func


local function try(s, err, ...)
    if not s then
       _G.error(err, 0)
    end
    return s, err, ...
end

-----------------------------
-- block until previous
-- AT commands has finished
-----------------------------
local at_cmd_running
local function lock_atproc(command)
   while at_cmd_running do
      sched.wait("atproc", "at_cmd_done")
   end
   at_cmd_running = command;
end

local function unlock_atproc()
    at_cmd_running = nil;
    sched.signal("atproc", "at_cmd_done")
end


local function readAtResponse(s, nb, prompt)
    local result = {}
    local line

    while not nb or nb>0 do
        line = nil

        if prompt then
            line = ""
            while not line:match(prompt) do
                 line = line..try(s:read(1))
            end
            if prompt_func then copcall(prompt_func) end
            line = prompt..try(s:read("*l")) -- finish the line !
            prompt = nil
        else
            while not line or line == "" do
                line = try(s:read("*l")) -- raise an exception in case of error
            end
        end

        table.insert(result, line)
        -- if this is a final response
        if (not nb) and line == "OK" or line == "ERROR" or string.find(line,"+CM%u ERROR:") or string.find(line,"+CPIN") then
            -- the "+CPIN" test is a workaround Wavecom's broken implementation
            -- of AT command "AT+CPIN", which won't answer "OK" even upon success.
            return result
        end
        if nb then nb = nb-1 end
    end

    return result
end


local function reader(s)

    local line
    local result

    while true do
        line = nil
        while not line or line == "" do
            line = try(s:read("*l")) -- raise an exception in case of error
        end

        -- If this is a user entered line
        if line == at_cmd_running then
            local result = readAtResponse(s, cmd_nb_of_lines, wait_for_prompt)

            -- reset some state vars
            cmd_nb_of_lines = nil
            wait_for_prompt = nil

            -- notify the end the atcommand
            sched.signal("atproc","response", result)

        -- this is a URC line
        elseif line:byte() == 43 then -- URC always start with a '+'
            sched.signal("atproc","urc", line)

        -- other lines are just ignored !
        else
            log("ATPROC", "INFO", " Unexpected AT line: "..line)
        end
    end

end

------------------------------------------
-- initiliaze module and start the reader
------------------------------------------
function init(port)

    if serial_port then
        if serial_port_name == port then
            return "already initialized"
        else
            serial_port:close()
        end
    end

    serial_port_name = port
    serial_port, err = serial.open(port)
    if err then return nil, err end

    sched.run(function () local s, err = copcall(reader, serial_port) if err ~= "closed" then log("ATPROC", "ERROR", "AT Reader failed with error %s", err) end end)

    return "ok"
end

function deinit()
    if serial_port then
        serial_port:close()
        serial_port = nil
    end
end

local function run_internal(nb_lines, prompt, func, cmd, ...)
    if not serial_port then return nil, "No serial port configured" end

    local command = string.format(cmd, ...)
    lock_atproc(command)

    cmd_nb_of_lines = nb_lines
    wait_for_prompt = prompt
    prompt_func = func
    serial_port:write(command.."\r\n")

    local event, result = sched.wait("atproc", {"response", 30}) -- there is a 30 s timeout
    if event == "timeout" then
        result = {"ATPROC timeout error", "ERROR"}
    end

    unlock_atproc()
    return result
end
-----------------------------------------------
-- Runs AT command given as parameter,
-- and block until nb_lines of the at response
-- has been read. If more at response are read,
-- events will be triggered as standard URCs
-----------------------------------------------
function runpart(nb_lines, cmd, ...)
    return run_internal(nb_lines, nil, nil, cmd, ...)
end


---------------------------------------
--Runs AT command given as parameter,
--and returns all intermediate
--responses in a table
---------------------------------------
function run(cmd, ...)
    return run_internal(nil, nil, nil, cmd, ...)
end


---------------------------------------
--Runs AT command given as parameter,
--and returns all intermediate
--responses in a table
---------------------------------------
function runprompt(prompt, func, cmd, ...)
    return run_internal(nil, prompt, func, cmd, ...)
end

---------------------------------------
-- This function allows to directly write on the serial link
-- It muts be used with extrem care so that it is not used while
-- somebody else is writing to the same interface...
---------------------------------------
function write_internal(buf)
    serial_port:write(buf)
end


-- make the at table call-able as a function
_G.setmetatable(_M, {__call = function(t, ...) return run(...) end})

