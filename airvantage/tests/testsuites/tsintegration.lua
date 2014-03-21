local print = print
local sched = require "sched"
local p = p
local rpc = require "rpc"
local pairs = pairs
local assert = assert
local unittest = require 'unittest'
local require = require
local os = require "os"

module(...)
local results = nil
local unittestslist = {}

function loadtests(testslist, env, target)
  local tests = testslist[env]
 
  assert(tests, "Test list empty for "..env.." environment")
 
  for k,v in pairs(tests) do

    unittestslist[v] = require("tests."..v)
    p(unittestslist[v])
    unittestslist[v].init(target)
    print(v.." loaded")
  end
end
 
function run(tm, target, testlist)
  results = {}

  print("\tRunning integration tests")
  loadtests(target.testslist, "integration", target)

  -- run integration tests
  unittest.run()

  --get results
  results = unittest.getStats()

  -- clear all loaded tests
  unittest.resettestsuite()
  sched.wait(1)
end


-- Return the results of tests run
function getResults()
  return results
end