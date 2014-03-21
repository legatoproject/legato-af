-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local require = require
if os.getenv 'LUADEBUG' then require 'debugger' ('localhost') end
local sched = require "sched"
require "strict"

--load platform specifics
local platform = require "agent.platform"
local print = print
local log = require "log"
local socket = require "socket"
local loader = require "utils.loader"
require "coxpcall"
local pairs = pairs
local ipairs = ipairs
local next = next
local string = string
require "agent.versions"
local setmetatable = setmetatable
local table = table
local type = type
local protect = socket.protect

--register to SIGTERM
local psignal = require "posixsignal"
psignal.signal("SIGTERM", function()
    local system = require "agent.system"
    system.stop(2)
    end)

--check if migration if needed----------------------------
local function setupmigration()
    local persist = require "persist"
    return persist.load("AgentVersion")
end
local function loadmigration(versionfrom)
    local migrationscript = require "agent.migration"
    migrationscript.execute(versionfrom, _MIHINI_AGENT_RELEASE)
    loader.unload("agent.migration")
end
local status, currentversion = copcall(setupmigration)
currentversion = status and currentversion or "unknown"
local migrationres, migrationerr
if currentversion ~= _MIHINI_AGENT_RELEASE then
    if status == false then
        loader.unload("persist")
    end
    migrationres, migrationerr = copcall(loadmigration, currentversion)
    local persist = require "persist"
    persist.save("AgentVersion", _MIHINI_AGENT_RELEASE)
end
----------------------------------------------------------

local config = require "agent.config"
-- Set the log configuration according to configuration
if config.log then
    log.setlevel(config.log.defaultlevel) -- default log level
    for m, l in pairs(config.log.moduleslevel or {}) do
        log.setlevel(l, m)
    end
    log.timestampformat = config.log.timestampformat
    log.format = config.log.format
    local colors = config.log.enablecolors
    if colors then
        local f = type(colors)=='table' and colors or
            {ERROR="\027[31;1m", WARNING="\027[33;1m", DEBUG="\027[37;1m", INFO="\027[32;2m", DETAIL="\027[36;2m"}
        function log.displaylogger(module, severity, log)
            local f = f[severity] or "\027[0m"
            if f then print(f..log.."\027[0m") else print(log) end
        end
    end
    if config.log.policy then
        local logtools = require "log.tools"
        local res, err = copcall(logtools.init, config.log.policy)
        if not res then log("GENERAL", "ERROR", "Failed to init log.tools, err = %s", err or "unknown") end
        --remove reference to unused logpolicies
        loader.unload("log.tools")
    end
end

-- Necessary to receive signal from outside the Agent Process (on non OAT OS)
if config.agent.signalport then
    sched.listen(config.agent.signalport)
end

local initializedmodules = {}   -- holds the list of already initialized modules,
                                -- in the form [name] = true when initialization succeeded
local scheduledmodules = {}     -- holds the list of all scheduled modules, in the form [name] = true,
                                -- the entry is cleared when init is done (successfully or not)
local moduledep = {}            -- holds all module dependencies moduledep[name] = {list of dependencies}. This is used for cycle detection
local failedmodules = {}        -- holds modules that initialization fails (for logs only)
local simplemodules = {}        -- holds simple modules defined as a single init function. Usefull when adding a
                                -- couple Lua Module file is too much for a couple of lines... simplemodules[name] = modinitfunction


local function protectedinit(mod, params)
    return (simplemodules[mod] or require('agent.'..mod).init)(params)
end
protectedinit = protect(protectedinit)


local function initmodule(name, mod, deperr, params)

    local s, e
    if not deperr then
        log("GENERAL", "DETAIL", "Starting [%s] module...", name)
        s, e = protectedinit(mod, params)
    else e = deperr end
    e = e or "unknown" -- make sure some error is defined...
    if not s then
        log("GENERAL", "ERROR", "Failed to initialize module [%s], err=%s", name, e)
        failedmodules[name] = e
    else log("GENERAL", "INFO", "Module [%s] initialized", name) end

    initializedmodules[name] = s and true
    scheduledmodules[name] = nil
    sched.signal(name, "InitDone")

    if not next(scheduledmodules) then -- detect end of initialization !
        if not next(failedmodules) then
            log("GENERAL", "INFO", "Agent successfully initialized")
        else
            log("GENERAL", "ERROR", "Agent initialization finished with some errors:")
            for n, e in pairs(failedmodules) do log("GENERAL", "ERROR", "\t [%s] failed with %s", n, e) end
        end
        -- Notify the rest of the world that the agent's initialization process is completed.
        sched.signal("Agent", "InitDone")
    end
end

-- return a module that it depends on (if not initialized)
--  or nil followed by a module name that it depends on and will not be initialized
--  or false if all dependencies are succesfully resolved
local function checkdep(dep)

    for name, req in pairs(dep) do

        if req and not initializedmodules[name] then -- unsatisfied required dep
            if not scheduledmodules[name] then
                if failedmodules[name] then return nil, string.format("depends on [%s] that did not initialize successfully", name)
                else return nil, string.format("depends on [%s] that was not scheduled to be initialized", name) end
            end
            return name

        elseif scheduledmodules[name] then -- unsatisfied optional dep
            return name
        end

        dep[name] = nil -- prevent from re-testing all passed dependency at each checkdep
    end

    return false
end

-- Schedule a module to be initialized
--      name is the name of the module. Used to identify modules for the dependency checking.
--      mod is the name of the agent Lua module (or simplemodule) that will be loaded
--          first simplemodules table is checked for a function that has this name
--          it not found there, a require 'agent.'..mod is attempted.
--      initflag is a flag stating if this module is to be initialized
--      reqdep is a list of module it depends on (required dependency)
--      optdep is a list of modules it depends on if they are enabled (optionaldependency)
--      params is an optional parameter that will be provided to the init function
-- Dependency is used to order the loading sequence. This means that:
--      - a module will not be initialized unless all its required dependencies have been initialized successfully
--      - a module will wait for an optional dependency to be initialized (if scheduled to!), and continue even if the
--          referenced module initialization fails. An opt dependency on a non scheduled module will be ignored.
--local function scheduleinit(name, mod, initflag, reqdep, optdep, params)
local function scheduleinit(p)

    if not p.initflag then return end -- the module is not configured to be initialized
    local name, mod, reqdep, optdep, params = p.name, p.mod, p.reqdep, p.optdep, p.params
    mod = mod or name -- default module to load set to the given name of the module

    -- Build dependency table
    -- dep is a key-value table, where: the key is the module name it depends on,
    --   and the value is true: required dep, false: opt dep
    reqdep = type(reqdep) == 'table' and reqdep or {reqdep}
    optdep = type(optdep) == 'table' and optdep or {optdep}
    local dep = {}
    for i, k in ipairs(optdep) do dep[k] = false end
    for i, k in ipairs(reqdep) do dep[k] = true end

    moduledep[name] =  dep -- store the dependency tree

    scheduledmodules[name] = true

    local function waitinit()
        local d, err = checkdep(dep)
        if not d then sched.run(initmodule, name, mod, err, params)
        else sched.sigonce(d, "InitDone", waitinit) end
    end

    sched.sigonce("Agent", "InitStart", waitinit) -- init of this module will start only when the actual init process is started !
end

-- function that check if there is no cycle in the init sequence
local function checkcycle()
    local t = {}
    local function visit(n)
        t[n] = "grey"
        for name in pairs(moduledep[n]) do
            if t[name] == "grey" then return nil, {name}
            elseif t[name] == "white" then
                local s, mods = visit(name)
                if not s then table.insert(mods, name) return nil, mods end
            end
        end
        t[n] = "black"
        return true
    end

    for name in pairs(scheduledmodules) do t[name] = "white" end
    for name, color in pairs(t) do
        if color == "white" then
            local s, mods = visit(name)
            if not s then table.insert(mods, name) return nil, mods end
        end
    end

    return "ok"
end

local function startInitProcess()
    -- Check that there are no cycle in the dependency graph
    local s, c = checkcycle()
    if not s then
        repeat
            log("GENERAL", "ERROR", "Detected a cycle in the module dependency graph: [%s]", table.concat(c, "<-"))
            local c1, c2 = c[1], c[2]
            log("GENERAL", "ERROR", "Will break last dependency of [%s] on [%s] in order to attempt to unlock init process (no guaranty it'll work...)", c2, c1)
            moduledep[c2][c1] = nil
            s, c = checkcycle()
        until s
    end

    -- Load PlatformSpecifics before anything
    local s, e = protectedinit("platform", config.platformsettings)
    if not s then log("GENERAL", "ERROR", "Error while loading PlatformSpecifics, error=%s",e or "unknown") end
    -- Actually start the init process
    sched.signal("Agent", "InitStart")
end

log("GENERAL", "INFO", string.rep("*", 60))
log("GENERAL", "INFO", "Starting Agent ...")
log("GENERAL", "INFO", "     Agent: %s", _MIHINI_AGENT_RELEASE)
log("GENERAL", "INFO", "     Lua VM: %s", _LUARELEASE)
log("GENERAL", "INFO", "     System: %s", _OSVERSION)
log("GENERAL", "INFO", string.rep("*", 60))
if migrationres == true then
    log("GENERAL", "INFO", "Migration executed successfully")
elseif migrationres == false then
    log("GENERAL", "ERROR", "Migration error: %s", migrationerr or "unknown error")
end --migrationres = nil means no migration done at boot


local function initialize()
    local c = config.get

    if (not c'agent.deviceId' or c'agent.deviceId' == "") then
        if type(platform.getdeviceid) == "function" then
            config.set('agent.deviceId', platform.getdeviceid())
        else
            log("GENERAL", "ERROR", "No deviceId defined in config and no function agent.platform.getdeviceid defined")
        end
    end
    log("GENERAL", "INFO", "Device ID = %q", c'agent.deviceId')

    -- Create the socket that is used for communication with all local clients
    scheduleinit{name="AssetConnector", mod="asscon", initflag=true, params=c"agent"}
    scheduleinit{name="ServerConnector", mod="srvcon", initflag=true, reqdep="AssetConnector", optdep="NetworkManager" }
    scheduleinit{name="Modem", mod="modem", initflag=c"modem.activate"}

    -- Start the shell if activated in the configuration
    scheduleinit{name="Lua Shell", mod="shell", initflag=c"shell.activate", params=c"shell"}

    -- Start the network manager
    scheduleinit{name="NetworkManager", mod="netman", initflag=c"network.activate"}
    scheduleinit{name="DummyNetman", mod="dummynetman", initflag=not c"network.activate"}

    -- Start the SMS Bearer
    scheduleinit{name="SMS", mod= "asscon.sms", initflag=c"modem.sms", optdep="Modem"}

    -- Start the Mediation client module
    scheduleinit{name="Mediation", mod="mediation", initflag=c"mediation.activate", reqdep="ServerConnector", optdep={"NetworkManager", "DummyNetman"}}

    -- Start the Software Update module
    scheduleinit{name="Update", mod="update", initflag=c"update.activate",
                 reqdep= {"DeviceManagement", "AssetConnector"}, optdep = {"ApplicationContainer", "SMS", "NetworkManager" }}

    -- Start the Application Container
    scheduleinit{name="ApplicationContainer", mod="appcon", initflag=c"appcon.activate", optdep= "DeviceManagement"}

    -- Start the Device Management module
    scheduleinit{name="DeviceManagement", mod="devman", initflag=c"device.activate", reqdep="AssetConnector", params=c"device"}

    -- Start the Monitoring Engine, currently disabled
    scheduleinit{name="Monitoring", mod="monitoring", initflag=c"monitoring.activate", reqdep="DeviceManagement"}

    -- Start the Time / NTP services
    scheduleinit{name="Time", mod="time", initflag=c"time.activate"}

    -- Add Lua RPC mechanism
    scheduleinit{name="Lua RPC", mod="rpc", initflag=c"rpc.activate", params=c"rpc"}

    -- Data manager: SendData requests between asset and server, both directions
    scheduleinit{name="DataManagement", mod="asscon.datamanager", initflag=c"data.activate",params=c"data", reqdep="AssetConnector"}

    scheduleinit{name="AutoExec", mod="autoexec", initflag=c"autoexec.activate",
                 reqdep= {"DeviceManagement"}, optdep = {"ApplicationContainer", "SMS", "NetworkManager" }}

    -- Start the rest module
    scheduleinit{name="Rest", mod="rest", initflag=c"rest.activate"}

    -- Launch the Init process
    startInitProcess()
end



-- Additional module definition.
-- Modules that does not deserve a independent Lua Module file may lay here. It should be no more that simple function
-- with a few nb of line of code !

function simplemodules.rpc(config)
    local rpc = require "rpc"
    return rpc.newmultiserver(config.address, config.port)
end

-- Placeholder for the network manager, when it isn't activated because network
-- connection is handled out of the agent.
function simplemodules.dummynetman()
    local sched = require 'sched'
    module 'agent.netman'
    function withnetwork(action, ...) return action(...) end
    sched.sigonce("Agent", "InitDone", function() sched.signal("NETMAN", "CONNECTED", "Default") end)
    return "ok"
end

function simplemodules.shell(config)
    local s = require "shell.telnet"
    return s.init(config)
end


-- Keep it last line: schedule all modules to be initialized
sched.run(initialize)

