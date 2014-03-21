-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- This is the hardware abstraction layer for the sms services
-- This file may be different for each hardware platform.
--
-- The API to implement in order to provide this sms layer is:
--
--      init():
--              Return non-false-like on success, nil,err otherwise.
--              It should initialize any resources that are needed for this module to work.
--              It is highly recommanded that the init call trigger the below signal if there are already
--                      received SMS stored in the device.
--
--      sendSMSPDU(pdusize, pdubuffer):
--              Return non-false-like on success, nil,err otherwise.
--                  pdusize is the size as required by the AT send PDU mode command: i.e. the number of bytes of
--                      the PDU message without the initial SMSC information
--                  pdubuffer is the string that contain the PDU, it is actually a text string of the hexadecimal
--                      representation of the PDU buffer (=> each byte from the actual buffer are 2 digits from 0 to F)
--
--      SIGNAL: upon SMS reception a signal is sent: emitter: "sms", event: "newpdu", attached data: pdu buffer
--                      represented as a hex string.
--
--
--
-- This is the implementation over AT commands. To work correctly it requires the AT interface
--      that is provided in a separate module.

local sched = require 'sched'
local log = require 'log'
local at = require 'at'
local ipairs = ipairs
local string = string
local tostring = tostring

module(...)



local function flush_received_sms(st)

    -- set the read storage list to the one of the received sms
    if st then
        at.run("at+cpms=%s", st)
    end

    -- read all sms from the list
    r = at.run("at+cmgl=4")
    local i
    for i=1, #r-1, 2 do
        local id, size = r[i]:match('+CMGL: (%d*),[^%,]*,[^%,]*,(%d*)')
        local pdu = r[i+1]

        log("SMS", "DEBUG", "New SMS URC: ID:%d, PDU: %s", id, pdu)

        -- Signal the new SMS
        sched.signal("sms", "newpdu", pdu)

        -- Delete the SMS from the memory
        at.run("at+cmgd=%d", id)
    end
end

function sendSMSPDU(size, pdu)

    local r, err =  at.runprompt(">", function() at.write_internal(pdu.."\26") end, 'at+cmgs=%d', size)

    if not r or r[#r] ~= "OK" then
        return nil, string.format("Error while sending the SMS by AT command. Error: %s", err or tostring(r[#r]))
    end

    return "ok"
end



function init()

    local r

    -- set the SMS PDU mode
    r = at.run("at+cmgf=0")
    if not r or r[#r] ~= "OK" then return nil, "Failed to switch to PDU mode" end

    -- set the verbose error mode (not mandatory but may help to debug)
    at.run("at+cmee=1")

    local function new_sms_handler(event, urc)
        local st, id = urc:match('+CMTI: ("%u*"),(%d*)')

        if id and st then
            sched.run(flush_received_sms, st)
        end
    end


    -- Add Receive SMS URC
    local hook = sched.sighook("atproc", "urc", new_sms_handler)
    r = at.run("at+cnmi=3,1")
    if not r or r[#r] ~= "OK" then
        sched.kill(hook)
        return nil, "Failed to setup New SMS URC"
    end

    -- Flush any SMSs that may be present before initialization
    sched.run(flush_received_sms)

   return "ok"
end
