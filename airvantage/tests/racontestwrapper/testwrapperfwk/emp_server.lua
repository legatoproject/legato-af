-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched  = require 'sched'
local socket = require 'socket'
local emp = nil
local skt_client = nil
local skt = nil

local M = {}

local cmdmaps = {
   ["SendData"]        = "TriggerTimeout",
   ["Register"]        = "SendCmd",
   ["ConnectToServer"] = "IpcBroken",
   ["RegisterSMSListener"] = "SimulateCrash",
}

local cmdhandler = { }

local function idle()
   if emp then
      emp:send_emp_cmd_wait("Unregister", nil)
   end
   sched.wait(0.1)
end

local function emp_cmdhook(cmdname, payload)
   local name = cmdmaps[cmdname]
   print(name .. " command")
   return cmdhandler[name](payload)
end

local function connectionhandler(skt)
   print("new connection " .. tostring(skt))

   skt_client = skt
   emp = require 'racon.empparser'.new(skt)
   emp.cmdhook = emp_cmdhook
   emp:run()
end

local function ipcbroken_handler(payload)
   skt_client:close()
   emp = nil
   return 0, nil
end

local function triggertimeout_handler(payload)
   for i=1, 100 do
      idle()
   end
   return 0, nil
end

local function sendcmd_handler(payload)
   idle()
   return 0, payload
end

local function simulatecrash_handler(payload)
   skt:close()
   skt_client:close()
   skt_client = nil
   emp = nil
   skt = nil
   return 0, nil
end

local function empserver(port)
   local i = 0

   print("EMP testing server on port", port)
   skt = socket.bind("localhost", port, connectionhandler)

   while true do
      idle()
      sched.signal("emp_server", "running")
   end
end

function M.setup(dir, args)
   cmdhandler["TriggerTimeout"] = triggertimeout_handler
   cmdhandler["SendCmd"] = sendcmd_handler
   cmdhandler["IpcBroken"] = ipcbroken_handler
   cmdhandler["SimulateCrash"] = simulatecrash_handler
   sched.run(empserver, tonumber(args))
   sched.wait("emp_server", "running")
end

return M
