-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
--     Romain Perier  for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local errstr  = require 'returncodes' .tostring

local M = { initialized=false }

-- force the init function to use a socket for IPC communications
M.FORCE_SOCKETS = false
-- address of the remote agent
M.address       = '127.0.0.1'
-- port of the remote agent
M.port          = 9999
-- number of reconnection tried when error "ipc broken" is triggered
M.retry         = 10
-- time used between each reconnection
M.timeout       = 3

local empparser = nil

-- Table where the handlers for received EMP commands are registered
-- by other modules.
M.emphandlers = { }

-- Table where the handlers for register back up layers services are saved
-- by other modules.
M.emp_ipc_brk_handlers = {}

-- Dispatch EMP commands received through the IPC to the appropriate handler.
local function emp_handler(cmdname, payload)
    local handler = M.emphandlers[cmdname]
    if handler then return handler (payload)
    else -- other commands are not supported (from agent to client)
        log("RACON", "WARNING", "No handler for EMP command %q",
            tostring(cmdname))
        return -1, "unsupported command"
    end
end

local function emp_reconnect()
   local log = require "log"
   local socket = require "socket"
   local agent = rawget(_G, "agent")
   local r = 0

   if agent then
      return nil, "failed"
   end

   while r < M.retry do
      log("EMP", "WARNING", "Connection lost, reconnecting to agent, retry #%d", r)
      M.socket = socket.connect(M.address, M.port)
      sched.wait(M.timeout)

      if M.socket then
     for _, handler in pairs(M.emp_ipc_brk_handlers) do
        handler()
     end
     return "ok", M.socket
      end
      r = r + 1
   end
   return nil, "failed"
end

function M.init()
    if M.initialized then return "already initialized" end

    -- detect best IPC choice
    local agent = rawget(_G, "agent")
    if M.FORCE_SOCKETS or not (agent and agent.asscon) then
        local socket = require "socket"
        M.socket = assert(socket.connect(M.address, M.port))
    else
        local ipc     = require "racon.ipc"
        local clients = require "agent.asscon"
        local a, b = ipc.new()
        M.socket = a
        sched.run (clients.connectionhandler, b)
    end

    -- start the cmd/ack reception thread
    local emp = require "racon.empparser"
    empparser = emp.new(M.socket, emp_handler)
    sched.run (function ()
        local s, err = empparser:run(emp_reconnect)
        log("RACON", "DEBUG", "Receiver error, %s", err)
        --sched.signal(self, "closed") -- TODO: probably not needed anymore
        if M.socket then M.socket:close(); M.socket = nil end
    end)

    M.initialized=true
    return M
end

function M.sendcmd (cmd, payload)
    if not M.initialized then error "Module not initialized" end
    local s, b = empparser:send_emp_cmd_wait(cmd, payload)
    if s == 0 then return "ok", (b~="" and b or nil) end
    return nil, b or errstr(s) or "unknown error"
end

return M
