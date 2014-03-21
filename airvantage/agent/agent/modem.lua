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
-------------------------------------------------------------------------------

local at = require 'at'
local config = require 'agent.config'

module(...)

function init()
    -- Initialize the at processor
    local s, err = at.init(config.modem.atport)
    if not s then return nil, err end

    -- Check that the modem is alive !
    local r
    r, err = at("at")
    if not r or r[1]~="OK" then
        r, err = at("at+cfun=1,1")
    end
    if not r or r[1]~="OK" then
        at.deinit()
        return nil, "Modem does not respond to 'AT' command: "..(err or r[1])
    end

    --  make error reporting more verbose
    at("at+cmee=1") -- works on OpenAT and other modem
    at("at+cmee=2") -- more verbose, but does not work on some modems (OpenAT)

    -- Enter the SIM pin if necessary
    r, err = at("at+cpin?")
    if not r then return nil, err end
    if r[1] == "+CPIN: READY" then
        return "ok"
    elseif r[1] == "+CPIN: SIM PIN" then
        if not config.modem.pin or config.modem.pin=="" then return nil, "SIM requires a pin code but none is provided into the config file" end
        r, err = at("at+cpin=%s", config.modem.pin)
        if not r or r[1]~="OK" then return nil, "Entering SIM Pin number failed"
        else return "ok" end
    else
        return nil, "Failed to guess PIN status, reported error="..r[1]
    end
end