-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
--     Romain Perier      for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- This module provides the ability to send and receive SMS (Short Message
-- Service) messages via the embedded radio module.
--
-- @module sms
--

local common = require 'racon.common'

local M = { notifysmsid = { }; initialized=false; sem_value=1 }

local function sem_wait()
   while M.sem_value <= 0 do
      sched.wait("sms.semaphore", "unlock")
   end
   M.sem_value = M.sem_value - 1
end

local function sem_post()
   M.sem_value = M.sem_value + 1
   if M.sem_value >= 1 then
      sched.signal("sms.semaphore", "unlock")
   end
end

-- Receive an SMS notification from racon
local function emp_handler_NewSMS (payload)
    local emitter, message, id = unpack(payload)
    for _, v in pairs(M.notifysmsid) do
       if v.regId == id then
      v.cb(emitter, message)
      return 0
       end
    end
    return -1, "cannot find callback"
end

--MUST NOT BLOCK !!!!
local function emp_ipc_broken_handler()

   --block sem: services using EMP are blocked until sem is released
   -- This avoids data races between reregistration and other threads which send EMP commands
   M.sem_value=0
   sched.run(function()
        for _, v in pairs(M.notifysmsid) do
           local status, id = common.sendcmd("RegisterSMSListener", { v.senderp, v.messagep })
           if status~="ok" then log("SMS", "WARNING", "Failed to register back callback %s", tostring(v.cb)) end
           v.regId = id
        end
        sem_post()
         end
        )
end

--------------------------------------------------------------------------------
-- Initialize the module. No service provided by this module will work unless
-- this initializer function has been called first.
--
-- @function [parent=#sms] init
-- @return a true value upon success
-- @return `nil` followed by an error message otherwise.
--
function M.init()
    if M.initialized then return "already initialized" end
    local a, b = common.init()
    if not a then return a, b end
    common.emphandlers.NewSMS = emp_handler_NewSMS
    common.emp_ipc_brk_handlers.SMSIpcBrkHandler = emp_ipc_broken_handler
    --sched.sigHook(common.emp, "ipc broken", sem_wait)
    M.initialized=true
    return M
end

--------------------------------------------------------------------------------
-- Sends an SMS.
-- Multiple SMS sending is done automatically if the message cannot be held in one SMS.
--
-- @function [parent=#sms] send
-- @param recipient string, number/name of the SMS recipient.
-- @param message string containing the message (binary).
-- @param format string defining the format to use to send the message.
--  Accepted values are "7bits", "8bits" or "ucs2", supported formats may differ depending on the hardware and network capabilities.
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--
function M.send(recipient, message, format)
    checks("string", "string", "string")
    if not M.initialized then error "Module not initialized" end
    return common.sendcmd("SendSMS", { recipient, message, format })
end

--------------------------------------------------------------------------------
-- Registers a callback on SMS reception.
-- New SMS will be notified if the content (sender or message) of the SMS
-- matches the given patterns.
--
-- @function [parent=#sms] register
-- @param callback function to be called on sms reception matching both patterns
--  with recepient and message as parameters.
--  Callback signature: `callback(recipient_string, message_string)`.
-- @param senderp string [lua pattern](http://www.lua.org/pil/20.2.html)
--  that matches the sender address; `nil` means "no filtering".
-- @param messagep string [lua pattern](http://www.lua.org/pil/20.2.html)
--  that matches the message (sms content), `nil` means "no filtering".
-- @return id (can be any Lua non-nil value) should be used only to
--  call @{#sms.unregister}.
-- @return `nil` followed by an error message otherwise.
--

function M.register(callback, senderp, messagep)
    checks("function", "?string", "?string")
    if not M.initialized then error "Module not initialized" end

    sem_wait()
    local status, id = common.sendcmd("RegisterSMSListener", { senderp, messagep })
    if status ~= "ok" then
       sem_post()
       return nil, (id or "unknown error")
    end
    local userId = 1
    for _, v in pairs(M.notifysmsid) do
       userId = v.userId
    end
    userId = userId + 1
    table.insert(M.notifysmsid, {userId=userId, regId=id, cb=callback, senderp=senderp, messagep=messagep})
    sem_post()
    return userId
end

--------------------------------------------------------------------------------
-- Cancels a callback registration on SMS reception.
--
-- @function [parent=#sms] unregister
-- @param id returned by a previous @{#sms.register} call.
-- @return `"ok"` on success.
-- @return `nil` followed by an error message otherwise.
--

function M.unregister(userid)
    checks("string|number")

    if not M.initialized then error "Module not initialized" end
    sem_wait()
    for k, v in pairs(M.notifysmsid) do
       if v.userId == userid then
      local s, msg = common.sendcmd("UnregisterSMSListener", v.regId)
      if s == "ok" then
         table.remove(M.notifysmsid, k)
      end
      sem_post()
      return s, msg
       end
    end
    sem_post()
    return nil, "invalid id to unregister"
end

return M
