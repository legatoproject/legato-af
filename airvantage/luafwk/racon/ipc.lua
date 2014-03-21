-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require 'sched'
local string = require 'string'
local setmetatable = setmetatable
local tostring = tostring
local error = error

local M = { }

------------------------------------------------------------------------------------------------------------------------------------------
-- String FIFO definition
-- If needed by someone else this could be exported in its own module
------------------------------------------------------------------------------------------------------------------------------------------

local FIFO_MT = {BUFFER_MAX_SIZE = 5*1024}; FIFO_MT.__index = FIFO_MT

local function newfifo()
    return setmetatable({buffer = ""}, FIFO_MT)
end

function FIFO_MT:read(n)
    local b = ""

    while true do
        local c = #self.buffer >= n and n or #self.buffer -- get the min value between n and #self.buffer
        b = b..self.buffer:sub(1, c)
        self.buffer = self.buffer:sub(c+1, -1)
        n = n - c
        sched.signal(self, "read")
        if n == 0 then break end
        local event = sched.wait(self, "*")
        if event ~= "write" and event ~= "read" then return nil, string.format("Event %s occured while reading", tostring(event)), b end
    end

    return b
end

function FIFO_MT:write(buffer)

    local idx = 1
    while true do
        local space = self.BUFFER_MAX_SIZE - #self.buffer
        local remaining = #buffer - idx +1
        local c = space >= remaining and remaining or space -- get the min value between #buffer-idx and BUFFER_MAX_SIZE-#self.buffer
        self.buffer = self.buffer..buffer:sub(idx, idx+c-1)
        idx = idx + c
        sched.signal(self, "write")
        if idx == #buffer+1 then break end
        local event = sched.wait(self, "*")
        if event ~= "write" and event ~= "read" then return nil, string.format("Event %s occured while writing", tostring(event)), idx end
    end

    return #buffer
end

function FIFO_MT:close()
    sched.signal(self, "close")
    return true
end

------------------------------------------------------------------------------------------------------------------------------------------
-- String IPC definition
-- This object provide the same API as a standard socket, but internally use a simple fifo mechanism
------------------------------------------------------------------------------------------------------------------------------------------
local function try(s, err, ...)
    if not s then
       error(err, 2)
    end
    return s, err, ...
end

local IPC_MT = {}
IPC_MT.__index = IPC_MT
-- Return a couple of full duplex local ipc
function M.new()
    local a, b
    a = setmetatable({}, IPC_MT)
    b = setmetatable({}, IPC_MT)
    a.peer = b
    b.peer = a
    a.fifo = newfifo()
    b.fifo = newfifo()

    return a, b
end

--TODO could implement the "standard" masks for receiveing data (line, all it can, etc)
function IPC_MT:receive(n)
    if not self.fifo then return nil, "ipc closed locally" end
    return self.fifo:read(n)
end
function IPC_MT:read(n)
    if not self.fifo then error("ipc closed locally") end
    return try(self.fifo:read(n))
end

-- TODO could implement full luasocket API for sending
function IPC_MT:send(buffer)
    if not self.peer.fifo then return nil, "ipc closed remotely" end
    return self.peer.fifo:write(buffer)
end
function IPC_MT:write(buffer)
    if not self.peer.fifo then error("ipc closed locally") end
    return try(self.peer.fifo:write(buffer))
end

-- TODO: could implement shutdown to close half duplex of the link
function IPC_MT:close()
    if self.fifo then self.fifo:close() end
    if self.peer.fifo then self.peer.fifo:close() end
    self.fifo = nil
    self.peer.fifo = nil
end

function IPC_MT:getpeername()
   return string.format("<local ipc=%s>", tostring(self.peer)), 0 -- socket:getpeername returns address, port
end

return M