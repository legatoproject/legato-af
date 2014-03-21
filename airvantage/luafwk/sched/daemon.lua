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

-------------------------------------------------------------------------------
-- Daemon handling module.
-- Daemon are similar to threads and hooks;
-- the difference is, if they die or fail, they are automatically restarted.
-------------------------------------------------------------------------------

local M = { }; daemon = M

-------------------------------------------------------------------------------
-- Daemon metatable, to allow easy type checking.
-------------------------------------------------------------------------------
local dmt = { __type='daemon' }

-------------------------------------------------------------------------------
-- List of daemons to kill with daemon.kill(). Appearing in this set's keys
-- prevents the thread/hook from relaunching/reattaching itself.
-------------------------------------------------------------------------------
M.tokill = { }

-------------------------------------------------------------------------------
-- Works as sched.run(), except that the resulting thread is registered
-- as a daemon and will be relaunched if it dies.
--
-- An optional first string arument gives a name to the daemon, thus allowing
-- better log messages to be generated upon the daemon's noteworthy
-- lifecycle events.
--
-- An optional second number argument represents a delay before relaunching
-- the thread: if run(10, f, x, y) is called, then f(x, y) will be launched
-- as a separate task, and every time it fails or terminate, the daemon system
-- will wait for 10 seconds before relaunching it.
-------------------------------------------------------------------------------
function M.run (name, delay, f, ...)
    if type(name) ~= 'string' then return M.run ("'?'", name, f, ...) end
    local f_args = { f, ... }
    local cell, delay
    if type(f)=='number' then
        checks('string', 'number', 'function')
        delay=f; f=table.remove (f_args, 1)
    else
        checks('string', 'function')
    end
    local function supertask()
        while true do
            local status, r = copcall (unpack (f_args))
            if M.tokill[cell] then
                log('SCHED-DAEMON', 'INFO', "Daemon thread %s killed.", name)
                M.tokill[cell] = nil
                return
            elseif not status then log('SCHED-DAEMON', 'WARNING',
                "Relaunch daemon thread %s which failed with error %q", name, r)
            else log('SCHED-DAEMON', 'INFO',
                "Relaunch daemon thread %s which terminated without error", name)
            end
            if delay then
                log('SCHED-DAEMON', 'INFO', "Wait %i seconds before relaunching", delay)
                sched.wait(delay)
            end
        end
    end
    cell = sched.run (supertask)
    setmetatable(cell, dmt)
    return cell
end

-------------------------------------------------------------------------------
-- Works as sched.kill(), except that it only accepts daemons, and daemons
-- killed with this function will not be relaunched at the next check.
-------------------------------------------------------------------------------
function M.kill (r)
    checks ('daemon')
    M.tokill[r] = true
    kill(r)
    M.tokill[r] = nil
end

return M
