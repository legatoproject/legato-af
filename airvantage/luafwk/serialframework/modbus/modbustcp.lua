-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Implementation of modbus TCP, TCP/RTU and TCP/ASCII.
--
-- This module relies on the @{socket#socket} module to handle socket configuration and
-- actual data exchanges.
--
-- Data read and written by this module are exchanged as _buffers_, i.e. either
-- strings or lists of strings. String(s) contain data as 8-bits bytes;
-- endianness depends on what is expected/returned by the slave device.
-- To encode such strings, it is suggested to rely on @{pack#pack}, the binary
-- string packing library.
--
-- @module modbustcp
--

local socket = require 'socket'
local lock = require 'sched.lock'
local checks = require 'checks'
local log = require 'log'
require 'pack'
local string = string
local setmetatable = setmetatable
local tonumber = tonumber
local tostring = tostring
local require = require
local table = table
local type = type
local pairs = pairs


local print=print -- TODO remove, for dbg only

module(...)

-- Table of Modbus instance methods
local MODBUS_MT = {}


--------------------------------------------------------------------------------
-- Creates a new @{#modbusdev}.
-- Default initialization is `TCP`
--
-- @function [parent=#modbus] new
-- @param cfg, an optional @{#config} table.
--  Default values are
--  `{ maxsocket = 1, timeout = 1 }`
-- @param mode tcp mode as a string: `"TCP", "ASCII"` or `"RTU"`. <br />
-- Defaults to `"TCP"`.
-- @return #modbusdev the new @{#modbusdev} on success
-- @return `nil` + error message
--

function new(cfg, mode)
    local core = require 'modbus.serializer'
    local obj, err = core.initContext(mode or "TCP")
    return setmetatable({internal=obj, cfg=cfg, mode=(mode or "TCP"), thost={}, tlink={}}, {__index=MODBUS_MT})
end

--------------------------------------------------------------------------------
-- Modbus configuration table.
--
-- @type config
-- @field maxsocket
-- to set the maximum number of socket in use.
-- accepted value is a stricly positive integer.
-- @field timeout
-- to configure the socket timeout, in seconds.
-- accepted value is a stricly positive integer.
--

local function getsocket(self, host, port)
    local id = string.format("%s:%d", tostring(host) or "", tonumber(port) or 502)
    if not self.tlink[id] then
        if table.maxn(self.thost) >= (self.cfg and tonumber(self.cfg.maxsocket) or 1) then
            local dhost = table.remove(self.thost, 1)
            if dhost and self.tlink[dhost] then
                self.tlink[dhost]:close()
                self.tlink[dhost] = nil
            end
        end
        local link, err = socket.tcp()
        if not link then lock.unlock(self.internal) return nil, err end
        link:settimeout(self.cfg and tonumber(self.cfg.timeout) or 1)
        link:connect(host, tonumber(port) or 502)
        table.insert(self.thost, id)
        self.tlink[id] = link
    end
    return id
end

local function process_request(self, host, port, buffer, expected)
    -- Handle failures: free resources, return nil+msg
    local function fail(msg, id)
        msg = msg or 'unknown'
        if self.tlink[id] then self.tlink[id]:close() self.tlink[id] = nil end
        lock.unlock(self.internal)
        log('MODBUSTCP', 'WARNING', "Failure while processing a request: %s", msg)
        return nil, msg
    end
    -- retrieve a socket
    local id = getsocket(self, host, port)
    -- send request (reconnect if disconnected)
    local _, err, received, data
    --print(string.byte(buffer , 1 ,#buffer))
    _, err = self.tlink[id]:send(buffer)
    if err then return fail(err, id) end
    -- receive response
    if self.mode == "TCP" then
        local header, body
        header, err = self.tlink[id]:receive(6)
        if not header then return fail(err, id) end
        --print(string.byte(header , 1 ,#header))
        _, _, _, expected = string.unpack(header, ">H3")
        --print(length)
        if expected > 0 then body, err = self.tlink[id]:receive(expected) end
        received = (body and header..body) or header
    else
        local r
        received, err, r = self.tlink[id]:receive(expected)
        if not received then
            if not r then return fail(err, id) end
            received = r
        end
    end
    -- parse response
    data, err = self.internal:receiveResponse(received)
    -- if communication error, close the socket
    if err and not string.match(err, "%-") then return fail(err, id) end
    lock.unlock(self.internal)
    log("MODBUSTCP","DEBUG","Request processsed successfully")
    return data, err
end

--------------------------------------------------------------------------------
-- Closes a modbus instance.
--
-- @function [parent=#modbusdev] close
-- @param self
-- @return "ok"
--

function MODBUS_MT:close ()
    self.internal = nil
    if self.tlink then
        for k,v in pairs(self.tlink) do
            self.tlink[k]:close()
        end
    end
    self.mode = nil
    self.tlink = nil
    self.thost = nil
    self.cfg = nil
    setmetatable(self, nil)
    return "ok"
end


--------------------------------------------------------------------------------
-- Read a sequence of consecutive coils' content.
--
-- @function [parent=#modbusdev] readCoils
-- @param self
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
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
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param nbvalues number of coils to write
-- @param values values to write, as a bytes buffer (8 coil per byte)
-- @return `"ok"`
-- @return `nil` + error message
-- @usage
--  m = modbustcp.new()
--  res,err=m:writeMultipleCoils('10.41.51.50', 502, 1, 0, 8, string.pack('x8',
--  true,false,false,true,false,true,false,true)
--


-----------------------------------------------------------------------------------
-- Write a sequence of consecutive registers.
--
-- @function [parent=#modbusdev] writeMultipleRegisters
-- @param self
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
-- @param sid number defining the slave id to send the request to
-- @param address starting address
-- @param values values to write, as a word buffer (each register is a 16bits word)
-- @return `"ok"`
-- @return `nil` + error message
-- @usage
--  m = modbustcp.new()
--  res,err=m:writeMultipleRegisters('10.41.51.50', 502, 1, 0, string.pack("<H8",21,159,357,654,852,
--  357,654,852))
--

-- Create each individual request method, which are mostly variant of a same closure.
local REQUEST_NAMES = {
    "readCoils", "readDiscreteInputs", "readHoldingRegisters",
    "readInputRegisters",  "writeSingleCoil", "writeSingleRegister",
    "writeMultipleCoils", "writeMultipleRegisters" }
for _, name in pairs(REQUEST_NAMES) do
    MODBUS_MT[name] = function(self, host, port, sid, address, ...)
        checks('?', 'string', '?number', 'number', 'number')
        if not self.internal then return nil, "not ready" end
        lock.lock(self.internal)
        local s, err = self.internal[name](self.internal, sid, address, ...)
        if not s then lock.unlock(self.internal); return nil, err
        else return process_request(self, host, port, s, err) end
    end
end

-------------------------------------------------------------------------------
-- Send a custom MODBUS request, not directly supported by the API.
--
-- @function [parent=#modbusdev] customRequest
-- @param self
-- @param req number identifying the custom function
-- @param host ip address or resolvable hostname of server
-- @param port port number of server, defaults to port 502
-- @param sid slave id
-- @param payload optional data bytes to send, as a string
-- @return the response as a buffer.
--   Beware, it contains some error-checking bytes as a suffix.
-- @return `nil` + error message.
--

function MODBUS_MT :customRequest (req, host, port, sid, payload)
    checks('?', 'number', 'string', '?number', 'number', '?string')
    if not self.internal then return nil, "not ready" end
    lock.lock(self.internal)
    local s, err = self.internal:customRequest(sid, req, payload)
    if not s then
        lock.unlock(self.internal)
        return nil, err or "custom request error"
    else
        return process_request(self, host, port, s, err)
    end
end


-- Deprecated function, for backward compatibility only. Multiplexes various
-- kinds of modbus requests together.
function MODBUS_MT:request (req, host, port, sid, address, ...)
    checks('?', 'string|number', 'string', '?number', 'number', '?number')
    if type(req)=='number' then return self :customRequest(req, host, port, sid, ...)
    else
        req = req :gsub ("^[A-Z]", string.lower)
        print ("REQ = "..req)
        local m = MODBUS_MT[req]
        if not m then return nil, "unsupported request "..req
        else return m(self, host, port, sid, address, ...) end
    end
end

