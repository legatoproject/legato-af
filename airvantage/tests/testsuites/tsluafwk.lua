local print = print
local sched = require "sched"
local p = p
local rpc = require "rpc"
local pairs = pairs
local assert = assert

module(...)
local results = nil

function loadtestsontarget(testslist, env, rpcclient)
  local tests = testslist[env]

  assert(tests, "Test list empty for "..env.." environment")
  assert(rpcclient)
  

  for k,v in pairs(tests) do
    rpcclient:call("loadinfwk", "tests."..v)
    print(v.." loaded")
  end
end

function run(tm, target, testlist)
  results = {}

  print("\tRunning luafwk tests")
  target:install()

  target:startlua()

  sched.wait(3)
  local rpcclient = target:getrpc(true)
  loadtestsontarget(target.testslist, "luafwk", rpcclient)

  rpcclient:call('startLuaFwkTests')
  results = rpcclient:call('LuaFWKgettestsResults')
  sched.wait(1)

  target:stop()
end


-- Return the results of tests run
function getResults()
  return results
end