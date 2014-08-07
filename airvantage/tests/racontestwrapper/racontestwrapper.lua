#!bin/lua

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

local AGENT_DIR = nil
local env = nil
local depmodule = {}
local depopt = nil

local function loadtestwrappermodule()
    for _, dep in pairs(depmodule) do
        local mod = require ("testwrapperfwk." .. dep)
        if mod and mod.setup then
            mod.setup(AGENT_DIR, depopt)
        end
    end
end

local function unloadtestwrappermodule()
    for _, dep in pairs(depmodule) do
        local mod = require ("testwrapperfwk." .. dep)
	if mod and mod.teardown then
            mod.teardown()
	end
    end
end

local function run_lua_unittest(testname)
    testname = testname:gsub(".lua", "")
    require ("tests." .. testname)
    unittest.run()

    local status = (#unittest.getStats().failedtests ~= 0) and 1 or 0
    return status
end

local function run_non_standalone_lua_unittest(testname)
    testname = testname:gsub(".lua", "")

    local client = require 'rpc'.newclient("localhost", 2012)
    if not client then
        print("Ready agent not started")
        return
    end

    local run_unit_test = client:newexec(function(testname)
        require ('tests.'..testname)
        unittest.run()
        local status = (#unittest.getStats().failedtests ~= 0) and 1 or 0
        return status
    end)
    return run_unit_test(testname)
end

local function run_native_unittest(testname)
    local status = os.execute("env - " .. env .. " " .. AGENT_DIR .. "/bin/" .. testname)
    if status >= 128 then
       require 'print'
       p("received signal " .. tostring(status-128))
    end
    return status
end

local function run_wrapper(testname, testtype)
    local func = (testtype == "non-standalone") and run_non_standalone_lua_unittest or ( (testtype == "native") and run_native_unittest or run_lua_unittest)

    if #depmodule then
       loadtestwrappermodule()
    end

    local status = func(testname)

    if #depmodule then
        unloadtestwrappermodule()
    end
    os.exit(status)
end

local function envsetup(progname)
    local fd = io.popen("cd $(dirname ".. progname .. ") && pwd")
    AGENT_DIR=fd:read('*l')
    fd:close()

    -- Unfortunately os.setenv() does not exist in standard Lua
    local LUA_PATH = AGENT_DIR .. "/?.lua;" .. AGENT_DIR .. "/lua/?.lua;" .. AGENT_DIR .. "/lua/?/init.lua"
    local LUA_CPATH = AGENT_DIR .. "/lua/?.so"
    env = "LD_LIBRARY_PATH=" .. AGENT_DIR .. "/lib"
    env = env .. " LUA_PATH=\"" .. LUA_PATH .. "\""
    env = env .. " LUA_CPATH=\"" .. LUA_CPATH .. "\""

    if os.getenv("SWI_LOG_VERBOSITY") then
        env = env .. " SWI_LOG_VERBOSITY=" .. os.getenv("SWI_LOG_VERBOSITY")
    end

    package.path = LUA_PATH
    package.cpath = LUA_CPATH

    _G.wrapper_env = env
end

local function usage()
    print("Usage: " .. arg[0] .. " [ -t type ] [ -l dependency ] [ -o dependency_options ] unittest")
    print("Available types:\n standalone\n non-standalone")
    print("Available dependencies:")
    require 'lfs'
    for d in lfs.dir(AGENT_DIR .. "/testwrapperfwk") do
        if d ~= '.' and d ~= '..' then
            local modname, _ =  string.gsub(d, ".lua", "")
            print(" " .. modname)
        end
    end
    os.exit(1)
end

local function main(argv)
    envsetup(argv[0])
    if #argv < 1 then
        usage()
    end

    local type, testname = "standalone", nil

    for i=1,#argv do

        if argv[i] == "-t" then
            type = (argv[i+1] == "non-standalone") and argv[i+1] or (argv[i+1] == "standalone") and argv[i+1] or "unknown"
            if type == "unknown" then
                print("Unknown type", type)
                usage(argv[0])
            end
            i = i + 2
        elseif argv[i] == "-l" then
	    table.insert(depmodule, argv[i+1])
            i = i + 2
        elseif argv[i] == "-o" then
            depopt = argv[i+1]
            i = i + 2
	else
	    testname = argv[i]
        end
    end

    local from, to = string.find(testname, ".lua")
    local testtype = (type == "non-standalone") and "non-standalone" or ((not from and not to) and "native" or "lua")
    local sched = require 'sched'

    local iterator = (#depmodule ~= 0) and string.gmatch(depmodule[1], "[a-z_0-9\\.]+") or nil
    if iterator then
       depmodule = {}
       for dep in iterator do
	  table.insert(depmodule, dep)
       end
    end
    sched.run(run_wrapper, testname, testtype)
    sched.loop()
end

main(arg)
