-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require "sched"
local logstore = require "log.store"
local log = require "log"
local unpack = unpack
local assert = assert
local tostring = tostring
local pairs = pairs

module(...)

local init_func = { }


local levels = log.levels

----------------------
-- log policy that stores:
-- * all logs in ram
-- * only ERROR or WARNING logs in flash (no context)
----------------------

local function logpolicy_sole(ram, min_level, module, severity, s)
    s=s.."\n"
    if ram then logstore.logramstore(s) end
    if levels[severity] and levels[min_level] >= levels[severity] then logstore.logflashstore(s) end
end

function init_func.sole(config)
  if config.ramlogger then assert(logstore.lograminit(config.ramlogger)) end
  assert(logstore.logflashinit(config.flashlogger))
  log.storelogger = function (module, severity, s) logpolicy_sole(config.ramlogger ~= nil, config.level or "WARNING", module, severity, s) end
end

----------------------
-- log policy that stores:
-- * all logs in ram
-- * all logs in flash too: store in flash ram buffer content when ram buffer is full.
----------------------


local function logpolicy_buffered_all_hook(ev)
  local err_ctx = logstore.logramget()
  logstore.logflashstore(err_ctx)
end

local function logpolicy_buffered_all(module, severity, s)
    logstore.logramstore(s.."\n")
end


-- specific init needed here
function init_func.buffered_all(config)
  assert(logstore.lograminit(config.ramlogger))
  assert(logstore.logflashinit(config.flashlogger))
  sched.sighook("logramstore", "erasedata", logpolicy_buffered_all_hook)
  log.storelogger = logpolicy_buffered_all
end


----------------------
-- log policy that stores:
-- * all logs in ram
-- * use logram as context buffer, store in flash using logram signal.
----------------------



local function logpolicy_context(min_level, module, severity, s)
    --always store in ram
    logstore.logramstore(s.."\n")
    if levels[severity] and levels[min_level] >= levels[severity] then
        -- get context from ram
        local err_ctx = logstore.logramget()
        if err_ctx then logstore.logflashstore(err_ctx) end
    end
end

function init_func.context(config)
  assert(logstore.lograminit(config.ramlogger))
  assert(logstore.logflashinit(config.flashlogger))
  log.storelogger = function (module, severity, s) logpolicy_context(config.level or "ERROR", module, severity, s) end
end


--external init function fun
--takes as parameter a table with log tools config
--this tables must have *name* parameter corresponding to one of the log policies,
--plus parameters used to init low level storing api (ramlogger and flashlogger)
--e.g.: log_config = {name="sole", params={level = "WARNING", ramlogger = {size=2048}, flashlogger={size=2048} } }
-- ->init(log_config)
--successive init call are not supported because:
--lograminit and logflashinit may not support successive init calls.
--To be checked
function init(config)
    assert(config.name, "must give a log policy name")
    assert(init_func[config.name], "no existing policy found for "..tostring(config.name))
    assert(config.params, "must give a params table in log config")
    init_func[config.name](config.params)
end


