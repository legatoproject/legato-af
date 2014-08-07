--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Cuero Bugot    for Sierra Wireless - initial API and implementation
--     Julien Desgats for Sierra Wireless - initial API and implementation
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
--     Romain Perier  for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require = require
local sched   = require 'sched'
local log     = require 'log'
local yajl    = require 'yajl'
local errnum  = require 'returncodes' .tonumber

require 'pack'
require 'coxpcall'
local pairs = pairs
local setmetatable = setmetatable
local assert = assert
local error = error
local copcall = copcall
local string = string
local type = type
local tostring = tostring
local _G  = _G

local M = { cmd_timeout = 60 }

local COMMAND_NAMES = { }

-- make the table symmetric
for k, v in pairs{
    [1]='SendData',
    [2]='Register',
    [3]='Unregister',
    [4]='ConnectToServer',
    [7]='RegisterSMSListener',
    [51]='UnregisterSMSListener',
    [8]='NewSMS',
    [52]='SendSMS',

    [ 9]='GetVariable',
    [10]='SetVariable',
    [11]='RegisterVariable',
    [12]='NotifyVariable',
    [13]='UnregisterVariable',

    [20]='SoftwareUpdate',
    [21]='SoftwareUpdateResult',
    [22]='SoftwareUpdateStatus',
    [23]='SoftwareUpdateRequest',
    [24]='RegisterUpdateListener',
    [25]='UnregisterUpdateListener',

    [30]='PData',
    [32]='PFlush',
    [33]='PAcknowledge',

    [40]='TableNew',
    [41]='TableRow',
    [43]='TableSetMaxRows',
    [44]='TableReset',
    [45]='ConsoNew',
    [46]='ConsoTrigger',
    [47]='SendTrigger',

    [50]='Reboot',

} do COMMAND_NAMES[v], COMMAND_NAMES[k] = k, v end

local api = { }; api.__index = api

local serialize = yajl.to_string

local function deserialize(str)
    return str and yajl.to_value('['..str..']', {check_utf8=false})[1] or yajl.null
end

-- Create and return a new instance of an EMP Parser
-- skt must be a "stream" object that supports read/write calls (as for channels/sockets)
function M.new(skt, cmdhook)
    return setmetatable({skt = skt, inprogress = {n=0}, cmdhook = cmdhook}, api)
end


-- Listen for messages coming from the agent and send a signal on message reception
local function parse(self)
    local skt = self.skt
    while true do
        local _, cmd, type, size, status, cmdname, rid
        _, cmd, type, rid, size = assert(skt:receive(8)):unpack(">HbbI") -- get the message header
        rid = rid+1
        cmdname = COMMAND_NAMES[cmd] or cmd
        if type%2 == 1 then -- this is a reply message

            if size < 2 then
                log("EMP", "ERROR", "Missing status bytes in EMP ack, defaulting status to OK")
                status = 0
            else
                --command status is transmitted as a big endian signed short
                _, status = assert(skt:receive(2)):unpack(">h")
                size = size -2
            end
            local serialized_payload = size>0 and assert(skt:receive(size)) or nil
            local payload = deserialize(serialized_payload)
            if payload == yajl.null then payload=nil end

            log('EMP', 'DEBUG', "[->RCV] [RSP] #%d %s %s", rid, cmdname, serialized_payload)

            if self.inprogress[rid] == "TIMEDOUT" then
                self.inprogress[rid] = nil
                self.inprogress.n = self.inprogress.n-1
            elseif self.inprogress[rid] then
                self.inprogress[rid] = nil -- this finishes an in-progress command
                self.inprogress.n = self.inprogress.n-1
                sched.signal(self, cmdname..tostring(rid), status, payload)
            else
                log("EMP", "ERROR", "Received an unexpected response: cmd [%s], payload [%s], rid[%d]", cmdname, payload, rid)
            end


        else -- this is a command message
            local serialized_cmd_payload = size>0 and assert(skt:receive(size))
            local cmd_payload = serialized_cmd_payload and deserialize(serialized_cmd_payload)
            if cmd_payload == yajl.null then cmd_payload=nil end

            log('EMP', 'DEBUG', "[->RCV] [CMD] #%d %s %s", rid, cmdname, serialized_cmd_payload or '<none>')
            local function execcmd()
                log('EMP', 'DEBUG', "payload = %s, type = %s, serialized = %s", tostring(cmd_payload), _G.type(cmd_payload), serialized_cmd_payload)
                local copcall_status, cmd_status, resp_payload = copcall(self.cmdhook, cmdname, cmd_payload)
                if not copcall_status then
                    cmd_status, resp_payload = errnum 'WRONG_PARAMS', cmd_status
                elseif _G.type(cmd_status)~='number' then
                    log('EMP', 'ERROR', "Invalid status returned by handler for %q: %s", cmdname, sprint(cmd_status))
                    cmd_status = errnum 'WRONG_PARAMS'
                end
                local serialized_resp_payload = serialize (resp_payload)
                log('EMP', 'DEBUG', "[<-SND] [RSP] #%d %s %s", rid-1, cmdname, serialized_resp_payload)
                --send the command execution result to the command sender
                --(command status is transmitted as a big endian signed short)
                assert(skt:send(string.pack(">HbbIh", cmd, 1, rid-1, 2+#serialized_resp_payload, cmd_status)..serialized_resp_payload))
            end
            sched.run(execcmd)
        end
    end
end


-- run the reception thread
-- this call is blocking until some error happen of the skt is closed
function api:run(reconnect_handler)
    local s, err, skt, status
    while true do
       s, err = copcall(parse, self) --parse doesn't return anything, err is only Lua error given by copcall.
       self.skt = errnum 'IPC_BROKEN'
       sched.signal(self, "ipc broken")

       if not reconnect_handler then break end
       status, skt = reconnect_handler()
       -- If the reconnection is impossible, then die
       if not status then break end
       self.skt = skt
    end
    self.skt = 512
    sched.signal(self, "server unreachable")
    return nil, err
end

local function getrequestid(self, cmdname)
    -- make sure we have a free request id
    while self.inprogress.n >= 256 do
        local ev = sched.wait(self, "*")
        if ev == 'closed' then error("closed") end
    end
    local rid = #(self.inprogress)+1    -- try to get an available value
    if rid > 256 then                   -- if the value is bigger than the max value (256) then we need to find another one...
                                        -- in practive the chance that this happens is so small that it is not a prb to spend some time here
        for i=1, 256 do rid=i if not self.inprogress[i] then break end end
    end
    self.inprogress[rid] = cmdname -- could have just put a boolean there , but it give more information to put the cmd without taking any memory or perf hit.
    self.inprogress.n = self.inprogress.n+1

    return rid
end


-- Write a command into the pipe!
local function sendcmd(self, cmdname, rid, payload)
    local cmd = COMMAND_NAMES[cmdname] -- convert to number id
    local serialized_payload = serialize(payload)
    log('EMP', 'DEBUG', "[<-SND] [CMD] #%d %s %s", rid, cmdname, serialized_payload)
    assert(self.skt:send(string.pack(">HbbI", cmd, 0, rid-1, #serialized_payload)..serialized_payload))
end

function api:send_emp_cmd(cmd, payload)
    assert(type(cmd) == "string" and COMMAND_NAMES[cmd]) -- ensure the command exist
    if not self.skt    then return errnum 'SERVER_UNREACHABLE', "SERVER_UNREACHABLE:connection closed" end
    if self.skt == 517 then return errnum 'IPC_BROKEN', "IPC_BROKEN:ipc broken" end

    sendcmd(self, cmd, getrequestid(self, cmd), payload)
    return "ok"
end

local ERR_IPC_BROKEN = errnum 'IPC_BROKEN'
local ERR_SERVER_UNREACHABLE = errnum 'SERVER_UNREACHABLE'

function api:send_emp_cmd_wait(cmd, payload)
    assert(type(cmd) == "string" and COMMAND_NAMES[cmd]) -- ensure the command exist
    if not self.skt then return ERR_SERVER_UNREACHABLE, "SERVER_UNREACHABLE:connection closed" end
    if self.skt == ERR_IPC_BROKEN then return ERR_IPC_BROKEN, "IPC_BROKEN:ipc broken" end
    if self.skt == ERR_SERVER_UNREACHABLE then return ERR_SERVER_UNREACHABLE, "SERVER_UNREACHABLE:server unreachable" end

    local rid = getrequestid(self, cmd)
    -- launch it in background (sched.run does not cause a yield) so we are sure that the following wait will not miss an event !!
    sched.run(sendcmd, self, cmd, rid, payload)

    local ev, s, p = sched.wait(self, {cmd..tostring(rid), 'closed', 'ipc broken', M.cmd_timeout})
    if ev == 'closed' then
       self.inprogress[rid] = nil
       return errnum 'COMMUNICATION_ERROR', "COMMUNICATION_ERROR:server unreachable"
    end
    if ev == 'timeout' then
       self.inprogress[rid] = "TIMEDOUT"
       return errnum 'TIMEOUT', "TIMEOUT:timeout for ack expired"
    end
    if ev == 'ipc broken' then
       self.inprogress[rid] = nil
       return errnum 'CLOSED', "CLOSED:ipc broken"
    end
    return s, p
end

return M
