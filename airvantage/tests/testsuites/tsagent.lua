local print = print
local sched = require "sched"
local p = p
local rpc = require "rpc"
local pairs = pairs
local assert = assert

module(...)
local results = nil

-- Load tests on the target system
function loadtestsontarget(testslist, env, rpcclient)
  local tests = testslist[env]

  assert(tests, "Test list empty for "..env.." environment")
  assert(rpcclient)

  for k,v in pairs(tests) do
    rpcclient:call("loadinfwk", "tests."..v)
    print("\t"..v.." loaded")
  end
end

-- setup test environment and run tests before killing process properly
function run(tm, target, testlist)
  results = {}

  print("\tRunning agent tests")
  target:install()

  target:start()

  sched.wait(3)
  local rpcclient = target:getrpc(false)

  rpcclient:call('require', 'tests.agenttests')
  loadtestsontarget(target.testslist, "agent", rpcclient)
  rpcclient:call('startAgentTests')
  results = rpcclient:call('AgentgettestsResults')
  sched.wait(1)

  target:stop()
  print("\tagent process stopped")
end


-- Return the results of tests run
function getResults()
  return results
end