-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--
-- Sched is a Lua collaborative scheduler: it allows several Lua tasks to
-- run in parallel, and to communicate together when they need to
-- interact.
--
-- It offers a convinient way to write programs which address multiple
-- I/O driven issues simultaneously, with much less hassle than with
-- preemptive multithreading frameworks; it also doesn't require
-- developers to adopt unusual programming styles, as expected by Erlang,
-- map-reduce variants, or callback-driven frameworks such as node.js.
-- Among other appropriate usages, it allows to easily write and deploy
-- the applications typically powering machine-to-machine
-- infrastructures.
--
--
-- General principles
-- ==================
--
-- Vocabulary
-- ----------
--
-- * **processes**: concurrent fragments of programs, each having
--    exclusive access to its own memory space.
--
-- * **threads**: concurrent fragments of programs, sharing the same
--    memory space, and which therefore need to synchronize their
--   memory operations.
--
-- * **tasks**: concurrent fragments of programs of any
--   sort. Processes and threads are particular kinds of tasks.
--
--
-- Collaborative multi-tasking
-- ---------------------------
--
-- "Collaborative" implies that the currently running task only changes
-- when it calls a blocking function, i.e. a function which has to wait
-- for an external event. For instance, an attempt to read on an empty
-- socket will block until more data arrives from the network. If the
-- current task tries to read from an empty socket, it will be
-- unscheduled, and will let other tasks run, until more network data
-- arrive. Most collaborative multi-tasking systems, including sched,
-- cannot leverage multi-core architectures.
--
-- Collaborative multi-tasking has one big advantage: it considerably
-- reduces the concurrency issues. Since there are few places where the
-- running task can change, there are much fewer occasions for race
-- conditions, deadlocks, and other concurrency-specific problems.
-- Programmers used to develop preemptively multithreaded applications
-- will be delighted to see how uneventful to debug collaborative
-- concurrent systems are.
--
-- To quote the authors of Lua, "_we did not (and still do not) believe
-- in the standard multithreading model, which is preemptive concurrency
-- with shared memory: we still think that no one can write correct
-- programs in a language where a=a+1 is not deterministic_"
-- [[pdf](http://www.lua.org/doc/hopl.pdf)]. Hence, Lua coroutines share
-- their memory, but give up non-deterministic preemptive scheduling.
--
--
-- Alternatives
-- ------------
--
-- Other languages make the complementary choice: for instance, Erlang
-- retains preemptive concurrency, but forbids shared memory; although
-- such languages allow to reach unmatched levels of reliability
-- [[pdf](http://www.sics.se/~joe/thesis/armstrong_thesis_2003.pdf)],
-- they deeply change the way programmers have to model their
-- programs. They are therefore less suitable to a generalist audience.
--
-- It should be noted that Unix' original design philosophy (many small
-- and short-lived specialized processes, with separated memories,
-- which communicate through file descriptors and IPC), also restrains
-- memory sharing to keep maintainable preemptive scheduling.
--
-- Finally, some process oriented (separate memory and message-passing)
-- multi-tasking systems are available in Lua, most notably Luaproc
-- [[pdf](http://www.inf.puc-rio.br/~roberto/docs/ry08-05.pdf)]. Although
-- such systems make more sense for computation-intensive jobs on
-- multi-core and distributed architectures, it is possible to make it
-- cohabit with sched's multitasking (having several Luaproc
-- processes, each running several sched threads).
--
--
-- Collaborative limitation
-- ------------------------
--
-- Collaborative multi-tasking comes with a limitation, compared to the
-- preemptive variant. Greedy tasks, which never pause nor perform any
-- blocking API call, might monopolize the CPU indefinitely. Although it
-- rarely happen unless on purpose, it means that collaborative
-- schedulers are unsuitable for real-time systems. If such real-time
-- needs occur in a sched-based application, they must be addressed in
-- C in a separate process, and the underlying OS must offer the suitable
-- real-time performances.
--
-- Another perceived issue is that a rogue task can crash all other tasks
-- in the scheduler, but that's true of any pool of tasks sharing their
-- memory, including systems like pthreads.
--
--
-- Sched principles
-- ================
--
-- Communication between tasks
-- ---------------------------
--
-- Sched offers a fundamental communication mechanism, called the signal,
-- over which other mechanisms (mutexes, fifos, synchronized program
-- sections etc.) can be built. You will often find that signals are the
-- simplest and most suitable way to coordinate tasks together, although
-- the classic POSIX-like IPC systems are always available when needed or
-- preferred.
--
-- A signal is composed of:
--
-- * an arbitrary emitter object;
-- * an event: a string describing what noteworthy event happened to the
--   emitter;
-- * optional arguments, which complete the description of the event's
--   circumstances.
--
-- So every object in Lua can emit signals, and emitted signals can
-- trigger reactions:
--
-- * a signal can wake up a task which was waiting for it;
-- * a signal can trigger the execution of a hook, i.e. a function which
--   is run every time a registered signal is detected.
--
-- Objects are encouraged to emit signals every time an event of interest
-- happens to them, so that other tasks can react to these events. For
-- instance, the modem module emits a signal every time a SMS arrives; the
-- NTP time synchronizer advertises clock changes through signals; IP
-- interfaces signal important events such as going up or going down;
-- every task emits a `'die'` signal when it exits, allowing other tasks
-- to monitor their termination; etc. Many complex systems, such as the
-- telnet shell, the Agent initialization process, or TCP data
-- handling, are internally synchronized through signals.
--
--
-- Blocking tasks on signals with `sched.wait`
-- -------------------------------------------
--
-- A wait or a hook can be registered to many signals simultaneously, and
-- conversely, a single signal can trigger many hooks and task wake-ups.
--
-- Most waits and hooks wait for signals from only one emitter. Consider
-- the following example:
--
--      local event, status, result = sched.wait(some_task, 'die')
--
-- It will block the current task until the task `some_task` terminates.
-- @{#sched.wait} returns the triggering event (here always `'die'`),
-- and extra signal arguments. In the case of task `'die'` events, those are
-- the status (whether the task exited successfully or because of an
-- error), and any result returned by the function (or an error message
-- if `status` is `false`).
--
-- A task can wait for several events from a single emitter. For
-- instance, the following statement will wait until the the network
-- manager either succeeds or fails at mounting an IP interface:
--
--      sched.wait('NETMAN', {'MOUNTED', 'MOUNT_FAILED'})
--
-- If a number is added in the events list, it's a timeout in seconds: if
-- none of the subscribed events occur before the timeout elapses, then
-- @{#sched.wait} returns with the event `'timeout'`:
--
--      local event = sched.wait('NETMAN', {'MOUNTED', 'MOUNT_FAILED', 30})
--      if     event == 'MOUNTED' then print "Success!"
--      elseif event == 'MOUNT_FAILED' then print "Failure!"
--      elseif event == 'timeout' then print "Took more than 30s to mount!"
--      else assert(false, "This cannot happen") end
--
-- One can also subscribe to every events sent by a given emitter, by
-- subscribing to the special `'*'` string:
--
--      local event = sched.wait('NETMAN', '*')
--      if     event == 'MOUNTED' then print "Success!"
--      elseif event == 'MOUNT_FAILED' then print "Failure!"
--      else   print("Ignore event "..event) end
--
-- Special `'*'` event can be combined with a timeout, as in
-- `sched.wait(X, {'*', 30})`.
--
-- A task might need to wait on signals sent by several potential
-- emitters. This need is addressed by the @{sched.multiWait} API,
-- which works as @{sched.wait} except that it takes a list of
-- emitters rather than a single one.
--
-- Finally, a task can reschedule itself without blocking. It gives
-- other tasks an opportunity to run; once every other task had a
-- chance to run, the rescheduled task can restart. Task rescheduling
-- is performed by calling @{sched.wait} without argument.
--
--     for i=1, BIG_NUMBER do
--         perform_long_computation_chunk(i)
--         sched.wait()
--     end
--
-- Attaching hooks to signals
-- --------------------------
--
-- Hooks can be registered the same way tasks can be blocked: they are
-- attached to a signal emitter, and to one, several or all (`'*'`)
-- events. A hook has a function, which is executed when one of the
-- registered signal is registered. The variants of hook attachment
-- are:
--
-- * @{#sched.sigHook}`(emitter, events, func, hook_args...)`: the function
--   `func(event, hook_args..., signal_args...)` will be run every time
--   `sched.signal(emitter, event, signal_args...)` is triggered. It is
--   run synchronously, i.e. before `sched.signal()` returns, so it can't
--   contain any blocking API call. It will keep being triggered
--   every time one of the registered signal occurs, until the reference
--   returned by @{#sched.sigHook} is passed to @{#sched.kill}.
--
-- * @{#sched.sigOnce}`(emitter, events, func, hook_args...)`: behaves as
--   @{#sched.sigHook}, except that it's only run once. The hook
--   function is also forbidden from performing blocking calls.
--
-- * @{#sched.sigRun}`(emitter, events, func, hook_args...)`: works as
--   @{#sched.sigHook}, except that the function is run as a scheduled
--   task. As a result, there is no guarantee that it will be executed as
--   soon as the signal has been sent (there might be a delay), but it is
--   allowed to call blocking APIs. For most usages, this form is to be
--   preferred as simpler.
--
-- * @{#sched.sigRunOnce}`(emitter, events, func, hook_args...)`: behaves
--   as @{#sched.sigRun}, except that it's only run once. The hook
--   function is also allowed to perform blocking calls.
--
-- There is a guarantee that hooks won't miss an signal. Blocking a task
-- might lose some signals, if they are not "consumed" fast enough.
--
-- For instance, if one task A produces a signal every 2 seconds,
-- and a task B waits for them, but takes 3 seconds to process each
-- of them, some of them will be lost: they will occur when B processes
-- the previous one, and doesn't wait for any signal.
--
-- Therefore, if it is important not to lose any occurrence of a signal, a
-- waiting task is not an adequate solution: either handle them in a
-- hook, or pass them through a pipe.
--
--
-- Example
-- -------
--
--     sched.sigRun('FOO', 'BAR', function(event, arg)
--         print(">>> sigRun FOO.BAR received event " .. event .. ", arg " .. arg)
--     end
--
--     sched.sigRunOnce('FOO', 'BAR', function(event, arg)
--         print(">>> sigRunOnce FOO.BAR received event " .. event .. ", arg " .. arg)
--     end
--
--     sched.sigRun('FOO', '*', function(event, arg)
--         print(">>> sigRun FOO.* received event " .. event .. ", arg " .. arg)
--     end
--
--     sched.sigRunOnce('FOO', '*', function(event, arg)
--         print(">>> sigRunOnce FOO.* received event " .. event .. ", arg " .. arg)
--     end
--
--     sched.signal('FOO', 'GNAT', 1)
--     >>> sigRun FOO.* received event GNAT, arg 1
--     >>> sigRunOnce FOO.* received event GNAT, arg 1
--
--     sched.signal('FOO', 'BAR', 2) -- sigRunOnce FOO.* now detached
--     >>> sigRun FOO.BAR received event FOO, arg 2
--     >>> sigRunOnce FOO.BAR received event FOO, arg 2
--     >>> sigRun FOO.* received event FOO, arg 2
--
--     sched.signal('FOO', 'BAR', 3) -- sigRunOnce FOO.BAR now detached
--     >>> sigRun FOO.BAR received event FOO, arg 3
--     >>> sigRun FOO.* received event FOO, arg 2
--
--     sched.signal('GNAT', 'BAR', 2) -- wrong emitter, no hook triggered
--
--
-- Tasks life cycle
-- ----------------
--
-- Tasks are created by @{#sched.run}: it takes a function, and
-- optionally arguments to this function, and runs it as a separate,
-- parallel task. It also returns the created task, as a regular
-- coroutine. The main use for this is to either cancel it
-- with @{#sched.kill}, or wait for its termination with
-- `sched.wait(task,'die')`.
--
-- Tasks are sorted in an internal table:
--
-- * blocked tasks are indexed, in `__tasks.waiting`, by the emitters and
--   events which might unblock them;
--
-- * tasks which are ready to run are stacked in a `__tasks.ready` list.
--
-- A task created with @{#sched.run} doesn't start immediately, it is
-- merely stacked in `__tasks.ready`. When the current task dies or
-- blocks, the next one in `__tasks.ready` takes over; it can be the last
-- created one, or another one which went ready before it.
--
-- The termination of a task is advertised by a `'die'` signal. By
-- waiting for this signal, one can synchornize on a task's completion.
-- The `'die'` signal has additional arguments: a status (`true` if the
-- task returned successfully, `false` if it has been aborted by an
-- error, `"killed"` if the task has been cancelled by @{#sched.kill}),
-- followed by either the returned value(s) or the error message. Here
-- are some examples:
--
--     -- Common case, the task terminates successfully:
--     task = sched.run(function()
--         for i=1,3 do
--            print ">>> plop"
--            wait (2)
--         end
--         return "I'm done plopping."
--     end)
--     sched.sigHook(task, 'die', function(event, ...)
--         print(">>> He's dead, Jim. His last words were: "..sprint{...})
--     end)
--     >>> plop
--     >>> plop
--     >>> plop
--     >>> He's dead, Jim. His last words were: { true, "I'm done plopping" }
--
--     -- If the task is interrupted with sched.kill(task):
--     >>> plop
--     sched.kill(task)
--     >>> He's dead, Jim. His last words were: { "killed" }
--
--     -- If task dies with an error:
--     task = sched.run(function()
--         for i=1,3 do
--            print ">>> plop"
--            wait (2)
--         end
--         error "Aaargh!"
--     end)
--     sched.sigHook(task, 'die', function(event, ...)
--         print(">>> He's dead, Jim. His last words were: "..sprint{...})
--     end)
--     >>> plop
--     >>> plop
--     >>> plop
--     >>> He's dead, Jim. His last words were: { false, "Aaargh!" }
--
--
-- Finally, there is the issue of launching the scheduler at the
-- top-level. This issue is OS-dependent, but on POSIX-like systems, this
-- is done by calling `sched.loop`: this function will run every task in
-- a loop, and perform timer management in order to avoid busy waits. It
-- never returns, unless you call `os.exit`. Therefore, before starting
-- the loop, it is important to have prepared some tasks to launch, with
-- @{#sched.run}.
--
-- As an example, here is a complete program which starts a telnet server
-- and writes the time in parallel, in a file, every 10 seconds.
--
-- Notice that `shell.telnet.init` creates a listening socket, which will
-- launch a new telnet client for each connection request, and each of
-- these clients will run concurrently with other tasks.
--
--     require 'sched'
--     require 'shell.telnet'
--
--     sched.run (function()
--         shell.telnet.init{
--             address     = '0.0.0.0',
--             port        = 2000,
--             editmode    = "edit",
--             historysize = 100 }
--     end)
--
--     sched.run(function()
--         local f = io.open('/tmp/time.txt', 'w')
--         while true do
--             f :write(os.date(), '\n')
--             f :flush()
--             wait(10)
--         end
--     end)
--
--     sched.loop()
--
-- @author Fabien Fleutot
-- @module sched


require 'log'
require 'checks'

local sched = { }; _G.sched = sched

require 'sched.timer'

--
-- if true, scheduling functions are stored in the global environment as well
-- as in table `sched`. For instance, `signal()` is accessible both as
-- `signal()` and as `sched.signal()`.
--
local UNPACK_SCHED = true

--
-- if true, public `sched.*` APIs are provided in all-lowercase spelling, as most
-- standard with generic Lua libraries.
--
local LOWERCASE_ALIASES = true

--
-- When this magic token is passed to a resumed task, it's expected to kill itself
-- with an `error "KILL"`.
-- Used by @{#sched.kill}() to order the current task's suicide, and @{sched.wait}() to perform it.
--
local KILL_TOKEN = setmetatable({ "<KILL-TOKEN>" },
    { __tostring = function() return "Killed thread" end})

--
-- Set to true when parts of `__tasks` need to be cleaned up by @{sched.gc}()
--
local CLEANUP_REQUIRED = false

--
-- This table keeps the state of the scheduler and its threads.
-- It must not be used as a direct way to manipulate tasks, but analysing its
-- content interactively  is an effective way to debug concurrency issues.
--
__tasks       = { }
if   rawget(_G, 'proc') then proc.tasks=__tasks
else rawset(_G, 'proc', { tasks=__tasks}) end

------------------------------------------------------------------------------
-- List of tasks ready to be rescheduled.
-- The first task in the list will be rescheduled first by the next non-nested
-- call to @{sched.run}(). Tasks can be represented:
--
-- * as functions or threads if it is the first time they will be scheduled
-- * as a 2 elements list, thread and args list if rescheduled by a signal.
--
-- Hooks are never listed as ready: they are executed within @{sched.signal}().
------------------------------------------------------------------------------
__tasks.ready = { }

------------------------------------------------------------------------------
-- True when a signal is being processed.
-- Prevents a garbage collector from removing a list of registered cells which
-- are currently being processed.
------------------------------------------------------------------------------
__tasks.signal_processing  = false

------------------------------------------------------------------------------
-- Table of tasks and hooks waiting for signals.
-- Keys are potential signal emitters; values are sub-tables where keys are
-- event names and values are cell lists.
-- A cell holds either a paused task or a hook.
--
-- Hooks have a field `hook` containing the synchronous hook function.
--
-- Tasks have a field `thread` containing the paused coroutine. They have
-- another field `multiwait` set to `true` if the cell is registered to more
-- than one event (can only be set by @{sched.multiWait}()).
--
-- Both cell types have a field `multi` set to `true` if they are registered
-- to more than one event (including event wildcard `"\*"` and timeout callbacks).
--
-- Hook cells may have, in an `xtrargs` field, a list of parameters which will be
-- passed as extra hook parameters just after the triggering event.
--
-- Hook cells may have a `once` field, indicating that they shouldn't be
-- reattached after they caught a signal they were listening to.
--
-- The table is weak in its keys: if a potential event emitter isn't referenced
-- anywhere else, it will be garbage collected, and so will the cells waiting
-- for it to emit a signal.
--
-- @table __tasks.waiting
------------------------------------------------------------------------------
__tasks.waiting = setmetatable({ }, {__mode='k'})

------------------------------------------------------------------------------
--  Holds the coroutine which is currently running or `nil` if no task
--  is currently running.
------------------------------------------------------------------------------
__tasks.running = nil

local getinfo=debug.getinfo
local function iscfunction(f) return getinfo(f).what=='C' end

------------------------------------------------------------------------------
-- Runs a function as a new thread.
--
-- `sched.run(f[, args...])` schedules `f(args...)` as a new task, which will
-- run in parallel with the current one, after the current one yields.
--
-- @function [parent=#sched] run
-- @param f function or task to run
-- @param ... optional arguments to pass to param `f`
-- @return new thread created to run the function
--

function sched.run (f, ...)
    checks ('function')
    local ready = __tasks.ready

    if iscfunction(f) then local cf=f; f=function(...) return cf(...) end end

    local thread = coroutine.create (f)
    local cell   = { thread, ... }
    table.insert (ready, cell)
    log.trace('SCHED', 'DEBUG', "SCHEDULE %s", tostring (thread))

    return thread
end

------------------------------------------------------------------------------
-- Runs all the tasks which are ready to run, until every task is blocked,
-- waiting for an event (i.e. until `__tasks.ready` is empty).
--
-- This function is used by the scheduler's top-level loop, but shouldn't
-- be called explicitly by users.
--
-- @function [parent=#sched] step
--

function sched.step()
    local ptr = __tasks.ready

    -- If scheduling already runs, don't relaunch it
    if __tasks.running or __tasks.hook then return nil, 'already running' end

    --------------------------------------------------------------------
    -- If there are no task currently running, resume scheduling by
    -- going through `__tasks.ready` until it's empty.
    --------------------------------------------------------------------
    while true do
        local cell = table.remove (ptr, 1)
        if not cell then break end
        local thread = cell[1]
        __tasks.running = thread
        log.trace('SCHED', 'DEBUG', "STEP %s", tostring (thread))
        local success, msg = coroutine.resume (unpack (cell))
        if not success and msg ~= KILL_TOKEN then
            -- report the error msg
            print ("In " .. tostring(thread)..": error: " .. tostring(msg))
            print (debug.traceback(thread))
        end
        ---------------------------------------------
        -- If the coroutine died, signal it for those
        -- who synchronize on its termination.
        ---------------------------------------------
        if coroutine.status (thread) == "dead" then
            sched.signal (thread, "die", success, msg)
        end
    end

    __tasks.running = nil

    -- IMPROVE: a better test for idleness is required: maybe there are
    -- TCP data coming at a fast pace, for instance. Putting the cleanup
    -- in a timer, so that it's triggered after a couple seconds of idleness,
    -- would be better.
    if CLEANUP_REQUIRED then sched.gc(); CLEANUP_REQUIRED = false end
end

-- ----------------------------------------------------------------------------
-- Runs a cell:
--
-- * if it has been emptied, leave it alone;
-- * if it is a task, insert the list of appropriate run() args in
--   `wokenup_tasks`;
-- * if it is a hook, run it immediately. If it must be reattached and there is
--   a `new_queue list`, insert it there.
--
-- If `wokenup_tasks` isn't provided, insert tasks in `__tasks.ready` instead.
-- ----------------------------------------------------------------------------
local function runcell(c, emitter, event, args, wokenup_tasks, new_queue)
    local nargs  = args.n

    -- Handle hook attachment-time extra arguments
    if c.xtrargs then
        -- before: args = {ev, a2, a3}, args2={b1, b2, b3}
        -- after:  args = {ev, b1, b2, b3, a2, a3}
        local nxtrargs = c.xtrargs.n
        local newargs = { args[1], unpack(c.xtrargs, 1, nxtrargs) }
        for i=2,nargs do newargs[i+nxtrargs] = args[i] end
        args = newargs
        nargs = nargs + nxtrargs
    end

    if c.multi then
        CLEANUP_REQUIRED = true -- remember to clean pt.waiting
        -- A cell might have a timer plus other events; the timer must be cleaned ASAP anyway
        local timer = c.timer
        if timer then sched.timer.removetimer(timer) end
    end

    local thread = c.thread
    if thread then -- coroutine to reschedule
        local newcell =
            c.multiwait and { thread, emitter, unpack(args, 1, nargs) } or
            { thread, unpack(args, 1, nargs) }
        if not wokenup_tasks then wokenup_tasks = __tasks.ready end
        table.insert(wokenup_tasks, newcell)
        for k in pairs(c) do c[k]=nil end

    elseif c.hook then -- callback is run synchronously
        local function f() return c.hook(unpack(args, 1, nargs)) end
        local ok, errmsg = xpcall(f, debug.traceback)
        local reattach_hook = not c.once
        if ok then -- pass
        elseif errmsg == KILL_TOKEN then -- killed with killself()
            reattach_hook = false
        else -- non-KILL error
            if type(errmsg)=='string' and errmsg :match "^attempt to yield" then
                errmsg = "Cannot block in a hook, consider sched.sigrun()\n" ..
                    (errmsg :match "function 'yield'\n(.-)%[C%]: in function 'xpcall'"
                    or errmsg)
            end
            errmsg = string.format("In signal %s.%s: %s",
                tostring(emitter), event, tostring(errmsg))
            log('SCHED', 'ERROR', errmsg)
            print (errmsg)
        end
        if reattach_hook then
            if new_queue then table.insert (new_queue, c) end
        else for k in pairs(c) do c[k]=nil end end
    else end -- emptied cell, ignore it.
end


------------------------------------------------------------------------------
-- Sends a signal from `emitter` with message `event`, plus optional extra args.
--
-- This means:
--
-- * rescheduling every task waiting for this `emitter.event` signal;
-- * immediately running the hooks listening to this signal;
-- * reattaching the hook cells which don't have a `once` field.
--
-- @function [parent=#sched] signal
-- @param emitter arbitrary Lua object which sends the signal.
-- @param event a string representing the event's kind.
-- @param ... extra args passed to the woken up tasks and triggered hooks.
-- @return nothing.
--

function sched.signal (emitter, event, ...)
    checks('!', '!') -- TODO should we accept non-string events?
    log.trace('SCHED', 'DEBUG', "SIGNAL %s.%s", tostring(emitter), event)
    local args  = table.pack(event, ...) -- args passed to hooks & rescheduled tasks
    local ptw   = __tasks.waiting
    local ptr   = __tasks.running
    local ptrdy = __tasks.ready

    --------------------------------------------------------------------
    -- `emq`: event->tasks_lists table of cells watching events from `emitter`
    -- `queues`: the event queues to parse: the one associated with `event`,
    --           and the wildcard queue `'*'`
    -- `wokenup_tasks`: tasks to reschedule because they were waiting
    --                  for this signal.
    --------------------------------------------------------------------
    local emq = ptw [emitter]; if not emq then return end
    local wokenup_tasks = { }

    local function parse_queue (event_key)
        local old_queue = emq [event_key]
        if not old_queue then return end
        local new_queue = { }
        emq[event_key] = new_queue

        --------------------------------------------------------------------
        -- 1 - accumulate tasks to wake up, run callbacks, list
        --     the callbacks to reattach (others will be lost).
        --------------------------------------------------------------------
        for _, c in ipairs(old_queue) do -- c :: cell to parse
            runcell(c, emitter, event, args, wokenup_tasks, new_queue)
        end

        --------------------------------------------------------------------
        -- 2 - remove expired tasks / hooks / rows
        --------------------------------------------------------------------
        if not next(new_queue) then
            emq [event_key] = nil -- nobody left waiting for this event
            if not next(emq) then -- nobody left waiting for this emitter
                ptw [emitter] = nil
            end
        end
    end

   local was_already_processing = __tasks.signal_processing
   __tasks.signal_processing = true
    parse_queue('*')
    parse_queue(event)
    __tasks.signal_processing = was_already_processing

    --------------------------------------------------------------------
    -- 3 - Actually reschedule the tasks
    --------------------------------------------------------------------
    for _, t in ipairs (wokenup_tasks) do table.insert (ptrdy, t) end

end

------------------------------------------------------------------------------
-- Helper to let @{sched.sigHook}, @{sched.sigRun}, @{sched.sigRunOnce} etc.
-- register and be notified when a signal of interest occurs.
--
-- @usage
--
-- `register(cell, timeout)` registers for a timer event after the specified
-- time elapsed (in seconds).
--
-- `register(cell, emitter, event_string)` is an admissible shortcut for
-- `register(cell, emitter, {event_string})`.
--
-- `register(cell, timeout_number)` is an admissible shortcut for
-- `register(cell, "timeout", {timeout_number})`.
--
-- @param cell entry to register
-- @param emitter of the signal of interest
-- @param events list of events to register, or `"*"` for every event;
--        the list can feature a timeout number.
-- @return the list of events subscribed
--
------------------------------------------------------------------------------
local function register (cell, emitter, events)

    --------------------------------------------------------------------
    -- 1 - Set emitters and events. events still contain timer events
    --------------------------------------------------------------------
    if events==nil and type(emitter)=='number' then
        emitter, events = nil, { emitter }
    elseif emitter==nil then
        error 'signal emitters cannot be nil'
    end
    if #events > 1 then cell.multi = true end

    local ptw  = __tasks.waiting
    local ptwe = ptw[emitter]
    if not ptwe then ptwe={ }; ptw[emitter]=ptwe end

    --------------------------------------------------------------------
    -- 2 - register timeout callbacks
    --------------------------------------------------------------------
    local hastimeout = false
    for _, event in ipairs(events) do
        if type(event)=='number' then
            if hastimeout then error("Several timeouts for one signal registration") end
            local function timeout_callback()
                if next(cell) then
                    runcell(cell, emitter, 'timeout', { 'timeout', event, n=2 })
                    CLEANUP_REQUIRED = true -- Cannot remove cell from waiting list
                end
            end
            hastimeout  = true
            local delay = event
            local ev, timer = sched.timer.set(delay, emitter, timeout_callback)
            cell.timer = timer -- so that it can be released when the cell is run or killed
            local ptwet = ptwe.timeout
            if ptwet then table.insert(ptwet, cell) else ptwe.timeout={cell} end
            log.trace('SCHED', 'DEBUG', "Registered cell for %ds timeout event", event)
        end
    end

    --------------------------------------------------------------------
    -- 3 - register non-timer events
    --------------------------------------------------------------------
    -- Retrieve the row, or create it if it doesn't already exist
    if emitter then
        local emq = ptw[emitter]
        if not emq then emq = { }; ptw[emitter] = emq end
        for _, event in ipairs(events) do
            if type(event) ~= 'number' then
                -- Retrieve the cell in the row, or create it if required
                local evq = emq [event]
                if not evq then emq[event] = { cell }
                else table.insert (evq, cell) end
            end
        end
    end
    return events
end



------------------------------------------------------------------------------
--  Forces the currently running task out of scheduling until a certain
--  signal is received.
--
--  Take a list of emitters, and a list of events. The task will be
--  rescheduled the next time one of the listed events happens to one of the
--  listed emitters. If there is a single emitter (resp. a single event),
--  it can be passed outside of a table, i.e.
--      wait(x, "open")
--  is the same as
--      wait({x}, {"open"})
--
--  If no emitter or no event is listed, the task waits for nothing in
--  particular, i.e. it puts itself back at the end of the scheduling
--  queue, thus giving other threads a chance to run.
--
--  There must be a task currently running, i.e.
--  @{__tasks.running} ~= nil.
--
--  Cf. description of @{__tasks.waiting} for a description of how tasks
--  are put to wait on signals.
--
-- @function [parent=#sched] wait
-- @param emitter table listing emitters to wait on. Can also be string to define
--  single emitter, or a number to specify a timeout in seconds (wait used as sleep function)
-- @param varargs optional varargs: can be events list in a table, or several events in
--  several arguments. Last event can be a number to specify a timeout call.
-- @return the event and extra parameters of the signal which unlocked the task.
--
-- @usage
--
--     --reschedules current task, giving other tasks a chance to run:
--     sched.wait()
--
--     -- blocks the current task for `timeout` seconds:
--     sched.wait(timeout)
--
--     -- waits until `emitter` signals this `event`:
--     sched.wait(emitter, event)
--
--     -- waits until `emitter` signals any event:
--     sched.wait(emitter, "*")
--
--     -- waits until `emitter` signals any one of the `eventN` in the list:
--     sched.wait(emitter, {event1, event2, ...})
--
--     -- waits until `emitter` signals any one of the `eventN` in the list,
--     -- or `timeout` seconds have elapsed, whichever occurs first:
--     sched.wait(emitter, {event1, event2, ...timeout})
--
--     -- admissible shorcut for `sched.wait(emitter, {event1, event2...}):
--     sched.wait(emitter, event1, event2, ...)
--

function sched.wait (emitter, ...)
    local current = __tasks.running or
        error ("Don't call wait() while not running!\n"..debug.traceback())
    local cell = { thread = current }
    local nargs = select('#', ...)

    if (emitter==nil and nargs == 0) or (type(emitter) == "number" and emitter == 0) then -- wait()
        log('SCHED', 'DEBUG', "Rescheduling %s", tostring(current))
        table.insert (__tasks.ready, { current })
    else
        local events
        if nargs==0 then -- only makes sense for wait(number)
            checks('number')
            emitter, events = '*', {emitter}
        elseif nargs==1 then
            events = (...)
            if type(events)~='table' then events={events} end
        else -- nargs>1, wait(emitter, ev1, ev2, ...);
            events = {...}
        end

        register(cell, emitter, events)

        if log.musttrace('SCHED', 'DEBUG') then -- TRACE:
            local ev_msg = { }
            for i=1, #events do ev_msg[i] = tostring(events[i]) end
            local msg =
                "WAIT emitter = " .. tostring(emitter) ..
                ", events = { " .. table.concat(ev_msg,", ") .. " } )"
            log.trace('SCHED', 'DEBUG', msg)
        end -- /TRACE
    end

    --------------------------------------------------------------------
    -- Yield back to step() *without* reregistering in __tasks.ready
    --------------------------------------------------------------------
    __tasks.running = nil
    local x = { coroutine.yield () }

    if x and x[1] == KILL_TOKEN then
        for k in pairs(cell) do cell[k]=nil end; error(KILL_TOKEN)
    else return unpack(x) end
end

------------------------------------------------------------------------------
-- Waits on several emitters.
-- Same as @{#sched.wait}, except that:
--
--  * the first argument is a list of emitters rather than a single emitter;
--  * it returns `emitter, event, args...` instead of just `event, args...`;
--    indeed, the caller might want to know which emitter rescheduled it;
--  * it's illegal not to enclose events in a list.
--
-- @function [parent=#sched] multiWait
-- @param emitters table containing a list of the emitters to wait on
-- @param events table containing a list of the events to wait on, or a string
--  describing an event's kind, or a number defining timeout for this call.
-- @return emitter, event, args that caused this call to end.
--

function sched.multiWait (emitters, events)
    checks('table', 'string|table|number')
    local current = __tasks.running or
        error ("Don't call wait() while not running!\n"..debug.traceback())
    if type(events)~='table' then events={events} end

    local cell = { thread=current, multiwait=true, multi=true }
    for _, emitter in ipairs(emitters) do
        register(cell, emitter, events)
    end

    if log.musttrace('SCHED', 'DEBUG') then -- TRACE:
        local em_msg = { }
        for i=1, #emitters do em_msg[i] = tostring(emitters[i]) end
        local ev_msg = { }
        for i=1, #events do ev_msg[i] = tostring(events[i]) end
        local msg =
            "WAIT emitters = { " .. table.concat(em_msg,", ") ..
            " }, events = { " .. table.concat(ev_msg,", ") .. " } )"
        log.trace('SCHED', 'DEBUG', msg)
    end -- /TRACE

    --------------------------------------------------------------------
    -- Yield back to step() *without* reregistering in __tasks.ready
    --------------------------------------------------------------------
    __tasks.running = nil
    local x = { coroutine.yield () }

    if x and x[1] == KILL_TOKEN then
        for k in pairs(cell) do cell[k]=nil end; error(KILL_TOKEN)
    else return unpack(x) end
end


------------------------------------------------------------------------------
-- Hooks a callback function to a set of signals.
--
-- Signals are described as for @{#sched.wait}. See this function for more
-- details.
--
-- The callback is called synchronously as soon as the corresponding
-- signal is emitted, and every time it is emittes. That is, the function
-- `f` will have returned before the call to @{#sched.signal} which triggered
-- it returns.
--
-- This puts a constraint on `f`: it must not block and try to reschedule itself.
-- If a hook function calls a blocking API, it will trigger an error.
--
--  The hook will receive as arguments the triggering event, and any extra params
--  passed along with the signal.
--
-- @function [parent=#sched] sigHook
-- @param emitter list of signal emitters to watch or a string describing
--  single emitter to watch
-- @param events events to watch from the emitters: a table containing a list
--  of the events to wait on, a string discribing an event's kind,
--  or a number defining timeout for this call.
-- @param f function to be used as hook
-- @param varargs extra optional params to be given to hook when called
-- @return registered hook, which can be passed to @{#sched.kill} to cancel
--  the registration.
--

function sched.sigHook (emitter, events, f, ...)
    checks ('!', 'string|table|number', 'function')
    local xtrargs = table.pack(...); if not xtrargs[1] then xtrargs=nil end
    local cell = { hook = f, xtrargs=xtrargs }
    if type(events)~='table' then events={events} end
    register (cell, emitter, events)
    return cell
end

------------------------------------------------------------------------------
-- Hooks a callback function to a set of signals, to be triggered only once.
--
-- This function has the same API and behavior as @{#sched.sigHook},
-- except that the hook will only be run once: it detaches itself from
-- all of its registrations when it's first triggered.
--
-- @function [parent=#sched] sigOnce
-- @param emitter a list of signal emitters to watch or a string describing
--  a single emitter to watch
-- @param events events to watch from the emitters: a table containing a list
--  of the events to wait on, a string describing an event's kind,
--  or a number defining timeout for this call.
-- @param f function to be used as hook
-- @param varargs extra optional params to be given to hook when called
-- @return registred hook
--

function sched.sigOnce (emitter, events, f, ...)
    checks ('!', 'string|table|number', 'function')
    local xtrargs = table.pack(...); if not xtrargs[1] then xtrargs=nil end
    local cell = { hook = f, once=true, xtrargs=xtrargs }
    if type(events)~='table' then events={events} end
    register (cell, emitter, events)
    return cell
end

-- Common helper for sigrun and sigrunonce
local function sigrun(once, emitter, events, f, ...)
    local xtrargs = table.pack(...); if not xtrargs[1] then xtrargs=nil end
    local cell, hook
    -- To ensure that a killself() in the task also kills
    -- the wrapping hook, attach this to the task's 'die' event.
    -- TODO: ensure that the hook cannot leak by leaving a zombie event
    local function propagate_killself(die_event, status, err)
        if not status and err==KILL_TOKEN then sched.kill(cell) end
    end
    local function hook (ev, ...)
        local t = sched.run(f, ev, ...)
        if not once then
            sched.sigonce(t, 'die', propagate_killself)
        end
    end
    cell = { hook=hook, once=once, xtrargs = xtrargs }
    if type(events)~='table' then events={events} end
    register (cell, emitter, events)
    return cell
end

------------------------------------------------------------------------------
-- Hooks a callback function to a set of signals.
--
-- This function has the same API as @{#sched.sigHook}, except that `f` runs
-- in a separate thread. The consequences are:
--
-- * `f` is not run immediately, but after the task which called the triggering
--   signal rescheduled;
-- * `f` is allowed to call blocking APIs.
--
-- @function [parent=#sched] sigRun
-- @param emitter a signal emitter to watch
-- @param events events to watch from the emitters: a table containing a list
--  of the events to wait on, a string describing an event's kind,
--  or a number defining timeout for this call.
-- @param f function to be used as hook
-- @param varargs extra optional params to be given to hook when called
-- @return registered hook, which can be passed to @{#sched.kill} to cancel
--  the registration.
--

function sched.sigRun(emitter, events, f, ...)
    checks ('!', 'string|table|number', 'function')
    return sigrun(false, emitter, events, f, ...)
end
------------------------------------------------------------------------------
-- Hooks a callback function to a set of signals. The hook will be called
-- in a new thread (allowing the hook to block), and only one time.
--
-- This function has the same API and behavior as @{#sched.sigRun},
-- except that the hook will only be run once: it detaches itself from
-- all of its registrations when it's first triggered.
--
-- @function [parent=#sched] sigRunOnce
-- @param emitters a signal emitter to watch
-- @param events events to watch from the emitters: a table containing a list
--  of the events to wait on, a string describing an event's kind,
--  or a number defining timeout for this call.
-- @param f function to be used as hook
-- @param varargs extra optional params to be given to hook when called
-- @return registered hook, which can be passed to @{#sched.kill} to cancel
--  the registration.
--

function sched.sigRunOnce(emitter, events, f, ...)
    checks ('!', 'string|table|number', 'function')
    return sigrun(true, emitter, events, f, ...)
end

------------------------------------------------------------------------------
-- Does a full Garbage Collect and removes dead tasks from waiting lists.
--
-- Dead tasks are removed when the expected event happens or when the expected
-- event emitter dies. If that never occurs, and you still want to claim
-- the memory associated with these dead tasks, you can always call this
-- function and it will remove them.
--
-- There's usually no need to call this function explicitly, it will be triggered
-- automatically when it's needed and the scheduler is about to go idle.
-- @function [parent=#sched] gc
-- @return memory available (in number of bytes) after gc.
--

function sched.gc()
    -- Design note: no need to do that on the `__tasks.ready` list: it
    -- auto-cleans itself when `run`() goes through it.                   --
    local costatus = coroutine.status
    local not_processing, ptw = not __tasks.signal_processing, __tasks.waiting

    -- Getting rid of entries waiting for a dead thread / dead channel
    for emitter, events in pairs(ptw) do
        for event, event_queue in pairs(events) do
            local i, len = 1, #event_queue
            while i<=len do
                local cell = event_queue[i]
                if not next(cell) -- dead cell
                or cell.thread and costatus(cell.thread)=="dead" -- dead thread
                then table.remove(event_queue, i); len=len-1 else i=i+1 end
            end
            if not_processing and not next(event_queue) then events[event] = nil end  -- event with empty queue
        end
        -- remove emitter if there's no pending event left:
        if not_processing and not next(events) then ptw[emitter] = nil end
    end

    collectgarbage 'collect'
    return math.floor(collectgarbage 'count' * 1000)
end

------------------------------------------------------------------------------
-- Kills a task.
--
-- Implementation principle: the task is killed by either
--
--  * making it send a `KILL_TOKEN` error if it is currently running
--  * waking it up from a @{#sched.wait} yielding with `KILL_TOKEN` as an argument,
--    which in turn makes @{#sched.wait} send a `KILL_TOKEN` error.
--
-- @function [parent=#sched] kill
-- @param x task to kill, as returned by @{#sched.sigHook} for example.
-- @return `nil` if it killed another task,
-- @return never returns if it killed the calling task.
--

function sched.kill (x)
    local tx = type(x)
    if tx=='table' then
        local timer = x.timer -- if there's an associated timer event, clean it up
        if timer then sched.timer.removetimer(timer) end
        if x.hook then
            -- Cancel a hook
            for k in pairs(x) do x[k]=nil end
            CLEANUP_REQUIRED = true
        elseif not next(x) then -- emptied cell
            log('SCHED', 'DEBUG', "Attempt to kill a dead cell")
        else
            log('SCHED', "ERROR", "Don't know how to kill %s", tostring(x))
        end
    elseif x==__tasks.running then
        -- Kill current thread
        error (KILL_TOKEN)
    elseif tx=='thread' then
        -- Kill a non-running thread
        coroutine.resume (x, KILL_TOKEN)
        sched.signal (x, "die", "killed")
    else
        log('SCHED', "ERROR", "Don't know how to kill %s", tostring(x))
    end
end

------------------------------------------------------------------------------
-- Kills the current task.
-- @function [parent=#sched] killSelf
-- @return never returns, since the task is killed.
--

function sched.killSelf()
    error (KILL_TOKEN)
end

------------------------------------------------------------------------------
-- For some non-technical reason, we need the public API to be CamelCased.
-- This loop aliases CamelCased names into more standard lowercase ones.
-- This is performed in two steps: `pairs(t)` expects `t`'s hash-part keys
-- to remain constant while the `for` loop is running.
------------------------------------------------------------------------------
if LOWERCASE_ALIASES then
    local aliases = { }
    for camelCaseName, value in pairs (sched) do
        local lowercasename = camelCaseName :lower()
        if lowercasename ~= camelCaseName then aliases[lowercasename] = value end
    end
    for k, v in pairs(aliases) do sched[k] = v end
end

------------------------------------------------------------------------------
-- Export sched content if applicable. This must occur after lowercase aliasing,
-- if we want lowercase variants to be exported.
------------------------------------------------------------------------------
if UNPACK_SCHED then
    for k, v in pairs(sched) do
        rawset (_G, k, v)
    end
end


-- Load platform-dependent code
require 'sched.platform'

return sched
