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
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local emp = require 'racon.common'
local sched  = require 'sched'
local u = require 'unittest'
local t = u.newtestsuite("emp")
local reconnected = 0
local cb_invoked = 0

local cmdmaps = {
   ["TriggerTimeout"] = "SendData",
   ["SendCmd"] = "Register",
   ["IpcBroken"] = "ConnectToServer",
   ["SimulateCrash"] = "RegisterSMSListener",
}

local tasks = {}

local function newCallbackCmd(payload)
   cb_invoked = 1
   return 0, nil
end

local function empReconnectionCallback()
   reconnected = 1
end

local function send_cmd(id)
   while true do
      local s, p = emp.sendcmd(cmdmaps["SendCmd"], "sendcmd_payload")

      if not s and p ~= "CLOSED" then
     log("EMP_TEST", "ERROR", "EMP sender thread #%d failed: %s", id, p)
     os.exit(1)
      end
   end
end

function t: setup()
   emp.port = 1235
   emp.retry = 2
   emp.timeout = 2
   require 'racon.empparser'.cmd_timeout = 2
   emp.emphandlers["Unregister"] = newCallbackCmd
   emp.emp_ipc_brk_handlers.empTestIpcBrkHandler = empReconnectionCallback
   assert(emp.init())

   assert(emp.init() == "already initialized")
   assert(emp.init() == "already initialized")
end

function t: teardown()
   assert(cb_invoked == 1)
end

function t: test_00_reconnecting()
   emp.sendcmd(cmdmaps["IpcBroken"], nil)

   while reconnected == 0 do
      sched.wait(0.1)
   end
end

function t: test_01_start_mt_cmd()
   for i = 1, 4 do
      tasks[i] = sched.run(send_cmd, i)
   end
end

function t: test_02_trigger_response_timeout()
   local s, p = emp.sendcmd(cmdmaps["TriggerTimeout"])
   print("s", s, "p", p)
   assert(not s and p:match("TIMEOUT"))
end

function t: test_03_stop_mt_cmd()
   for i = 1, 4 do
      sched.kill(tasks[i])
   end
end

--FIXME: Fix this testcase
function t: reconnecting_fail()
   local s, err

   --FIXME: luasocket seems buggy when calling socket.connect twice
   repeat
      s, err = emp.sendcmd(cmdmaps["SimulateCrash"])
   until (not s and err == "COMMUNICATION_ERROR")
end

return t
