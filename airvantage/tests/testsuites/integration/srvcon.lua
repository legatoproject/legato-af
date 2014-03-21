local u             = require 'unittest'
local math          = require 'math'
local platformAPI   = require 'tester.platformAPI'
local yajl          = require 'yajl'
local pltconf       = {}
local platformTable = nil
--local appprefix     = "MihiniModel_"
local appprefix     = "MihiniModel_Tests"
local p             = p
local sprint        = sprint
local table         = table
local http          = require 'socket.http'
local loader        = require 'utils.loader' 


local t,errts = u.newtestsuite('ServerConnector')
local targetManager = nil
local defaultport = 8097
local baseurl = "http://localhost:"..defaultport.."/"
local deviceUID = nil

local function modifyDefaultConfigFile(url, deviceId, auth, encrypt, path)
  -- load the file
  local defconf = require "agent.defaultconfig"
  
  defconf.server.url = url
  
  if (auth == "None") then auth = nil end
  defconf.server.authentication = auth
  
  if (encrypt == "None") then encrypt = nil end
  defconf.server.encryption = encrypt
  
  defconf.agent.deviceId = deviceId
  
  local s = "local config = " .. sprint(defconf) .. "\nreturn config"
  local file,err = io.open(path .. "/runtime/lua/agent/defaultconfig.lua", "w")
  u.assert_not_nil(file, err)
  file:write(s)
  file:flush()
  file:close()
end


local function getDT(key)
    local dt = require 'devicetree'
    dt.init()
    return dt.get(key)
end


function init(tm)
  targetManager = tm
end

local function registerDevice(devicename, crypto, registrationPassword)

  if not registrationPassword then registrationPassword = "" end

  -- Fill the structure to create the device:
  -- - Device Name
  -- - Serial Number
  -- - Application to use
  -- - Registration Password to use
  local config = {
   name = devicename,
  
   gateway = {
      serialNumber = devicename
   },
   
   applications = {{
      name =  appprefix .. crypto, -- "hmac-sha1_aes-cbc-128" for instance
      type = "Mihini Software",
      revision = "Tests"
   }},
   
   communication =   {
      m3da =    {
          password = registrationPassword
      }
    }
  }
  
  -- Create the device with the associated model
  print("Registering device " .. devicename .. " using " .. crypto .. " and its password is " .. registrationPassword)
  local operationTable,errmess =  platformAPI.registerSystem(platformTable, config)
  
  -- Check device has been successfuly created
  u.assert_equal("200HTTP/1.1 200 OK", errmess)
  u.assert_not_nil(operationTable)
  u.assert_not_nil(operationTable.uid)
  print("System ".. operationTable.uid .." registered")
  
  -- Activate Device
  local systemID = { uids = {operationTable.uid} }
  local operationActTable,errmess =  platformAPI.activateSystems(platformTable, systemID)
  u.assert_equal(errmess, "200HTTP/1.1 200 OK")
  
  return operationTable.uid
end

local function createSyncJob(uid)
  u.assert_not_nil(uid)
  
  local syncJob = {
    action = "MIHINI_DM_SYNCHRONIZE",
    systems = {
       uids = { uid }
    }
  }
  
  local operationTable,errmess =  platformAPI.syncSystems(platformTable, syncJob)
  u.assert_equal(errmess, "200HTTP/1.1 200 OK")
  print("Synchronize operation created: " .. operationTable.operation)
  local syncJobID = operationTable.operation
  
  
  u.assert_not_nil(syncJobID)
  return syncJobID
end

local function createCommand(device_uid, app_name, commandId, parameters)
  u.assert_not_nil(device_uid)
  
  local AssetJob = {
    application = { name = app_name },
    systems = { uids = { device_uid } },
    commandId = commandId,
    parameters = parameters
  }
  
  local operationTable,errmess =  platformAPI.customCommand(platformTable, AssetJob)
  u.assert_equal(errmess, "200HTTP/1.1 200 OK")
  print("Asset job created: " .. operationTable.operation)
  local JobID = operationTable.operation
  
  u.assert_not_nil(JobID)
  return JobID
end

local function createApplySetting(device_uid, app_name, settings)
  u.assert_not_nil(device_uid)
  
  local AssetJob = {
    application = { name = app_name },
    systems = { uids = { device_uid } },
    settings = settings
  }
  
  local operationTable,errmess =  platformAPI.applySystemSettings(platformTable, AssetJob)
  u.assert_equal("200HTTP/1.1 200 OK", errmess)
  print("Asset job created: " .. operationTable.operation)
  local JobID = operationTable.operation
  
  u.assert_not_nil(JobID)
  return JobID
end

local function createRetrieveData(device_uid, data)
  u.assert_not_nil(device_uid)
  
  local AssetJob = {
    --application = { name = app_name },
    systems = { uids = { device_uid } },
    data = data
  }
    
  local operationTable,errmess =  platformAPI.retreiveSystemData(platformTable, AssetJob)
  u.assert_equal("200HTTP/1.1 200 OK", errmess)
  print("Asset job created: " .. operationTable.operation)
  local JobID = operationTable.operation
  
  u.assert_not_nil(JobID)
  return JobID
end


function t:setup()
  u.assert_not_nil(targetManager)
  u.assert_not_nil(targetManager.config)
  u.assert_not_nil(targetManager.config.integConfig)
  pltconf = targetManager.config.integConfig

  
  local platform,errmess = platformAPI.new(pltconf)
  u.assert_not_nil(platform,errmess)
  platformTable = platform
  print("Platform URL is: "..platformTable.serverURL)
  local accessGranted,errmess = platformAPI.getAccessToken(platformTable)
  u.assert_true(accessGranted,errmess)
  
  -- backup defaultconfig file
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  os.execute("cp "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup")
  
  deviceUID = registerDevice("testautoSrvCon", "")
  u.assert_not_nil(deviceUID)
end

function t:teardown()
  os.execute("mv "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  
  platformAPI.delDevice(platformTable, "testautoSrvCon")
  loader.unload("agent.defaultconfig")
end

-- Test: test_ReadDeviceTreeConfig
-- Description: Read a field in the config device tree 
-- Validation: 
--     - All setup steps are OK
--     - RetrieveData job status is success
--     - modified fields are synchronized
function t:test_ReadDeviceTreeConfig()
  local testValue = "ReadConfigSuccess"
  local testedKey = "@sys.config.time.ntpserver"
  
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset = racon.newAsset("TestAsset")
    
      myasset.tree.APN = "DefaultAPN"
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end
     
     local dt = require "devicetree"
     dt.init()
     assert(dt.set("config.time.ntpserver", value))

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- create job on server side
  --local app_name = appprefix
  local data = { testedKey }
  local jobID = createRetrieveData(deviceUID, data)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues[testedKey])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end

-- Test: test_synchronize
-- Description: check that synchronization works correctly
-- Validation: 
--     - All setup steps are OK
--     - Synchronize job status is success
--     - customized content fields are synchronized
function t:test_synchronize()
  local testValue = "SyncSuccess"
  local testedKey = "@sys.config.time.ntpserver"
   
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset, err = racon.newAsset("TestAsset")
    
      print(err)
      myasset.tree.APN = "DefaultAPN"
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end
     
     local dt = require "devicetree"
     dt.init()
     assert(dt.set("config.time.ntpserver", value))

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- Create a synchronize job on AVMS  
  local syncJobID = createSyncJob(deviceUID)
  sched.wait(5)
 
  -- force device to connect and get the job
  local rescon, err = rpcclient:call("agent.srvcon.connect")
  
  local status =  platformAPI.waitJobEnd(platformTable, syncJobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues[testedKey])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end


-- Test: test_readConfig
-- Description: try to read the whole config table
-- Validation: 
--     - All setup steps are OK
--     - RetrieveData job status is success
--     - customized content fields are synchronized
function t:test_readConfig()
  local testValue = "ReadWholeConfigSuccess"
  local testedKey = "@sys.config.time.ntpserver"
   
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset = racon.newAsset("TestAsset")
    
      myasset.tree.APN = "DefaultAPN"
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end
     
     local dt = require "devicetree"
     dt.init()
     assert(dt.set("config.time.ntpserver", value))

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- create job on server side
  --local app_name = appprefix
  local data = { "@sys.config" }
  local jobID = createRetrieveData(deviceUID, data)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues[testedKey])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end



-- Test: test_readConfigEmptyPath
-- Description: try to read an empty path
-- Validation: 
--     - All setup steps are OK
--     - Read job status is success
--     - full configuration has been synchronized (config, update, appcon ...)
function t:test_readConfigEmptyPath()
   local testValue = "ReadSuccess"
   local testedKey = "@sys"
   
   local name = "testautoSrvCon"
   local path = targetManager.targetdir
   modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset = racon.newAsset("TestAsset")
    
      myasset.tree.APN = value
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- create job on server side
   local data = { testedKey }
   local jobID = createRetrieveData(deviceUID, data)
   sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()
  

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end


-- Test: test_readConfigNonExistingPath
-- Description: Read a field in the system configuration tree that does not exist
-- Validation: 
--     - All setup steps are OK
--     - Read job status is success
--     - field is nil
function t:test_readConfigNonExistingPath()
   local testValue = "ReadSuccess"
   local testedKey = "@sys.config.nonexistingnode.nonexistingsetting"
   
   local name = "testautoSrvCon"
   local path = targetManager.targetdir
   modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset = racon.newAsset("TestAsset")
    
      myasset.tree.APN = value
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- create job on server side
   local data = { testedKey }
   local jobID = createRetrieveData(deviceUID, data)
   sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal('function', type(ReadValues[testedKey])) -- means that values was not found on server side.
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end


-- Test: test_writeConfig
-- Description: Write a field in the system configuration tree
-- Validation: 
--     - All setup steps are OK
--     - Write job status is success
--     - field has been corectly written
function t:test_writeConfig()
  local testValue = "MyNTPWriteSuccess"
  local testedKey = "@sys.config.time.ntpserver"
  
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  local function createAsset()
     local log = require 'log' 
     local racon = require "racon"
     racon.init() -- configure the module
 
     -- create asset
     local myasset, err = racon.newAsset("TestAsset")
   
     print(err)
     myasset.tree.APN = "default"
     myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
     
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f()
 
  -- create job on server side
  local app_name = appprefix
  local settings = { { key = testedKey, value = testValue } }
  local jobID = createApplySetting(deviceUID, app_name, settings)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  -- Create a synchronize job on AVMS  
  local syncJobID = createSyncJob(deviceUID)
  sched.wait(5)
 
  rescon, err = rpcclient:call("agent.srvcon.connect")
  
  status =  platformAPI.waitJobEnd(platformTable, syncJobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues[testedKey])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end


-- Test: test_readAsset
-- Description: Read a field of an assert tree
-- Validation: 
--     - All setup steps are OK
--     - RetrieveData job status is success
--     - field content has been corectly read
function t:test_readAsset()
   local testValue = "ReadSuccess"
   
   local name = "testautoSrvCon"
   local path = targetManager.targetdir
   modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
   
   -- Start the Agent and get a RPC connection on it
   -- Then initialize the device (create the asset to read)
   targetManager:start()
   sched.wait(2)
   local rpcclient = targetManager:getrpc(false)
   local function createAsset(value)
      local log = require 'log' 
      local racon = require "racon"
      racon.init() -- configure the module
  
      -- create asset
      local myasset,err = racon.newAsset("TestAsset")
    
      print(err)
      myasset.tree.APN = value
      myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
      
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f(testValue)
 

  -- create job on server side
  --local app_name = appprefix
   local data = { "TestAsset.APN" }
   local jobID = createRetrieveData(deviceUID, data)
   sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues["TestAsset.APN"])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end



-- Test: test_writeAsset
-- Description: Write a field in an asset tree
-- Validation: 
--     - All setup steps are OK
--     - Write job status is success
--     - field has been corectly written
function t:test_writeAsset()
  local testValue = "WriteSuccess"
  
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  local function createAsset()
     local log = require 'log' 
     local racon = require "racon"
     racon.init() -- configure the module
 
     -- create asset
     local myasset = racon.newAsset("TestAsset")
   
     myasset.tree.APN = "default"
     myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
     
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end

     local d = require "agent.devman"
     d.asset.tree.commands.Reboot = function() print "Reboot requested - Ignored for testing"; return 0 end

     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f()
 
  -- create job on server side
  local app_name = appprefix
  local settings = { { key = "TestAsset.APN", value = testValue } }
  local jobID = createApplySetting(deviceUID, app_name, settings)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  -- Create a synchronize job on AVMS  
  local syncJobID = createSyncJob(deviceUID)
  sched.wait(5)
 
  rescon, err = rpcclient:call("agent.srvcon.connect")
  
  status =  platformAPI.waitJobEnd(platformTable, syncJobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()

  -- Check results
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues["TestAsset.APN"])
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)
end



-- Test: test_assetCommand
-- Description: Call a command associated to an asset
-- Validation: 
--     - All setup steps are OK
--     - Command execution status is successful on server side
--     - field has been correctly written
function t:test_assetCommand()
  local testValue = "MyCustomCarrier"
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  local function createAsset()
     local log = require 'log' 
     local racon = require "racon"
     assert(racon.init()) -- configure the module
 
     -- create asset
     local myasset = assert(racon.newAsset("TestAsset"))
     
     myasset.tree.APN = "default"
     myasset.tree.commands.SetAPN = function(asset, data, path) asset.tree.APN = data["APNValue"]; return 0; end
     
     -- register asset
     local status, err = myasset:start()
     if not status then log("TestAsset", "ERROR", "Error when starting the asset: "..err) else log("TestAsset", "INFO", "Asset successfully registered") end
          
     return status, err
  end
    
  local f = rpcclient:newexec(createAsset)
  local assetResultStatus, assetErr = f()
 
  -- create job on server side
  local app_name = appprefix
  local commandId = "TestAsset.SetAPN"
  local parameters = { APNValue = testValue }
  local jobID = createCommand(deviceUID, app_name, commandId, parameters)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  -- Retrieve the data set by the command
  local data = { "TestAsset.APN" }
  local jobID = createRetrieveData(deviceUID, data)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  local ReadValues, errReadValues = platformAPI.dataValues(platformTable, { uid = deviceUID })
  
  targetManager:stop()
  
  -- Check results
  u.assert_equal("ok", assetResultStatus)
  u.assert_nil(assetErr)
  u.assert_not_nil(ReadValues, errReadValues)
  u.assert_equal(testValue, ReadValues["TestAsset.APN"])
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("SUCCESS", status)

end



-- Test: test_unkownCommand
-- Description: Call an unknown command (only unkown on device side)
-- Validation: 
--     - All setup steps are OK
--     - Command execution status is error
function t:test_unkownCommand()
  local name = "testautoSrvCon"
  local path = targetManager.targetdir
  modifyDefaultConfigFile(pltconf.avmstcp, name, "None", "None", path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  local app_name = appprefix
  local commandId = "@sys.UnkCommand"
  local parameters = { ValueToSet = "alpha-omega" }
  
  -- create job on server side
  local jobID = createCommand(deviceUID, app_name, commandId, parameters)
  sched.wait(5)
  
  -- force device to connect and get the job
  local rescon, err = rpcclient:call('agent.srvcon.connect')
  local status =  platformAPI.waitJobEnd(platformTable, jobID, 2*60)
  
  targetManager:stop()
  
  
  -- Check results
  u.assert_not_nil(rescon, err)
  u.assert_equal("ok", rescon) -- "ok" is expected as the only correct result of "agent.srvcon.connect()"
  u.assert_not_nil(status)
  u.assert_equal("unknown FAILURE", status)

end

return {init = init}