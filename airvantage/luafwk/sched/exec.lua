-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- This module provides versions of `os.execute` and `io.popen` compatible
-- with `sched`, i.e. not blocking the VM while the subprocess is running.
--
-- Caveat: it's based on checking when the child closes its stdout; as such,
-- it will return prematuredly if this closure is performed explicitly before
-- the process terminates, or if it uses the "dup2" system call.
--
-- @module system
--------------------------------------------------------------------------------

local core  = require 'sched.exec.core'
local psignal = require 'posixsignal'

local M = {}

-- Breaks the following circular dependency:
-- agent.boot -> agent.init -> sched.init -> sched.platform -> sched.exec -> fdwrapper -> sched
local function wrap_new(...)
   return require 'fdwrapper' .new(...)
end

--------------------------------------------------------------------------------
--- Executes a command synchronously.
-- The Lua VM is not blocked by the execution of the command.
-- @param cmd string containing the command to execute.
-- @return command exit code as a number.
---@return nil followed by an error message otherwise.
--------------------------------------------------------------------------------
function M.execute(cmd)
    local pid, err = core.execute(cmd)
    if not pid then return nil, err end

    psignal.signal("SIGCHLD", true)
    sched.wait("posixsignal", "SIGCHLD")

    local status, err
    while true do
       status, err = core.waitpid(pid)
       if not status and type(err) == "string" then return nil, err end
       if status then break end
       sched.wait(1)
    end
    return status
end

-- overwrite the orignal fd close function so to block til the end of the command execution
local function pclose(self)
    -- Retrieve the original `close` method, as provided by `fdwrapper`.
    local close = getmetatable(self).__index(self, "close")
    local status, err
    while true do
       status, err = core.waitpid(self.file:getpid())
       if not status and type(err) == "string" then return nil, err end
       if status then break end
       sched.wait(1)
    end
    local s, e = close(self)
    if not s then return nil, e end
    return status
end

--------------------------------------------------------------------------------
--- Executes a command and allows access to the command's standard input and output.
-- The Lua VM is not blocked by the execution of the command.
-- @param cmd string containing the command to execute.
-- @return [File](http://www.lua.org/manual/5.1/manual.html#pdf-file:read) descriptor to access
-- to command standard input and output.
-- @return nil followed by an error message otherwise.
--------------------------------------------------------------------------------
function M.popen(cmd)
   local fd = core.popen(cmd)
   if not fd then return nil, err end
   local fdw = wrap_new(fd)
   fdw.close = pclose
   return fdw
end

return M
