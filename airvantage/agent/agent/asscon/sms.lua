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
-------------------------------------------------------------------------------

local require = require
local messaging = require "messaging"
local asscon = require "agent.asscon"
local log = require "log"
local mime = require"mime"
local errnum = require 'returncodes'.tonumber

local table = table
local string = string
local ipairs = ipairs
local pairs = pairs
local sched = sched
local tostring = tostring
local unpack = unpack

module(...)

-- assetpatterns table: hold the association between clients' name and patterns
local assetpatterns = {}

local initialized

local function new_sms_handler(_, sms)
   -- Check that this is a M3DA protocol message:
   if sms.message:sub(1, 3) == "DA3" then
      log("SMS", "ERROR", "Unsupported M3DA SMS")
   end
   -- notify clients
   for assetname, patterns in pairs(assetpatterns) do
       for id, pattern in pairs(patterns) do
           if pattern.senderp and not sms.address:match(pattern.senderp) then
               break
           elseif pattern.messagep and not sms.message:match(pattern.messagep) then
               break
           end
           sched.run(asscon.sendcmd, assetname, 'NewSMS', { sms.address, sms.message, id })
       end
   end
end

-----------------------------
-- Unregister given client --
-----------------------------
local function unregister(name, id)
    local patterns = assetpatterns[name]
    if not patterns then
        return nil, "nothing to unregister"
    end
    patterns[id] = nil
    return 0
end

-- EMP command for unregistering new SMS listeners
local function EMPUnregisterSMSListener(assetid, id)
    if not id then return nil, "no id provided to unregister" end
    local res, err = unregister(assetid, id)
    if not res then return errnum 'UNSPECIFIED_ERROR', err
    else return 0, nil end
end

----------------------------------------------
-- Register given client as SMSListener for --
-- incomming sms matching given pattern     --
----------------------------------------------
local registerid = 0
local function register(name, senderp, messagep)
    if not initialized then return nil, "not initialized" end
    -- register new client
    if not assetpatterns[name] then
        assetpatterns[name] = {}
        local emp = asscon.assets[name]
        if emp then
            sched.sigrun(emp, "closed", function(...) assetpatterns[name] = nil end)
        else
            log("SMS", "WARNING", "Failed to retrieve EMP for Asset (%s)", name)
        end
    end
    registerid = registerid + 1
    local patterns = assetpatterns[name]
    patterns[registerid] = {senderp = (senderp ~= "") and senderp or nil, messagep = (messagep ~= "") and messagep or nil}
    log("SMS", "DETAIL", "Asset (%s) registered for SMS [%s|%s] with id %d", tostring(name), tostring(senderp), tostring(messagep), registerid)
    return registerid
end

-- EMP command for registering new SMS listeners
local function EMPRegisterSMSListener(assetid, payload)
    local senderp, messagep = unpack(payload)
    local res, err = register(assetid, senderp, messagep)
    if not res then return errnum 'UNSPECIFIED_ERROR', err
    else return 0, res end
end

-- EMP command for sending new SMS
local function EMPSendSMS(assetid, payload)
    local recepient, message, format = unpack(payload)
    local s, err = messaging.sendSMS(recepient, message, format)
    if not s then
        err = tostring(err)
        log("SMS", "ERROR", "Failed to send SMS from asset (%s), '%s'", tostring(assetid), err)
        return errnum 'NOT_AVAILABLE', err
    end
    return 0
end

function init()
    if not initialized then
        initialized = true
        -- Initialize SMS module
        local s, err = messaging.init()
        if not s then return nil, err end
        -- Register a handler to receive SMS
        sched.sighook("messaging", "sms", new_sms_handler)
        -- Register EMP command
        asscon.registercmd("RegisterSMSListener", EMPRegisterSMSListener)
        asscon.registercmd("UnregisterSMSListener", EMPUnregisterSMSListener)
        asscon.registercmd("SendSMS", EMPSendSMS)
    end
    return "ok"
end
