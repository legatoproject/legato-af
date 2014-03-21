-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Implementation of modbus RTU, ASCII master.
--
-- This module relies on the @{serial#serial} module to handle port configuration and
-- actual data exchanges.
--
-- Data read and written by this module are exchanged as _buffers_, i.e. either
-- strings or lists of strings. String(s) contain data as 8-bits bytes;
-- endianness depends on what is expected/returned by the slave device.
-- To encode such strings, it is suggested to rely on @{pack#pack}, the binary
-- string packing library.
--
-- @module modbus
--

local wait = require 'sched'.wait
local serial = require 'serial'
local lock = require 'sched.lock'
local checks = require 'checks'
local log = require 'log'
local string = string
local setmetatable = setmetatable
local tonumber = tonumber
local require = require
local type = type
local pairs = pairs

local print=print -- TODO remove, for dbg only

module(...)

-- Table of Modbus instance methods
local MODBUS_MT = { }


--------------------------------------------------------------------------------
-- Creates a new @{#modbusdev}.
-- Default initialization is `"/dev/ttyS0", 115200, "none", "RTU", 10`
--
-- @function [parent=#modbus] new
-- @param port the serial port on which modbus operates. As for @{serial#serial.open},
--   it can be given as a port number, or as a file path (POSIX operating
--   systems only). Defaults to port 0, i.e. `"/dev/ttyS0"` on POSIX.
-- @param cfg a optional serial device config table, see @{serial.config} for
--   details on accepted fields. <br />
--   Same default values as in @{serial.open}.
--   An additional field 'timeout' can be put in cfg as an integer to configure modbus timeout in seconds.
-- @param mode serial mode as a string: `"ASCII"` or `"RTU"`. <br />
-- Defaults to `"RTU"`.
-- @return #modbusdev the new @{#modbusdev} on success
-- @return `nil` + error message
--

function new (port, cfg, mode)
    local core = require 'modbus.serializer'
    local obj, err = core.initContext(mode)
    if not obj then return nil, err end
    local timeout
    if cfg then
        timeout = cfg.timeout
        cfg.timeout = nil --set cfg.timeout to nil: not accepted by serial
    end
    timeout = tonumber(timeout) or 1
    local link, err = serial.open(port or "/dev/ttyS0", cfg)
    if err then if link then link:close() link = nil end return nil, err end
    link:settimeout(timeout)
    return setmetatable({internal=obj, name=port, cfg=cfg, link=link}, {__index=MODBUS_MT})
end

local function process_request(self, buffer, expected)
    -- Handle failures: free resources, return nil+msg
    local function fail(msg)
        msg = msg or 'unknown'
        if self.link then self.link :close(); self.link=nil end
        lock.unlock(self.internal)
        log('MODBUS', 'WARNING', "Failure while processing a request: %s", msg)
        return nil, msg
    end
    -- send request
    if not self.link then
        local link, err = serial.open(self.name or "/dev/ttyS0", self.cfg)
        if err then return fail(err) end
        link:settimeout(self.cfg and tonumber(self.cfg.timeout) or 1)
        self.link = link
    end
    self.link:flush()
    local written, err, received, data, r
    written, err = self.link:write(buffer)
    if not written or err then return fail(err) end
    -- receive response
    received, err, r = self.link :read (expected)
    if not received then
        if not r then return fail(err) end
        received = r
    end
    -- parse response
    data, err = self.internal :receiveResponse(received)
    if self.tempo then wait(self.tempo) end
    lock.unlock(self.internal)
    log("MODBUS","DEBUG","Request processsed successfully")
    return data, err
end

--------------------------------------------------------------------------------
-- Closes a modbus instance.
--
-- @function [parent=#modbusdev] close
-- @param self
-- @return "ok".
--

function MODBUS_MT:close ()
    self.internal = nil
    if self.link then self.link:close() self.link = nil end
    self.name = nil
    self.cfg = nil
    setmetatable(self, nil)
    return "ok"
end


--------------------------------------------------------------------------------
-- Read a sequence of consecutive coils' content.
--
-- @function [parent=#modbusdev] readCoils
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param length number of coils
-- @return read coil values in a buffer
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Read a sequence of consecutive discrete inputs' content.
--
-- @function [parent=#modbusdev] readDiscreteInputs
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param length number of inputs
-- @return read discrete inputs contant in a buffer
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Read a sequence of consecutive holding registers' content.
--
-- @function [parent=#modbusdev] readHoldingRegisters
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param length number of registers
-- @return read holding registers content in a buffer
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Read a sequence of consecutive input registers' content.
--
-- @function [parent=#modbusdev] readInputRegisters
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param length number of inputs
-- @return read input registers content in a buffer
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Write a value in a coil.
--
-- @function [parent=#modbusdev] writeSingleCoil
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address address of the coil
-- @param value Boolean to write in the coil
-- @return `"ok"`
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Write a value in a register.
--
-- @function [parent=#modbusdev] writeSingleRegister
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address address of the coil
-- @param value Short integer (`[0..0xFFFF]`) to write in the register
-- @return `"ok"`
-- @return `nil` + error message
--


-----------------------------------------------------------------------------------
-- Write a sequence of consecutive coils.
--
-- @function [parent=#modbusdev] writeMultipleCoils
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param nbvalues number of coils to write
-- @param values values to write, as a bytes buffer (8 coils per byte)
-- @return `"ok"`
-- @return `nil` + error message
-- @usage
--  m = modbus.new('/dev/ttyS0')
--  res,err=m:writeMultipleCoils(1, 0, 8, string.pack('x8',
--  true,false,false,true,false,true,false,true))
--


-----------------------------------------------------------------------------------
-- Write a sequence of consecutive registers.
--
-- @function [parent=#modbusdev] writeMultipleRegisters
-- @param self
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param values values to write, as a word buffer (each register is a 16bits word)
-- @return `"ok"`
-- @return `nil` + error message
-- @usage
--  m = modbus.new('/dev/ttyS0')
--  res,err=m:writeMultipleRegisters(1, 90, string.pack("<H8",21,159,357,654,852,
--  357,654,852))

-- Create each individual request method, which are mostly variant of a same closure.
local REQUEST_NAMES = {
    "readCoils", "readDiscreteInputs", "readHoldingRegisters",
    "readInputRegisters",  "writeSingleCoil", "writeSingleRegister",
    "writeMultipleCoils", "writeMultipleRegisters" }
for _, name in pairs(REQUEST_NAMES) do
    MODBUS_MT[name] = function(self, sid, address, ...)
        checks('?', 'number', 'number')
        if not self.internal then return nil, "not ready" end
        lock.lock(self.internal)
        local s, err = self.internal[name](self.internal, sid, address, ...)
        if not s then lock.unlock(self.internal); return nil, err
        else return process_request(self, s, err) end
    end
end

-------------------------------------------------------------------------------
-- Send a custom MODBUS request, not directly supported by the API.
--
-- @function [parent=#modbusdev] customRequest
-- @param self
-- @param req number identifying the custom function
-- @param sid slave id
-- @param payload optional data bytes to send, as a string
-- @return the response as a buffer.
--   Beware, it contains some error-checking bytes as a suffix.
-- @return `nil` + error message.
--

function MODBUS_MT :customRequest (req, sid, payload)
    checks('?', 'number', 'number', '?string')
    if not self.internal then return nil, "not ready" end
    lock.lock(self.internal)
    local s, err = self.internal:customRequest(sid, req, payload)
    if not s then
        lock.unlock(self.internal)
        return nil, err or "custom request error"
    else
        return process_request(self, s, err)
    end
end


-- Deprecated function, for backward compatibility only. Multiplexes various
-- kinds of modbus requests together.
function MODBUS_MT:request (req, sid, address, ...)
    checks('?', 'string|number', 'number', '?number')
    if type(req)=='number' then return self :customRequest(req, sid, ...)
    else
        req = req :gsub ("^[A-Z]", string.lower)
        print ("REQ = "..req)
        local m = MODBUS_MT[req]
        if not m then return nil, "unsupported request "..req
        else return m(self, sid, address, ...) end
    end
end
