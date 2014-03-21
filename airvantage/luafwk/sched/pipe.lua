-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- FIFO PIPES
-- ==========
--
-- Purpose
-- -------
-- Tasks can't easily exchange messages with sighook() or wait():
--
-- Calling wait() in a loop might lose some messages, if a message arrives
-- before the loop had the time to go back to wait(). Example:
--
--     ----------------------------------------------------------
--     -- sender --
--     for i=1, 100 do signal('foo', 'tick', i); sched.wait (2) end
--
--     -- receiver --
--     while true do
--        local  _, n = sched.wait('foo', 'tick')
--        printf ("received %i", n)
--        sched.wait(3)
--     end
--     ----------------------------------------------------------
--
-- sighooks are guaranteed to be called everytime the event is triggered,
-- but they are executed as interruptions, they can't pause a task.
--
-- Pipes solve this issue: everything that is written in a pipe with :send()
-- will be read back, in order, with :receive(). If there's nothing to read yet,
-- the task calling :receive() will pause and wait for the next :send() to
-- provide data to read back.
--
-- :receive() can yield, and therefore must not be used in a sighook.
--
-- :send() can yield only if there is a maxlength and the pipe is already full.
--
--
-- API
-- ---
-- - create a new pipe with:
--    P = pipe()
-- - push data in it with:
--    P :send(x). x can't be nil.
-- - read data back with:
--    P :receive() or P :receive(timeout).
--   This call will block if necessary. It also supports an optional timeout
--   parameter. If there is a timeout and nothing can be read within the delay,
--   nil is returned.
-- - do a non-blocking, non-consumming peek at next avlue with:
--    P :peek()
--   This might return nil is the pipe is currently empty.
-- - return all of the pipe's current content and reset it as empty with:
--    P :reset()
-- - cancel a reading by pushing back an element in the pipe with:
--    P :pushback(x)
-- - retrieve the number of elements currently in a pipe with:
--    P :length()
-- - tune the recompaction parameters:
--    P :setwaste(absolute_waste_factor, proportional_waste_factor)
--
--
-- Algorithmic complexity
-- ----------------------
-- The amortized complexity for sending and receiving is o(log(N)), N
-- being the current number of elements in the pipe. Notice that with
-- the obvious/naive implentation based on table.insert() and
-- table.remove(), one of the operations is in o(log(N)) but the other
-- is in o(N); therefore, writing K elements then reading them back
-- would have a time complexity in o(K^2), compared to o(K*log(K)) here.
--
--------------------------------------------------------------------------------

require 'checks'

--------------------------------------------------------------------------------
-- Object metatable/class
--------------------------------------------------------------------------------
local P = { __type='sched.pipe' }; P.__index = P

local insert, remove, monotonic_time = table.insert, table.remove, require 'sched.timer.core'.time

--------------------------------------------------------------------------------
-- A registry of all currently active pipes. This is intended as a debug aid,
-- and can be safely commented out if not needed
--------------------------------------------------------------------------------
-- proc.pipes = setmetatable({}, {__mode='kv'})

--------------------------------------------------------------------------------
-- Create a new pipe
--------------------------------------------------------------------------------
local function pipe(maxlength)
    checks ('?number')
    local instance = {
        sndidx     = 1;
        rcvidx     = 1;
        content    = { };
        maxlength  = maxlength;
        state      = 'empty';
        wasteabs   = 32 ;
        wasteprop  = 2 }
    if proc.pipes then
        proc.pipes[tonumber(tostring(instance):match'0x%x+')]=instance
    end
    setmetatable (instance, P)
    return instance
end


--------------------------------------------------------------------------------
-- Check whether the 'state' field is consistent with the pipe's content
-- and maxlength, produce a signal and change it if needed.
-- Helper for methodds which modify content or maxlength.
--------------------------------------------------------------------------------
local function updatestate (self)
    local length   = self.sndidx - self.rcvidx
    local oldstate = self.state
    local newstate = length==0 and 'empty' or self.maxlength==length and 'full' or 'ready'
    if oldstate ~= newstate then
        self.state=newstate
        sched.signal(self, 'state', newstate)
    end
end

--------------------------------------------------------------------------------
-- Move all entries to the beginning of content, if the following conditions
-- are both verified:
--  * there are more than self.wasteabs cells to collect;
--  * the number of wasted cells is at more than self.wasteprop times bigger
--    than the number of actually used cells.
--------------------------------------------------------------------------------
local function recompact (self)
    local rcvidx = self.rcvidx
    local wasted = rcvidx-1
    if wasted <= self.wasteabs then return end
    local sndidx = self.sndidx
    local used = sndidx - rcvidx
    if wasted <= self.wasteprop * used then return end
    -- Tests passed, proceed to recompaction
    local content
    if sndidx<2000 then content = { select (rcvidx, unpack (self.content)) } else
        -- avoid stack overflows: stacks can't grow beyond LUAI_MAXCSTACK,
        -- which is set to 8000 by default. Our choice of 2000 is just as
        -- arbitrary as 8000.
        local oldcontent
        oldcontent, content = self.content, { }
        for i=1,used do content[i]=oldcontent[i+wasted] end
    end
    self.content, self.rcvidx, self.sndidx = content, 1, used+1
end

--------------------------------------------------------------------------------
-- Read a value, yield the current task if necessary
--------------------------------------------------------------------------------
function P :receive (timeout)
    checks ('sched.pipe', '?number')
    local due_date
    while true do
        if self.rcvidx==self.sndidx then
            log('SCHED-PIPE', 'DEBUG', "Pipe %s empty, :receive() waits for data", tostring(self))
            due_date = due_date or timeout and monotonic_time() + timeout
            local timeout = due_date and due_date - monotonic_time()
            if timeout and timeout<=0 or sched.wait(self, {'state', timeout})=='timeout' then
                return nil, 'timeout'
            end
        else
            local content, rcvidx = self.content, self.rcvidx
            local x = content[rcvidx]
            content[rcvidx] = false
            self.rcvidx = rcvidx+1
            recompact (self)
            updatestate (self)
            return x
        end
    end
end

--------------------------------------------------------------------------------
-- Push a value to read
--------------------------------------------------------------------------------
function P :send (x, timeout)
    checks ('sched.pipe', '?', '?number')
    assert (x~=nil, "Don't :send(nil) in a pipe")
    local maxlength = self.maxlength
    local due_date
    while self.state == 'full' do
        log('SCHED-PIPE', 'DEBUG', "Pipe %s full, :send() blocks until some data is pulled from pipe", tostring(self))
        due_date = due_date or timeout and monotonic_time() + timeout
        local timeout = due_date and due_date - monotonic_time()
        if timeout and timeout<=0 or wait(self, {'state', timeout})=='timeout' then
            log('SCHED-PIPE', 'DEBUG', "Pipe %s :send() timeout", tostring(self))
            return nil, 'timeout'
        else
            log('SCHED-PIPE', 'DEBUG', "Pipe %s state changed, retrying to :send()", tostring(self))
        end
    end
    local sndidx = self.sndidx
    self.content[sndidx] = x
    self.sndidx = sndidx+1
    updatestate(self)
    return self
end

--------------------------------------------------------------------------------
-- Pushback the last read value
--------------------------------------------------------------------------------
function P :pushback (x)
    assert (x~=nil, "Don't :pushback(nil) in a pipe")
    if self.state=='full' then
        return nil, 'length would exceed maxlength'
    end
    local rcvidx = self.rcvidx-1
    if rcvidx==0 then
        table.insert(self.content, 1, x)
        self.sndidx = self.sndidx+1
    else
        self.content[rcvidx] = x
        self.rcvidx = rcvidx
    end
    updatestate (self)
    return self
end

--------------------------------------------------------------------------------
-- Empty the pipe, return what it contained in a list
--------------------------------------------------------------------------------
function P :reset()
    local r = self.content
    self.content = { }
    updatestate(self)
    return r
end

--------------------------------------------------------------------------------
-- Try to read without yielding nor removing from internal queue
--------------------------------------------------------------------------------
function P :peek()
    return self.content[self.rcvidx]
end

--------------------------------------------------------------------------------
-- Return the number of elements currently in the pipe.
--------------------------------------------------------------------------------
function P :length()
    return self.sndidx - self.rcvidx
end

--------------------------------------------------------------------------------
-- Change or remove (by passing nil) the curent pipe length limitation.
--------------------------------------------------------------------------------
function P :setmaxlength(maxlength)
    checks ('sched.pipe','?number')
    if self :length() > maxlength then return nil, 'length exceeds new maxlength' end
    if maxlength and maxlength<1 then return nil, 'invalid maxlength' end
    self.maxlength = maxlength
    updatestate (self)
    return self
end

--------------------------------------------------------------------------------
-- Change waste parameters, hence the memory/speed performance compromize.
--------------------------------------------------------------------------------
function P :setwaste(abs, prop)
    self.wasteabs, self.wasteprop = abs, prop
    return self
end

return pipe