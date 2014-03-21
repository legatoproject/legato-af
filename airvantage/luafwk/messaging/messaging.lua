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
-------------------------------------------------------------------------------

local sched = require 'sched'
local log = require 'log'
local table = table
local ipairs = ipairs
local type = type
local assert = assert
local internal = require 'messaginghal'
require 'coxpcall'
local copcall = copcall
local smspdu = require 'smspdu'
local tableutils = require 'utils.table'

module(...)


-- SMS Send/Receive API
-- Can receive SMS encoded in 7 or 8 bits per char (but lower level APIs/hardware/network may not support all encodings)
-- Can send SMS encoded in 7 or 8 bits per char (but lower level APIs/hardware/network may not support all encodings)
-- Handle concatenated sms in emission and reception
-- Register a function to receive a SMS
--
--- messaging.init()
--- Initialize SMS service.
--  Return non-false-like value on success or nil, err in case of error
--
--- messaging.sendSMS(recepient, message, format)
--- Send an SMS with message to recepient
--  When message is longer than one SMS, concatenated SMS are automatically sent
--  Return non-flase-like on success or nil, err otherwise
--  @param recepient is a string with recepient phone number formatted according to GSM standard.
--  @param message is a string with message data.
--  @param format is a string to configure SMS encoding, the value can be:
--   "7bits" to force 7 bits encoding
--   "8bits" to force 8 bits encoding
--   "ucs2" to force ucs2 encoding
--
--  SIGNAL: "messaging", "sms", sms: when a new SMS is received. sms is a table holding the parsed SMS, it has the fields "address",
--                              "timestamp", "message", plus some optional fields when receiving long SMS, port SMS, etc. (see smspdu.c)

-- table that holds partial concatenated sms
-- (concatsms[sms.ref])[sms.seqnb] = sms
local concatsms = {}
local function add_concat_sms(sms)
    if not concatsms[sms.concat.ref] then
        concatsms[sms.concat.ref] = {}
    end

    local t = concatsms[sms.concat.ref]
    t[sms.concat.seqnb] = sms.message

    for i=1, sms.concat.maxnb do
        if not t[i] then return end -- sequence not complete
    end

    local message = {}
    tableutils.copy(sms, message, true) -- copy all fields from the last SMS
    concatsms[sms.concat.ref] = nil
    message.message = table.concat(t)
    sched.signal("messaging", "sms", message)
end

local function new_sms_pdu(_, pdu)
    log("SMS", "INFO", "New SMS received")
    --decode the pdu
    local t, err = smspdu.decodePdu(pdu)

    if not t then
        log("SMS", "ERROR", "Failed to decode error:%s, pdu:%s", err, pdu)
        return
    end

    -- test if this is a concatenated sms
    if t.concat then
        add_concat_sms(t)
    else
        sched.signal("messaging", "sms", t)
    end
end


function sendSMS(recepient, message, format)
    assert(type(recepient) == "string" and type(message) == "string" and type(format) == "string")

    if format ~="8bits" then return nil, "Invalid format, only 8 bits PDU encoding format is supported for SMS sending" end

    local t, err = smspdu.encodePdu(recepient, message)
    if not t then return nil, err end

    for i, sms in ipairs(t) do
        local r
        r, err = internal.sendSMSPDU(sms.size, sms.buffer)
        if not r then
            return nil, err
        end
    end

    return "ok"
end


local module_initialized
function init()

    if module_initialized then return "already initialized" end

    -- Add Receive SMS signal handler
    sched.sighook("sms", "newpdu", new_sms_pdu)

    local s, err = internal.init()
    if not s then return nil, err end

    module_initialized = true
   return "ok"
end
