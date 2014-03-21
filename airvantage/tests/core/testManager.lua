-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local targetmanager = require 'tester.targetManager'
local print = print
local p = p
local string = string
local copcall = copcall
local table = table
local pairs = pairs
local ipairs = ipairs
local require = require
local setmetatable = setmetatable
local rpc = require 'rpc'
local sched = require 'sched'
local assert = assert
local tools = require 'tester.testerTools'
local os = require 'os'
local fmt = string.format
local type = type
--local print = tools.testerprint

module(...)
local managerapi = {}

--------------------------------------------------------------------------------
-- Utilities
--------------------------------------------------------------------------------

-- Function: tablesearch
-- Description: search for the provided value in the elements of the table.
-- Return: the index of the value in the table if found, nil otherwise
local function tablesearch(t, value)
  for i,v in ipairs(t) do
    if v == value then return i end
  end

  -- not found, return nil
  return nil
end

-- Function: tableconcat
-- Description: add all elements from source table into destination table
-- Return: nothing. Destiation table has been modified
local function tableconcat(source, destination)
  for i, val in ipairs(source) do
    table.insert(destination, val)
  end

  return destination
end


--------------------------------------------------------------------------------
-- Config Management
--------------------------------------------------------------------------------
-- Function: filterpolicy
-- Description: filter the input table according to the provided policy
-- Return: the new table containing only tests that respect the policy
--         if policy is empty, then return the exact table
local function filterpolicy(t, policy)
  if not policy then return t end

  local l_t = {}

  for k,v in pairs(t) do
    if string.lower(v.TestPolicy) == policy then l_t[k] = v end
  end

  return l_t
end

-- Function: parseFilter
-- Description: load the config file and compute a table containing all tests to run according to the filter
-- Return: A table containing only tests that respect the filter and organized by targets
local function parseFilter(filter)
  local l_filters = {}

  filter = string.lower(filter)

  -- extract target names
  local l_targets = filter:match("target=([^;]+);?$?")
  l_filters.targets = {}
  if l_targets then
    for target in l_targets:gmatch("([^,]+),?$?") do
      table.insert(l_filters.targets, target:match("%S+"))
    end
  end

  -- extract policy name
  local l_policy = filter:match("policy=([^;]+);?$?")
  if l_policy then l_policy = l_policy:match("%S+") end
  l_filters.policy = l_policy

  return l_filters
end

-- Function: loadconfig
-- Description: load the config file and compute a table containing all tests to run according to the filter
-- Return: A table containing only tests that respect the filter and organized by targets
local function loadconfig(testconfigfile, targetconfigfile, filter)
  print('loading config files:')
  print('--tests list definition: '..testconfigfile)
  print('--targets list definition: '..targetconfigfile)
  local l_testconfig = require('tester.'..testconfigfile)
  local l_targetconfig = require('tester.'..targetconfigfile)
  local l_filter = {}


  l_filter = parseFilter(filter)
  l_testconfig = filterpolicy(l_testconfig, l_filter.policy)

  local l_finalconfig = {}

  -- if no targets have been specified, then list all available targets in the table
  if (not l_filter.targets) or (#l_filter.targets == 0) then
    l_filter.targets = {}
    for k,v in pairs(l_testconfig) do
      for i, targetname in ipairs(v.target) do
        if not tablesearch(l_filter.targets, targetname) then table.insert(l_filter.targets, targetname) end
      end
    end
  end

  -- loop on all values of the target filter to fill the result table
  for tarkey,target in ipairs(l_filter.targets) do
    if not l_finalconfig[target] then l_finalconfig[target] = {} end

    -- insert the config table in the target configuration
    l_finalconfig[target].config = {}
    l_finalconfig[target].config = l_targetconfig[target]
    l_finalconfig[target].tests = {}
    p(l_testconfig)
    for k,v in pairs(l_testconfig) do
      local l_index = tablesearch(v.target, target)
      if l_index then
	if type(v.environment) == "table" then
	  for _,env in pairs(v.environment) do
	    print("Adding ".. k .. " to target "..target.. "/"..env)
	
	    -- Create new environment if not yet defined
	    -- Then insert the test in this environment
	    if not l_finalconfig[target].tests[env] then l_finalconfig[target].tests[env] = {} end
	    table.insert(l_finalconfig[target].tests[env], k)
	  end
	else
	  local env = v.environment
	  print("Adding ".. k .. " to target "..target.. "/"..env)
	  if not l_finalconfig[target].tests[env] then l_finalconfig[target].tests[env] = {} end
	  table.insert(l_finalconfig[target].tests[env], k)
	end
      end
    end
  end

  return l_finalconfig, l_testconfig
end

--------------------------------------------------------------------------------
-- Display Management
--------------------------------------------------------------------------------
-- Function: displayresults
-- Description: Display the result on the standard output. Number of tests passed, failed and aborted.
-- Return: success if no error occured, raise an error otherwise
function managerapi:displayresults()
  assert(self.results)

  local stats = self.results

  print(" ")
  print(string.rep("=", 41).." Automated Unit Test Report "..string.rep("=", 41))

  local globalresults = stats.all

  print(fmt("Run xUnit tests of %d testsuites and %d testcases (%d assert conditions) in %d seconds", globalresults.nbtestsuites, globalresults.totalnbtests, globalresults.nbofassert, stats.endtime-stats.starttime))
  print(fmt("\t%d passed, %d failed, %d errors", globalresults.nbpassedtests, globalresults.nbfailedtests, globalresults.nberrortests))

  -- Display failed and aborted testcases/testsuites
  if globalresults.nbfailedtests + globalresults.nberrortests > 0 then
      print(string.rep("-", 100))
      print("Detailed error log:")
      for i, err in ipairs(globalresults.failedtests) do
        print(fmt("%d) %s - %s.%s", i, err.type, err.testsuite, err.test))
        print(err.msg)
      end
  end

  if globalresults.nbabortedtestsuites > 0 then
      print(string.rep("-", 100))
      print(fmt("This test sequences contains %d testsuites that were aborted:", globalresults.nbabortedtestsuites))
      for _, name in ipairs(globalresults.abortedtestsuites) do print("\t"..name) end
  end

  print(string.rep("=", 100))
  print(" ")
end

--------------------------------------------------------------------------------
-- Tests Management
--------------------------------------------------------------------------------

-- -- Function: loadtestsontarget
-- -- Description: Load all tests corresponding to "environment" on the target. rpcclient must not be a valid object.
-- -- Return: success if no error occured, raise an error otherwise
-- local function loadtestsontarget(tests, environment, rpcclient)
--   assert(tests, "Tests list has to be a valid table")
--   assert(environment, "environment has to be specified")
--   assert(rpcclient, "RPC client has to be valid")
-- 
--   if not tests[environment] then
--     print('No tests defined for environment '..environment)
--   else
--     for key, value in pairs(tests[environment]) do
--       rpcclient:call('require', 'tests.'..value)
--       print(value .." loaded")
--     end
--   end
-- 
--   return "success"
-- end
-- 
-- -- Function: runluafwktests
-- -- Description: Run the luafwk specific tests for the current target. Store the results in the target table
-- -- Return: Nothing
-- local function runluafwktests(testmgt, target)
--   print("	Running luafwk tests")
--   target:install()
-- 
--   target:startlua()
-- 
--   sched.wait(3)
--   local rpcclient = target:getrpc(true)
--   loadtestsontarget(target.testslist, "luafwk", rpcclient)
-- 
--   rpcclient:call('startLuaFwkTests')
--   target.luafwkresults = rpcclient:call('LuaFWKgettestsResults')
--   sched.wait(1)
-- 
--   target:stop()
-- end
-- 
-- -- Function: runagenttests
-- -- Description: Run the Reagy Agent local tests for the current target. Store the results in the target table
-- -- Return: Nothing
-- local function runagenttests(testmgt, target)
--   print("	Running agent tests")
--   target:install()
-- 
--   -- start the Agent on the target
--   target:start()
-- 
--   sched.wait(10)
-- 
--   ---------------------------------------
--   -- Initialize the environment on the target. Load a test framework and all defined tests to run
--   local rpcclient = target:getrpc(false)
--   local res,err = rpcclient:call('require', 'tests.embtests')
--   if err then print("ERROR - Loading embedded tests fwk error: "..err) end
-- 
--   -- Load all tests to run
--   loadtestsontarget(target.testslist, "agent", rpcclient)
-- 
--   -- Execute tests and retrieve results from remote client
--   --assert(rpcclient:call('runTests'), "ERROR - starting tests on DuT error: ")
--   res, err = rpcclient:call('runTests')
-- 
--   res,err = rpcclient:call('getResults')
--   assert(res,err, "ERROR - getting tests results error: ")
-- 
--   --store results in the table
--   target.agentresults = res
--   target:stop()
-- end
-- 
-- -- Function: runhosttests
-- -- Description: Run tests of the Ready Agent with communications between the host and the server. Store the results in the target table
-- -- Return: Nothing
-- local function runhosttests(testmgt, target)
--   print("	Running host tests")
--   target:install()
-- 
--   target:start()
-- 
--   --target.hostresults
--   target:stop()
-- end

-- Function: processresults
-- Description: Aggregate the results to the entire result table. Store the target's results in the table and also sum them with the global other ones
-- Return: nothing
local function processresults(testmgt, target)
  print("Processing results for "..target.name)

  if (target) then
    testmgt.results.targets[target.name] =  { }

    -- Sum each field
    for k,v in pairs(target.results) do
      testmgt.results.all.nbpassedtests = testmgt.results.all.nbpassedtests + v.nbpassedtests
      testmgt.results.all.nbfailedtests = testmgt.results.all.nbfailedtests + v.nbfailedtests
      testmgt.results.all.nberrortests = testmgt.results.all.nberrortests + v.nberrortests
      testmgt.results.all.nbtestsuites = testmgt.results.all.nbtestsuites + v.nbtestsuites
      testmgt.results.all.nbabortedtestsuites = testmgt.results.all.nbabortedtestsuites + v.nbabortedtestsuites
      testmgt.results.all.nbofassert = testmgt.results.all.nbofassert + v.nbofassert

      testmgt.results.all.passedtests = tableconcat(v.passedtests, testmgt.results.all.passedtests)
      testmgt.results.all.failedtests = tableconcat(v.failedtests, testmgt.results.all.failedtests)
      testmgt.results.all.abortedtestsuites = tableconcat(v.abortedtestsuites, testmgt.results.all.abortedtestsuites)

      testmgt.results.all.totalnbtests = testmgt.results.all.nbpassedtests + testmgt.results.all.nbfailedtests + testmgt.results.all.nberrortests
    end
  end


end


-- Function: new
-- Description: Create a new test manager, according to the specified filter and config files
-- Return: the instance of the new test manager
function new(svndir, filter, testconfigfile, targetconfigfile, testdir)

  local stats = {nbpassedtests = 0,
              passedtests = {},
              nbfailedtests = 0,
              nberrortests  = 0,
              failedtests   = {},
              nbtestsuites  = 0,
	      nbofassert = 0,
              nbabortedtestsuites = 0,
	      totalnbtests = 0,
              abortedtestsuites = {}}
  local lresults = {targets = {},
      all = stats}

  local instance = setmetatable({ svndir = svndir,
      filter = filter,
      testconfigfile = testconfigfile,
      targetconfigfile = targetconfigfile,
      testdir = testdir,
      results = lresults}, {__index=managerapi})

  instance.filteredtests, instance.alltests = loadconfig(testconfigfile, targetconfigfile, filter)

  return instance
end


-- Function: runtarget
-- Description: Run the test suites on the specified target.
--              The testsuites shall implement 'run' and 'getResults' functions
--              'Run' shall accept the following parameters: the testmanager instance, the target object, and the list of tests to run on this target
-- Return: none
function managerapi:runtarget(target)
  target:compile()

  -- parse all test type
  for k,v in pairs(target.testslist) do
    local f = require("tests.ts"..k)
    f.run(self, target, v)

    target.results[k] = f.getResults()
  end

  processresults(self, target)

  return "success"
end

-- Function: run
-- Description: Run the test suites using the configured ones
-- Return: none
function managerapi:run()

  self.results.starttime = os.time()
  -- loop on all targets configured for the tests to play
  for key, value in pairs(self.filteredtests) do
    print("Running tests on target: " .. key)

      -- create a new target manager for the current target
      local l_target = targetmanager.new(key, 'tester.config'..key, value, self.svndir, self.testdir.."/../../targets/"..key)

      --protect the test run for this target
      local res, error = copcall(function() self:runtarget(l_target) end )
      
      -- log the message if an error occured during compilation process or something else.
      if not res then
        print(error)
        self.results.all.failedtests = tableconcat(self.results.all.failedtests, {{type = "tester", testsuite = "run", test = "run target for "..key, msg = error} })
        self.results.all.nbfailedtests = self.results.all.nbfailedtests + 1
      end
      
      print("end of target "..key)
  end

  self.results.endtime = os.time()

  self:displayresults()

  print("end of run.")

  if (self.results) then
    return self.results.all.nbfailedtests + self.results.all.nberrortests
  else
    return -1
  end
end