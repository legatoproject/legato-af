-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Thread-bound lock objects
-- =========================
--
-- A lock allows to define atomic units of code execution:
-- a portion of code that is protected by a lock will never be executed by more
-- than one thread at a time.
-- Because of the use of coroutines for the threading in Lua, the use of locks
-- is only relevant when you want to protect a portion of code that contains
-- blocking APIs (i.e. network access, scheduling, etc...)
--
-- A lock must be acquired and released by the same thread: an error will be
-- triggered if a thread tries to release a lock acquired by another thread.
-- This behavior is enforced because it is almost always a concurrency mistake
-- to have a thread releasing another thread's locks; but you can circumvent
-- this by passing the owning thread as an extra parameter.
--
-- A lock is automatically released when its owning thread dies, if it failed
-- to do so explicitly.
--
-- API
-- ---
--
-- lock.new()
--      return a new instance of a lock.
--
-- LOCK :acquire ()
--      acquire the lock.
--
-- LOCK :release ([thread])
--      release the lock. The optional thread param is defaulted to the
--      current thread. A lock must be released by the thread that
--      acquired it (or use the thread params to release another thread's lock).
--
-- LOCK :destroy()
--      destroy the lock. Any waiting thread on that lock will trigger a
--      "destroyed" error
--
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Associative lock objects
-- ------------------------
--
-- Function lock (object) allows to associate a lock to an arbitrary object.
-- This API creates a standard lock (as above). It removes the burden of
-- creating the lock and managing the association between the object
-- and the lock.
--
-- Locks associated to objects still have an owning thread, and the rule
-- "locks must be released by the thread which owns them" still applies.
--
-- API
--
-- lock.lock (object)
--      lock the object.
--
-- lock.unlock(object, thread)
--      unlock the object. thread is optional as in lock:acquire() function.
--
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Synchronized functions
-- ----------------------
--
-- A common pattern is to have a function that must not be called more than once
-- at a given time: if the function is already running in another thread, one
-- must wait for this call to finish before calling it again.
--
-- API
--
-- lock.synchronized (f)
--      return a synchronized wrapper around f. A typical idiom is:
--
--      mysynchronizedfunction = lock.synchronized (function (...)
--          [code]
--      end)
--------------------------------------------------------------------------------
local sched = require 'sched'
local checks = require 'checks'
local setmetatable = setmetatable
local tostring = tostring
local string = string
local error = error
local assert = assert
local table = table
local next = next
local proc = proc -- sched internal table
local pairs = pairs
local type = type
local unpack = unpack

require 'utils.table' -- needed for table.pack. To be removed when we switch to Lua5.2


module(...)

--------------------------------------------------------------------------------
--
--------------------------------------------------------------------------------
local objlocks = setmetatable({}, {__mode = "k"})
local hooks =  {}

-- Lock object
LOCK = {}


--------------------------------------------------------------------------------
-- Create a new lock object. It remains unlocked.
--------------------------------------------------------------------------------
function new()
    return setmetatable({ wt = {} }, { __index = LOCK })
end

--------------------------------------------------------------------------------
-- Destroy a lock object.
--------------------------------------------------------------------------------
function LOCK:destroy()
    self.owner = "destroyed" -- means that the lock is being destroyed !
    for t, _ in pairs(self.wt) do
        sched.signal(self, t)
        self.wt[t] = nil
    end
end

--------------------------------------------------------------------------------
-- Helper to release locks on dead threads.
--------------------------------------------------------------------------------
local function autorelease(thread)
    for l, _ in pairs(hooks[thread]) do
        if l.owner == thread then l:release(thread)
        elseif l.wt then l.wt[thread] = nil end
    end
    hooks[thread] = nil
end

--------------------------------------------------------------------------------
-- Helper to release locks on dead threads.
--------------------------------------------------------------------------------
local function protectdie(self, thread)
    if not hooks[thread] then
        local h = sched.sigonce(thread, "die", function()
            autorelease(thread)
        end)
        hooks[thread] = {sighook = h}
    end
    (hooks[thread])[self] = true
end

--------------------------------------------------------------------------------
-- Helper to release locks on dead threads.
--------------------------------------------------------------------------------
local function unprotectdie(self, thread)
    (hooks[thread])[self] = nil
    if not next(hooks[thread], next(hooks[thread])) then -- if this was the last lock attached to that thread...
        sched.kill(hooks[thread].sighook)
        hooks[thread] = nil
    end
end

--------------------------------------------------------------------------------
-- Attempt to take ownership of a lock; might block until the current owner
-- releases it.
-- If nonblocking is set to true, then the call return nil "lock is used" if the lock is used
--------------------------------------------------------------------------------
function LOCK:acquire(nonblocking)
    local t = proc.tasks.running --coroutine.running()
    assert(self.owner ~= t, "a lock cannot be acquired twice by the same thread")
    assert(self.owner ~= "destroyed", "cannot acquire a destroyed lock")

    if nonblocking and self.owner then return nil, "lock is used" end

    protectdie(self, t) -- ensure that the lock will be unlocked if the thread dies before unlocking...

    while self.owner do
        self.wt[t] = true
        sched.wait(self, {t}) -- wait on the current lock with the current thread
        if self.owner == "destroyed" then error("lock destroyed while waiting") end
    end
    self.wt[t] = nil
    self.owner = t

    return "ok"
end

--------------------------------------------------------------------------------
-- Return the number of waiting threads on that lock
--------------------------------------------------------------------------------
function LOCK:waiting()
    local n = 0
    for k in pairs(self.wt) do n = n+1 end
    return n
end
--------------------------------------------------------------------------------
-- Release ownership of a lock.
--------------------------------------------------------------------------------
function LOCK:release(thread)
    thread = thread or proc.tasks.running --coroutine.running()
    assert(self.owner ~= "destroyed", "cannot release a destroyed lock")
    assert(self.owner == thread, "unlock must be done by the thread that locked")
    unprotectdie(self, thread)
    self.owner = nil

    -- wakeup a waiting thread, if any...
    local t = next(self.wt)
    if t then
        sched.signal(self, t)
    end

    return "ok"
end

--------------------------------------------------------------------------------
-- Create and acquire a new lock, associated to an arbitrary object.
--------------------------------------------------------------------------------
function lock(object, nonblocking)
    assert(object, "you must provide an object to lock on")
    assert(type(object) ~= "string" and type(object) ~= "number", "the object to lock on must be a collectable object (no string or number)")
    if not objlocks[object] then objlocks[object] = new() end
    return objlocks[object]:acquire(nonblocking)
end

--------------------------------------------------------------------------------
-- Return the number of waiting threads on that locked object.
--------------------------------------------------------------------------------
function waiting(object)
    assert(object, "you must provide an object")
    if not objlocks[object] then return 0 end -- if the lock was never used then there is nobody waiting on it...
    return objlocks[object]:waiting()
end

--------------------------------------------------------------------------------
-- Release an object created with lock().
--------------------------------------------------------------------------------
function unlock(object, thread)
    assert(object, "you must provide an object to unlock on")
    assert(objlocks[object], "this object was not locked")
    return objlocks[object]:release()
end

--------------------------------------------------------------------------------
-- Create a synchronized version of function f, i.e. a function that behaves
-- as f, except that no more than one instance of it will be running at a
-- given time.
--------------------------------------------------------------------------------
function synchronized(f)
    checks('function')
    local function sync_f(...)
        local k = lock (f)
        local r = table.pack( f(...) )
        unlock(f)
        return unpack (r, 1, r.n)
    end
    return sync_f
end
