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

local sched = require 'sched'
local u = require 'unittest'
local psignal = require 'posixsignal'
local os = require 'os'
local io = require 'io'
local log = require 'log'

local tonumber = tonumber

local t = u.newtestsuite("posixsignal")

local counter = 0

local function updatecounter(expected, evt)
    return function()
        counter = counter + 1
        if counter == expected then
            sched.signal("TEST-PSIGNAL", evt)
        end
        log("TEST-PSIGNAL", "INFO", "counter='%d'", counter)
    end
end

local pid

function t:setup()
    local s, err = io.popen("ps -p ${1:-$$} -o ppid=;")
    u.assert_not_nil(s, err)
    pid = s:read("*a")
    s:close()
    u.assert_not_nil(pid)
    pid = string.match(pid, "%d+")
    log("TEST-PSIGNAL", "INFO", "pid='%s'", pid)
    log.setlevel("DEBUG", "SCHED")
end

function t:teardown()
     log.setlevel("INFO", "SCHED")
end

function t:test_signal_simple()
    local s, err = psignal.signal("SIGTERM", updatecounter(10, "A"))
    u.assert_equal("ok", s, err)
    counter = 0
    log("TEST-PSIGNAL", "INFO", "kill -TERM %s", pid)
    for i=1, 10 do
        os.execute("kill -TERM "..pid)
    end
    sched.wait("TEST-PSIGNAL", "A", 1)
    u.assert_equal(10, counter)

    s, err = psignal.signal("SIGTERM", updatecounter(20, "B"))
    u.assert_equal("ok", s, err)
    log("TEST-PSIGNAL", "INFO", "kill -TERM %s", pid)
    for i=1, 10 do
        os.execute("kill -TERM "..pid)
    end
    sched.wait("TEST-PSIGNAL", "B", 1)
    u.assert_equal(20, counter)

    s, err = psignal.signal(15)
    u.assert_equal("ok", s, err)
    for i=1, 10 do
        os.execute("kill -TERM "..pid)
    end
    u.assert_equal(20, counter)
end

function t:test_signal_multiple()
    local s, err = psignal.signal("SIGTERM", true)
    u.assert_equal("ok", s, err)

    s, err = psignal.signal(14, true)
    u.assert_equal("ok", s, err)

    s, err = psignal.signal("SIGBUS", updatecounter(15, "C"))
    u.assert_equal("ok", s, err)

    counter = 0
    log("TEST-PSIGNAL", "INFO", "kill -TERM %s", pid)
    log("TEST-PSIGNAL", "INFO", "kill -14 %s", pid)
    log("TEST-PSIGNAL", "INFO", "kill -BUS %s", pid)
    for i=1, 15 do
        os.execute("kill -TERM "..pid)
        os.execute("kill -14 "..pid)
        os.execute("kill -BUS "..pid)
    end
    sched.wait("TEST-PSIGNAL", "C", 1)
    u.assert_equal(15, counter)

    s, err = psignal.signal("SIGTERM")
    u.assert_equal("ok", s, err)

    s, err = psignal.signal(14)
    u.assert_equal("ok", s, err)

    s, err = psignal.signal("SIGBUS")
    u.assert_equal("ok", s, err)

end

function t:test_raise()
    local s, err = psignal.signal("SIGUSR1", updatecounter(10, "D"))
    u.assert_equal("ok", s, err)

    counter = 0
    log("TEST-PSIGNAL", "INFO", "raise SIGUSR1")
    for i=1, 10 do
        s, err = psignal.raise("SIGUSR1")
        u.assert_equal("ok", s, err)
    end
    sched.wait("TEST-PSIGNAL", "D", 1)
    u.assert_equal(10, counter)

    s, err = psignal.signal("SIGUSR1")
    u.assert_equal("ok", s, err)
end

function t:test_kill()
    local s, err = psignal.signal("SIGTERM", updatecounter(10, "E"))
    u.assert_equal("ok", s, err)
    counter = 0
    log("TEST-PSIGNAL", "INFO", "kill %s SIGTERM", pid)
    for i=1, 10 do
        s, err = psignal.kill(tonumber(pid), 'SIGTERM')
        u.assert_equal("ok", s, err)
    end
    sched.wait("TEST-PSIGNAL", "E", 1)
    u.assert_equal(10, counter)

    s, err = psignal.signal("SIGTERM")
    u.assert_equal("ok", s, err)
end