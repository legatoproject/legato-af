-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Julien Desgats     for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- The serial library  enables communication over the serial port.
--
-- It provides the usual `open`, `read`, `write`, and `close` file handling
-- methods, as well as serial port configuration features.
--
-- @module serial
--------------------------------------------------------------------------------



local core = require 'serial.core'
local wrap = require 'fdwrapper'
local checks = require 'checks'

local M = { }
serial = M

local normalized = {
    data         = 'data',
    stop         = 'stop',
    parity       = 'parity',
    flowcontrol  = 'flowcontrol',
    baudrate     = 'baudrate',

    numdatabits  = 'data',
    numstopbits  = 'stop',
    baud         = 'baudrate',
    flow         = 'flowcontrol' }

local function normalize_config(x)
    local y = { }
    for k, v in pairs(x) do
        local k2 = type(k)=='string' and k :lower() or k
        k2 = normalized[k2]
        if not k2 then error ("Invalid configuration option "..tostring(k)) end
        y[k2] = v
    end
    return y
end

--------------------------------------------------------------------------------
-- Creates and returns a new serial device instance: @{#serialdev}.
--
-- @function [parent=#serial] open
-- @param port the serial port to open. It can be given as a port number,
--  or as a POSIX device file path such as `"/dev/ttyS0"` on POSIX operating
--  systems.
-- @param config, an optional @{#config} table.
--  Default values are
--  `{ parity = "none", flowControl = "none", numDataBits = 8, numStopBits = 1, baudRate = 115200 }`
-- @return #serialdev a new instance of @{#serialdev} as a table when successful
-- @return `nil` followed by an error message otherwise
--

function M.open(port, cfg)
   checks("string|number", "?table")
   if type(port)=='number' then port="/dev/ttyS"..port end
   local p, err = core.open(port, normalize_config(cfg or {}))
   if not p then return nil, err end
   return wrap.new(p)
end


--------------------------------------------------------------------------------
-- Serial port configuration table.
-- Can be given to @{#serialdev.configure} and @{#serial.open} functions:
--
-- @type config
-- @field parity
--   to set serial port parity
--   accepted values are (as a string): `"none"`, `"odd"`, `"even"`
-- @field flowControl
-- to configure flow control on the serial port
-- accepted values are: `"none"`, `"rtscts"` or `"xon/xoff"` (as a string).
-- @field numDataBits
-- to set the number of data bits in each character
-- accepted values are: `5, 6, 7` and `8` (as an integer).
-- @field numStopBits
-- to configure the serial port stop bit
-- accepted values are: `1` to disable it, `2` to enable it (as an integer).
-- @field baudRate
-- to set the baudrate of the serial port
-- accepted values are: `1200, 9600, 19200, 38400, 57600` and `115200` (as an
-- integer).
--


--------------------------------------------------------------------------------
-- Serial class.
-- @type serialdev
--


--------------------------------------------------------------------------------
-- Configures the serial port.
--
-- @function [parent=#serialdev] configure
-- @param self
-- @param #config config a @{#config} table.
-- @return `"ok"` on success.
-- @return `nil` followed by error string otherwise.
--
local original_configure = core.__metatable.configure
function core.__metatable :configure(cfg)
    return original_configure(self, normalize_config(cfg))
end


--------------------------------------------------------------------------------
-- Writes buffer to the serial port.
--
-- @function [parent=#serialdev] write
-- @param self
-- @param buffer string containing data to send over the serial port.
-- @return the number of bytes written on success.
-- @return `nil` followed by an error message otherwise.
--


--------------------------------------------------------------------------------
-- Reads data from the serial port.
--
-- @function [parent=#serialdev] read
-- @param self
-- @param pattern can be a pattern string conforming to the
--   [Lua File API](http://www.lua.org/manual/5.1/manual.html#pdf-file:read),
--   or a number of bytes to read.
-- @return the read data, as a string, on success.
-- @return `nil` followed by error string otherwise.
--


--------------------------------------------------------------------------------
-- Flushes pending data.
--
-- @function [parent=#serialdev] flush
-- @param self
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--


--------------------------------------------------------------------------------
-- Closes the serial library instance.
--
-- Once close has been successfully called, the @{serialdev} instance becomes
-- unusable. A new one may be created if needed.
--
-- @function [parent=#serialdev] close
-- @param self
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

return M