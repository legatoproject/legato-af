--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------


local sched     = require 'sched'
local socket    = require 'socket'
local loader    = require 'utils.loader'
local checks    = require 'checks'
local treemgr   = require 'agent.treemgr'
local log       = require 'log'
local timer     = require 'timer'
local tmproxy   = require 'agent.treemgr.table'
local lfs       = require 'lfs'
local os        = require 'os'
local persist   = require 'persist'
local utilst    = require 'utils.table'
local table     = require 'table'
local system    = require 'utils.system'

local type = type
local loadstring = loadstring
local loadfile = loadfile
local error = error
local assert = assert
local setmetatable = setmetatable
local setfenv = setfenv
local getfenv = getfenv
local copcall = copcall
local pcall = pcall
local ipairs = ipairs
local pairs = pairs
local tonumber = tonumber
local _G = _G


local _M = {}
-- TODO: add local before all the following variables !
setupenv = {} -- environement in wich are run the monitoring script setup
execenv = setmetatable({}, {__index=_G})  -- environement in wich are running the monitoring actions
onboothooks = {}  -- a table (not a list: keys are 'random indexes', values are hooks) of hooks that are called a little after the monitoring module is initialized
--onconnect trigger is deactivated until a proper srvcon architecture allows a propoer implementation
--onconnecthooks = {} -- a table (not a list: keys are 'random indexes', values are hooks) of hooks that are called before a server connection is done

store_path = nil -- path where monitoring scripts are installed
installed_scripts = nil -- persisted table of installed scripts: key:rule name, value:activated flag
loaded_scripts = nil -- table containing the loaded scripts environements
-- End of 'local' TODO

-- Enable direct access to tree variables
execenv.vars = tmproxy   


--------------------------------------------------------------------------------------------
-- Monitoring Engine internal functions
--------------------------------------------------------------------------------------------

-- Check if the actions are filtered or executed
-- When sereval filter are in use, the output is a logical AND between all sub-filters, the test
-- finishes when an individual filter returns false (or fail)
local function testfilters(hooks)
    local function test(filter)
        local s, v = pcall(filter)
        if s and v then return true
        elseif not s then log("MONITORING", "ERROR", "Error while executing a monitoring filter: %s", v) end
        return false
    end

    local filters = hooks.filter
    if filters then
        if type(filters) == "function" then
            return test(filters)
        else
            for _, f in ipairs(filters) do if not test(f) then return false end end
        end
    end
    return true
end

-- Call the hooks from the table (regardless of filters)
-- Call the hooks in an ordered manner
local function callhooks(hooks, ...)
    for _, f in ipairs(hooks) do
        local s, e = copcall(f, ...)
        if not s then log("MONITORING", "ERROR", "Error while executing a monitoring action: %s", e) end
    end
end

-- Return a function that will execute 'actions' unless filtered
-- Simple functor that is used several times. This prevent from declaring many times the same anonymous function (save space...)
local function actioncaller(actions)
    local function f(...)
        if testfilters(actions) then -- execute the filtering function synchronously
            sched.run(callhooks, actions, ...)
        end
    end
    return f
end


-- Return the environement of the monitoring rule/script
-- This currently uses getfenv, but could be improved if performances were to be an issue
local function scriptenv()
    local env
    local level = 3
    while true do
        env = getfenv(level)
        if _G ~= env and env.__scriptname then break end -- make sure to skip _G as strict.lua make that table access fails
        level = level + 1
    end
    
    return env
end



--------------------------------------------------------------------------------------------
-- Monitoring Script Setup functions
--------------------------------------------------------------------------------------------

----------------------
-- Triggers


-- Activated when one of the variable from the list changes
-- ... is a variable number of variable to monitor, named as string: ex "system.batterylevel", "user.somevar", ....
-- the first argument can also be a table containing all the variables, in that case the remaining args are ignored
--      Note: the action functions will be given a table containing the changed variables and their value as first argument
function setupenv.onchange(...)
    local vars = ...
    if type(vars) ~= 'table' then vars = {...} end
    assert(#vars >= 1, "must hook on one or more variables")
    
    local actions = {}
    local tm = assert(treemgr.register(nil, vars, actioncaller(actions)))
    table.insert(scriptenv().__unload, tm) -- save registration id for a future unloading
    return actions
end

-- Activated when none of the listed variables change for the timeout amount of time
--    if timeout > 0 then once activated, the trigger is re-armed only when a variable changes.
--    if timeout < 0 then the trigger is automatically re-armed
-- ... is a list of variable as strings.
--      Note: the action functions will be given a table containing the variables and the value that caused the last reaem of the holding timer, as first argument
function setupenv.onhold(timeout, ...)
    checks('number')
    local last
    local actions = {}
    local function activate() actioncaller(actions)(last) end
    local t = timer.new(tonumber(timeout), activate)
    local tm = assert(treemgr.register(nil, {...}, function(v) t:rearm() last=v end))
    local unload = scriptenv().__unload
    table.insert(unload, t)
    table.insert(unload, tm)
    return actions
end

-- Activated on a periodic timer
-- a value >0 means a periodic timer
-- a value <= 0 means a one shot timer...
function setupenv.onperiod(period)
    checks('number')
    local actions = {}
    local tm = assert(timer.new(-period, actioncaller(actions))) -- start a periodic timer
    table.insert(scriptenv().__unload, tm)
    return actions
end

-- Activated on a date. This date is specified with a cron entry (see timer.cron documetnation for format)
function setupenv.ondate(cron)
    checks('string')
    local actions = {}
    local t = timer.cron(cron, actioncaller(actions)) -- start a cron timer
    table.insert(scriptenv().__unload, t)
    return actions
end

-- Activated a few seconds after the Monitoring module is initialized
function setupenv.onboot()
    local actions = {}
    local idx = #onboothooks+1
    onboothooks[idx] = actions
    table.insert(scriptenv().__unload, {otherhooks=onboothooks, idx = idx})
    return actions
end


--onconnect trigger is deactivated until a proper srvcon architecture allows a propoer implementation
-- Activated before the agent connects to the server 
-- function setupenv.onconnect()
-- duplicate onboothooks !
-- end


-- Activated when a value traverse a threshold (previous and new value are opposite side of the threshold value)
-- when edge is specified it can be one of "up" or "down" meaning only triggering on rising edge or falling edge
--      an "up" edge is detected when oldval < threshold and newval >= threshold
--      a "down" edge is detected when oldval >= threshold and newval < threshold
--      the default is "both" edge
function setupenv.onthreshold(threshold, var, edge)
    checks('number', 'string', '?string')
    edge = edge or "both"
    assert(edge=="up" or edge=="down" or edge=="both", "edge parameter is not valid")
    
    local side = (tonumber(treemgr.get(nil, var)) or 0) < threshold
    local up = edge=="up" or edge=="both"
    local down = edge=="down" or edge=="both"
    local function test()
        local newval = tonumber(treemgr.get(nil, var)) or 0
        if (newval>=threshold and side) or (newval<threshold and not side) then
            local r = (up and side) or (down and not side)
            side = newval < threshold
            return r
        else return false end
    end
    return setupenv.filter(test, setupenv.onchange(var))
end


-- Activated when a value goes outside a limited range: activated if abs(newval-oldval) >= deadband
-- oldval is updated when the trigger is activated
function setupenv.ondeadband(deadband, var)
    checks('number', 'string')

    local oldval = tonumber(treemgr.get(nil, var)) or 0
    local function test()
        local newval  = tonumber(treemgr.get(nil, var)) or 0
        local d = math.abs(newval - oldval)
        if d >= deadband then
            oldval = newval
            return true
        else return false end
    end
    return setupenv.filter(test, setupenv.onchange(var))
end




----------------------
-- Filter

-- Filter a trigger given a test function
-- test: a function that returns a boolean value: true=>the actions are executed
-- actions: output of a trigger function.
-- Example: connect(filter(function() return not system.onpower end, onchange("system.batterylevel")), function() print("Battery Level Changed") end)
--      will only print when battery level changes and the power cord is unplugued.
function setupenv.filter(test, trigger)
    checks('function', 'table')

    local actions = trigger
    if not actions.filter then
        actions.filter = test
    elseif type(actions.filter) == "function" then
        local f = actions.filter
        actions.filter = {f, test}
    else
        table.insert(actions.filter, test)
    end
    return actions
end


----------------------
-- Connector function

function setupenv.connect(trigger, action)
    checks('table','function')
    
    if trigger[action] then return end -- connection can only be done once for the same couple of action trigger
    trigger[action] = true
    table.insert(trigger, action)
end





-------------------------------------------------------------------------------
-- Monitoring module API
-------------------------------------------------------------------------------

-- Load a monitoring script in memory.
-- name is a string name
-- script can be either
--              a string containing a monitoring script
--              a table string buffer (list of chunk of string)
--              or an actual Lua function containing the script
function _M.load(name, script)
    checks("string", "string|table|function")
        
    local err
    local t = type(script)
    if t == "function" then -- nothing to do the function will be called below
    elseif t == "string" then script, err = loadstring(script, name)
    elseif t == "table" then script, err = loader.loadbuffer(script, name, true) end
    if not script then return nil, err end
    
    local env = setmetatable({__scriptname=name, __unload={}}, {__index=setupenv})
    setfenv(script, env)
    local s, err = copcall(script)
    if not s then return nil, err end
    setmetatable(env, {__index=execenv})
    _M.unload(name) -- unload a possible already existing script
    loaded_scripts[name] = env
    return "ok"
end

-- Unload (and stop) a monitoring script from memory.
-- name is a string name, given at loading time (see load() )
function _M.unload(name)
    checks('string')
    
    local env = loaded_scripts[name]
    if not env then return nil, "script not loaded" end
    
    for _, r in pairs(env.__unload) do
        if r.monitored_lpath_set then -- dirty way to detect that it is a tree manager registration
            treemgr.unregister(r)
        elseif r.nextevent then -- dirty way to detect that it is a timer registration
            timer.cancel(r)
        elseif r.otherhooks then
            r.otherhooks[r.idx] = nil
        else error("a ressource cannot be freed, please review monitoring module") end
--         env.__unload[_] = nil
    end
    --collectgarbage("collect") -- TODO remove
    --print(collectgarbage("count"))
--     for k,v in pairs(env) do env[k] = nil end
    loaded_scripts[name] = nil
    --collectgarbage("collect")
    --print(collectgarbage("count"))
    return "ok"
end

local function loadscriptfile(name, scriptfile)
    log('MONITORING', 'DETAIL', "Loading monitoring script [%s]", name)
    local f, err = loadfile(scriptfile)
    if not f then return nil, err
    else return _M.load(name, f) end
end


-- Install a monitoring script from a file. Installing a script with an already used name will cause the
-- initial script to be unloaded and uninstalled (replaced by this script)
--      name is the name given to the script
--      filename is the path of the file to install
--      enable is a boolean. true means that the script will enabled (started)
function _M.install(name, filename, enable)
    checks('string', 'string', '?boolean')
    
    if lfs.attributes(filename, "mode") ~= 'file' then return nil, filename.." is not a file, or cannot be accessed" end
    local scriptfile = store_path..name..".rule"
    assert(os.execute("cp -f "..filename.." "..scriptfile) == 0, "Error while copying the file")
    
    installed_scripts[name] = false
    return _M.enable(name, enable and true or false)
end


-- Uninstall a monitoring script.
-- This function automatically stop the given script if it was enabled (running).
--      name is the name given at installation time.
function _M.uninstall(name)
    checks('string')
    
    local scriptfile = store_path..name..".rule"
    os.execute("rm -f "..scriptfile)
    installed_scripts[name] = nil
    _M.unload(name) -- in case the script is loaded, will not do anything otherwise
    
    return "ok"
end

-- Set the enable flag on an already installed script.
-- Enabling a disabled script will cause a loading, and similarly, disabling an enabled
-- script will cause an unloading.
--      name is the name of the script to configure
--      set is a boolean: true will enable the script, false will disable the script
function _M.enable(name, set)
    checks('string', 'boolean')
    
    local status = installed_scripts[name]
    
    if  status == nil then return nil, "script not installed"
    elseif status and not set then _M.unload(name)
    elseif not status and set then 
        local scriptfile = store_path..name..".rule"
        local s, r = loadscriptfile(name, scriptfile)
        if not s then return nil, err end
    end
    installed_scripts[name] = set
    
    return "ok"
end

-- List installed or loaded scripts.
--      type: if type is set to "installed", list the installed scripts
--            if type is set to "loaded", list the loaded scripts
function _M.list(type)
    local t
    if type==nil or type == "installed" then t = installed_scripts
    elseif type == "loaded" then t = loaded_scripts
    else return nil, "not a valid list type" end
    
    return utilst.keys(t)
end


-- Module initialization function
function _M.init()
    
    -- Load the persisted script list
    installed_scripts = persist.table.new("MonitoringScriptList")
    loaded_scripts = {}
    
    -- Setup installation path
    store_path = (LUA_AF_RW_PATH or "./").."monitoring/"
    system.mktree(store_path)
    
    -- Load predefined monitoring scripts, if any
    local resources_path = (LUA_AF_RO_PATH or "./").."resources/monitoring/"
    if lfs.attributes(resources_path, "mode") ~= "directory" then log('MONITORING', 'DETAIL', "No predefined monitoring scripts to load in %s", resources_path)
    else
        for m in lfs.dir (resources_path) do
            if m:sub(-5, -1) == '.rule' then
                local fm = resources_path..m
                m = m:sub(1, -6)
                local s, err = loadscriptfile("predef_"..m, fm)
                if not s then log('MONITORING', 'ERROR', "Error when loading predefined monitoring script %s, err=%s", m, err) end
            end
        end
    end
    
    -- Load installed and enabled scripts
    for name, enabled in utilst.sortedpairs(installed_scripts) do
        if enabled then
            local scriptfile = store_path..name..".rule"
            local s, err = loadscriptfile(name, scriptfile)
            if not s then log('MONITORING', 'ERROR', "Error when loading installed monitoring script %s, err=%s", name, err) end
        end
    end

    -- Trigger the onboot event, 30 seconds after the initialization
    sched.sigonce("*", 30, function() for _, actions in pairs(onboothooks) do actioncaller(actions)() end end)

    return "ok"
end



return _M