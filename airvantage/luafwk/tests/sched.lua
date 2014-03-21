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

local sched = require 'sched'
local u = require 'unittest'
local t = u.newtestsuite 'sched'
require 'print'

local assert = u.assert -- used unittest assert in order to count used assert (for the stats)

__acc = { }
function reset() __acc = { } end
function acc(x) table.insert(__acc, x) end

function checkacc(model)
    local ok = (#model == #__acc)
    if ok then
        for i=1, #model do
            if model[i]~=__acc[i] then ok=false; break end
        end
    end
    if not ok then
        printf("Wrong accumulator.\r\nExpected: { %s }\r\nGot:      { %s }",
            table.concat(model, ", "),
            table.concat(__acc, ", "))
        error "bad acc state"
    end
end

function checkleak()
    sched.gc() -- remove dead cells
    local live_cell_found = false
    local foo_events = proc.tasks.waiting.foo
    if foo_events then
        for _, cell in pairs(foo_events) do
            if next(cell) then
                live_cell_found = cell
                break
            end
        end
    end
    if live_cell_found then
        error("Leak! something stayed attached to emitter foo: %s",
        sprint(proc.tasks.waiting.foo))
    end
end

test = { }

-- verify that wait() reschedules
function t :test_wait1()
    reset()
    local function f(prefix, n)
        return function()
            for i=1, n do
                acc(prefix..i)
                sched.wait()
            end
        end
    end
    sched.run(f("A", 5))
    sched.run(f("B", 10))
    sched.wait(3)
    checkacc{
        "A1","B1","A2","B2","A3","B3","A4","B4","A5","B5",
        "B6","B7","B8","B9","B10"}
    checkleak()
end

-- verify that wait(n) takes about n seconds to return
function t :test_wait_n()
    local d1 = 3
    local t1 = os.time()
    sched.wait(d1)
    local t2 = os.time()
    local d2 = t2-t1
    assert (d2>=d1)
    assert (d2-d1<=1, "Surprisingly long delay for wait(n)")
    checkleak()
end

-- verify that wait(em, n) timeouts
function t :test_wait_timeout1()
    local d1 = 3
    local t1 = os.time()
    local sigsent = false
    sched.run (function()
        sched.wait(1)
        sched.signal('foo', 'bar')
        sigsent=true
    end)
    local ev = sched.wait('foo', d1)
    local t2 = os.time()
    local d2 = t2-t1
    assert (d2>=d1)
    assert (sigsent)
    assert (ev == 'timeout')
    assert (d2-d1<=1, "Surprisingly long delay for wait(n)")
    checkleak()
end

-- verify that event '*' catches everything, with sighook
function t :test_hook_star()
    reset()
    sched.run (function()
        sched.wait(1)
        sched.signal('foo', 'bar1')
        sched.wait(1)
        sched.signal('foo', 'bar2')
        sched.wait(1)
        sched.signal('foo', 'bar3')
    end)
    local h = sched.sighook('foo', '*', acc)
    sched.wait(5)
    sched.kill(h)
    checkacc{'bar1', 'bar2', 'bar3'}
    checkleak()
end

-- verify that event '*' catches everything, with wait
function t :test_wait_star()
    reset()
    local writer = sched.run (function()
        sched.wait(1)
        sched.signal('foo', 'bar1')
        sched.wait(1)
        sched.signal('foo', 'bar2')
        sched.wait(1)
        sched.signal('foo', 'bar3')
    end)
    local reader = sched.run (function()
        for i=1, 3 do
            local ev = sched.wait('foo', '*')
            acc (ev)
        end
    end)
    sched.wait(reader, 'die')
    checkacc{'bar1', 'bar2', 'bar3'}
    checkleak()
end

-- verify '*' / timeout interactions: timeout occurs first
function t :test_wait_star_timeout1()
    local ev
    local t = sched.run(function()
        ev = sched.wait('foo','bar', 3)
    end)
    sched.wait(5)
    signal ('foo', 'bar')
    assert (ev=='timeout')
    checkleak()
end

-- verify '*' / timeout interactions: event occurs first
function t :test_wait_star_timeout2()
    local ev
    local t = sched.run(function()
        ev = sched.wait('foo',{'bar', 5})
    end)
    sched.wait(3)
    sched.signal ('foo', 'bar')
    sched.wait() -- let task t reschedule
    assert (ev=='bar')
    checkleak()
end

-- verify hook reattachment and killself
function t :test_hook_killself()
    reset()
    local i = 1
    local function f(ev)
        acc(ev)
        if i==3 then sched.killself() end
        i=i+1
    end
    local h = sched.sighook('foo', '*', f)
    for i=1,5 do sched.signal('foo', 'ev'..i) end
    checkacc{'ev1', 'ev2', 'ev3'}
    assert(not h.hook)
    checkleak()
end

-- verify external hook kill
function t :test_hook_kill()
    reset()
    local h = sched.sighook('foo', '*', acc)
    for i=1,5 do
        sched.signal('foo', 'ev'..i)
        if i==3 then sched.kill(h) end
    end
    assert(not h.hook)
    checkacc{'ev1', 'ev2', 'ev3'}
    checkleak()
end

-- verify hook once
function t :test_sigonce()
    reset()
    local h = sched.sigonce('foo', '*', acc)
    for i=1,3 do
        sched.signal('foo', 'ev'..i)
    end
    checkacc{'ev1'}
    checkleak()
end

-- verify sigrun, sigrunonce
function t :test_sigrunonce1()
    reset()
    local t = sched.sigrunonce('foo', '*', function(ev)
        wait(1)
        acc(ev)
    end)
    for i=1,3 do
        sched.signal('foo', 'ev'..i)
    end
    wait(4)
    checkacc{'ev1'}
    checkleak()
end

function t :test_sigrun_kill()
    reset()
    local t = sched.sigrun('foo', '*', function(ev)
        wait(1)
        acc(ev)
    end)
    for i=1,5 do
        sched.signal('foo', 'ev'..i)
        wait(1)
        if i==3 then sched.kill(t) end
    end
    wait(6)
    checkacc{'ev1','ev2', 'ev3'}
    checkleak()
end

-- verify sigrun killself
-- WARNING: it seems that sometimes this function's ordering isn't
-- consistent. Sometimes the accumulator contains ev1, ev3, ev2
function t :test_sigrun_killself()
    reset()
    local i=0
    local t = sched.sigrun('foo', '*', function(ev)
        i=i+1
        if i>3 then sched.killself() end
        wait(1)
        acc(ev)
    end)
    for j=1,5 do
        sched.signal('foo', 'ev'..j)
        wait(1)
    end
    wait(2)
    checkacc{'ev1','ev2', 'ev3'}
    checkleak()
end

-- reattach to a signal in a hook triggered by that signal
function t :test_hookreattachself()
    reset()
    local i = 1
    local function f()
        acc("ev"..i)
        if i<3 then sched.sigonce('foo', '*', f) end
        i=i+1
    end
    sched.sigonce('foo', '*',  f)
    for i=1,5 do
        sched.signal('foo', 'bar')
        wait(1)
    end
    checkacc{'ev1',  'ev2', 'ev3'}
    checkleak()
end

-- reattach to a signal in a sigrunonce triggered by that signal
function t :test_runreattachself()
    reset()
    local i = 1
    local function f()
        acc("ev"..i)
        wait(1)
        if i<3 then sched.sigrunonce('foo', '*', f) end
        i=i+1
    end
    sched.sigrunonce('foo', '*',  f)
    for i=1,4 do
        sched.signal('foo', 'bar')
        sched.wait(2)
    end
    checkacc{'ev1',  'ev2', 'ev3'}
    checkleak()
end

-- multiwait: verify that the message only occurs once
function t :test_mwait1()
    reset()
    local function f()
        for i=1,4 do
            local em, ev, arg = sched.multiwait({'foo','bar'}, '*')
            acc(table.concat({em,ev,arg},'-'))
        end
        acc 'done'
    end
    sched.run(f)
    sched.wait()
    sched.signal('foo', 'ev1', 'a')
    sched.wait()
    sched.signal('bar', 'ev2', 'b')
    sched.wait()
    sched.signal('foo', 'ev3', 'c')
    sched.wait()
    sched.signal('bar', 'ev4', 'd')
    sched.wait()
    sched.signal('foo', 'ev5', 'e')
    sched.wait()
    checkacc{'foo-ev1-a', 'bar-ev2-b', 'foo-ev3-c', 'bar-ev4-d', 'done'}
    checkleak()
end

-- multiwait + timeout: msg must occur only once, whether triggered by timeout or otherwise.
function t :test_mwait_timeout()
    reset()
    local function f()
        for i=1,4 do
            local em, ev, arg = sched.multiwait({'foo','bar'}, {'*', 2})
            acc(ev..i)
        end
    end
    local t = sched.run(f)
    sched.wait()
    sched.signal('foo', 'evfoo')
    sched.wait(3)
    sched.signal('bar', 'evbar')
    sched.signal('foo', 'evlost')
    sched.signal('bar', 'evlost')
    sched.wait(t, 'die')
    checkacc{'evfoo1', 'timeout2', 'evbar3', 'timeout4'}
    checkleak()
end

-- xtrargs on sighook
function t :test_sighook_xtra()
    reset()
    local function f(ev, xtra1, xtra2, arg1, arg2)
        acc(table.concat({ev, xtra1, xtra2, arg1, arg2}, '-'))
        sched.killself()
    end
    sched.sighook('foo','bar', f, 'a', 'b')
    sched.signal('foo', 'bar', 'c', 'd')
    checkacc{ 'bar-a-b-c-d' }
    checkleak()
end

-- xtrargs on sigonce
function t :test_sigonce_xtra()
    reset()
    local function f(ev, xtra1, xtra2, arg1, arg2)
        acc(table.concat({ev, xtra1, xtra2, arg1, arg2}, '-'))
    end
    sched.sigonce('foo','bar', f, 'a', 'b')
    sched.signal('foo', 'bar', 'c', 'd')
    checkacc{ 'bar-a-b-c-d' }
    checkleak()
end

-- xtrargs on sigrun
function t :test_sigrun_xtra()
    reset()
    local function f(ev, xtra1, xtra2, arg1, arg2)
        acc(table.concat({ev, xtra1, xtra2, arg1, arg2}, '-'))
        sched.killself()
    end
    sched.sigrun('foo','bar', f, 'a', 'b')
    sched.signal('foo', 'bar', 'c', 'd')
    wait()
    checkacc{ 'bar-a-b-c-d' }
    checkleak()
end

-- xtrargs on sigrunonce
function t :test_sigrunonce_xtra()
    reset()
    local function f(ev, xtra1, xtra2, arg1, arg2)
        acc(table.concat({ev, xtra1, xtra2, arg1, arg2}, '-'))
    end
    sched.sigrunonce('foo','bar', f, 'a', 'b')
    sched.signal('foo', 'bar', 'c', 'd')
    sched.wait()
    checkacc{ 'bar-a-b-c-d' }
    checkleak()
end


-- check for leaked resources after triggered timeouts
function t :test_timeout_leak1()
    reset()
    sched.sigonce('foo', {'bar', 2}, acc)
    wait(3)
    sched.signal('foo', 'bar')
    checkacc{'timeout'}
    checkleak()
end

-- check for leaked resources after never-triggered timeouts
function t :test_timeout_leak2()
    reset()
    sched.sigonce('foo', {'bar', 2}, acc)
    sched.signal('foo', 'bar')
    wait(3)
    checkacc{'bar'}
    checkleak()
end

-- check that hooks stay attached even if they fail
function t :test_hook_error()
    reset()
    local h = sched.sighook('foo', 'bar', function()
        acc 'x'
        error "kaboom"
    end)
    signal('foo', 'bar')
    signal('foo', 'bar')
    signal('foo', 'bar')
    sched.kill (h)
    checkacc{'x', 'x', 'x'}
    checkleak()
end

-- check that sigruns stay attached even if they fail
function t :test_sigrun_error()
    reset()
    local t = sched.sigrun('foo', 'bar', function()
        acc 'x'
        wait(1)
        error "kaboom"
    end)
    signal('foo', 'bar')
    signal('foo', 'bar')
    signal('foo', 'bar')
    wait(2)
    sched.kill (t)
    checkacc{'x', 'x', 'x'}
    checkleak()
end

local function ensure_correct_wait(time)
   local monotonic_time = require 'sched.timer.core'.time
   local b = monotonic_time()
   sched.wait(time)
   local e = monotonic_time()
   local delta=time * 1.0 / 100
   u.assert_gte(time, e - b)
   u.assert_lte(time+delta, e - b)
end

function t: test_wait_various_time()
   ensure_correct_wait(0.1)
   ensure_correct_wait(0.2)
   ensure_correct_wait(0.4)
   ensure_correct_wait(0.5)
   ensure_correct_wait(1)
   ensure_correct_wait(1.0)
   ensure_correct_wait(1.15)
   ensure_correct_wait(2.333)
end
