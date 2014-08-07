local u             = require 'unittest'
local math          = require 'math'
local platformAPI   = require 'tester.platformAPI'
local yajl          = require 'yajl'
local pltconf       = {}
local platformTable = nil
local appprefix     = "MihiniModel_"
local p             = p
local sprint        = sprint
local table         = table
local loader        = require 'utils.loader' 


local t = u.newtestsuite 'RaconWrapper'
local targetManager = nil


function init(tm)
  targetManager = tm
end


-- Compile third party modules and configure the agent
function t:setup()
  u.assert_not_nil(targetManager)
  u.assert_not_nil(targetManager.config)
  
  local makelist = {
      "start_dev_test",
      "unittest_sys_test",
      "unittest_sms_test",
      "unittest_emp_test",
      "unittest_dt_test",
      "unittest_updatetest",
      "unittest_dset_test",
      "unittest_asset_tree",
      "unittest_racon",
      "unittest_emp"
  }
  
  local makestring = ""
  for k,v in pairs(makelist) do
      makestring = makestring .. " " .. v
  end
  
  -- backup defaultconfig file
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  os.execute("cd "..targetManager.targetdir.." && make" .. makestring)
  os.execute("cp "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup")
  
end

function t:teardown()
  os.execute("mv "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  
  loader.unload("agent.defaultconfig")
end


function t:test_asset_tree()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 10 -R asset_tree.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_racon()
    targetManager:start()
    sched.wait(2)
    -- 20 seconds timeout because sometimes it lasts more than 10 seconds
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 30 -R racon.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_devicetree()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R devicetree.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_sms()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R sms.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_system()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R system.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_emp()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 10 -R emp.lua")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_dset_test()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R dset_test")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_emp_test()
    targetManager:start()
    sched.wait(2)
    -- 40 seconds timeout because sometimes it lasts more than 10 seconds
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 40 -R emp_test")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_av_test()
    targetManager:start()
    sched.wait(2)
    -- 40 seconds timeout because sometimes it lasts more than 10 seconds
    local val = os.execute("cd "..targetManager.targetdir.." && make unittest_av_test && ctest --output-on-failure -j2 --timeout 40 -R av_test")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_sms_test()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.."  && ctest --output-on-failure -j2 --timeout 5 -R sms_test")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_sys_test()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R sys_test")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_updatetest()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R updatetest")
    targetManager:stop()
    u.assert_equal(0, val)
end

function t:test_dt_test()
    targetManager:start()
    sched.wait(2)
    local val = os.execute("cd "..targetManager.targetdir.." && ctest --output-on-failure -j2 --timeout 5 -R dt_test")
    targetManager:stop()
    u.assert_equal(0, val)
end


return {init = init}