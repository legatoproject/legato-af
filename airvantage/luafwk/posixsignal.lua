-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require "sched"
local psignal = require "sched.posixsignal"
local checks = require "checks"
local log = require "log"

local type = type
local tostring = tostring

local M = { }

M.__posixrun = { }

function M.signal(sig, cb)
    checks("string|number", "?function|boolean")
    sig = type(sig) == "number" and psignal.__sigdef[sig] or sig
    if sig and M.__posixrun[sig] then
        sched.kill(M.__posixrun[sig])
        log("PSIGNAL", "DEBUG", "'%s' unregistered on '%s'", tostring(M.__posixrun[sig]), sig)
        M.__posixrun[sig] = nil
    end
    if not cb then
        log("PSIGNAL", "DEBUG", "unregistering '%s'", sig)
        return psignal.signal(sig)
    end
    if sig and type(cb) == "function" then
        M.__posixrun[sig] = sched.sigrun("posixsignal", sig, cb)
        log("PSIGNAL", "DEBUG", "registering '%s' on '%s'", tostring(M.__posixrun[sig]), sig)
    else
        log("PSIGNAL", "DEBUG", "registering '%s'", sig)
    end
    return psignal.signal(sig, true)
end

M.raise = psignal.raise
M.kill = psignal.kill

return M
