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
local system        = require 'utils.system'


local t = u.newtestsuite 'Server Connector'
local targetManager = nil
local defaultport = 8097
local baseurl = "http://localhost:"..defaultport.."/"
local deviceUID = nil
local ServerDeviceName = "DevicetestautoUpdate"



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
      name =  "MihiniModel_None_None",
      type = "MihiniModel",
      revision = "None_None"
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


local function createUpdateJob(uid, name, version)
  u.assert_not_nil(uid)
  
  local appUID, err = platformAPI.getApplicationUID(platformTable, name, version)
  u.assert_not_nil(appUID, err)
  
  local updateJob = {
    systems = {
       uids = { uid }
    },
    application = appUID
  }
  
  --local operationTable,errmess = platformAPI.customCommand(platformTable, updateJob)
  local operationTable,errmess = platformAPI.installApplication(platformTable, updateJob)
  
  u.assert_equal("200HTTP/1.1 200 OK", errmess)
  
  local updateJobID = operationTable.operation
  print("Asset Install operation created: " .. operationTable.operation)
  
  u.assert_not_nil(updateJobID)
  return updateJobID
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
  
  
  deviceUID = registerDevice(ServerDeviceName, "")
  u.assert_not_nil(deviceUID)
end

function t:teardown()
  os.execute("mv "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  platformAPI.delDevice(platformTable, ServerDeviceName)
  
  loader.unload("agent.defaultconfig")
end



local function setup_test(pkgname, expectedresult, producePkg)
    
    -- Assert parameters validity
    u.assert_not_nil(pkgname)
    u.assert_not_nil(expectedresult)
    u.assert_not_nil(producePkg)
    
    local function getUpdateResult()
        local update = require"agent.update.common"
        if (not update.data) then
            return nil, "Update Data table is nil"
        elseif (not update.data.swlist) then
            return nil, "Update SWList table is nil"
        elseif (not update.data.swlist.lastupdate) then
            return nil, "Last Update info table is nil"
        else
            return update.data.swlist.lastupdate.result
        end
    end

    -- copy test package to drop directory
    system.mktree(targetManager.targetdir .. "/runtime/update/drop/")
    producePkg(pkgname)

    --     if (expectedresult ~= 453) then
--         local command = "cd " .. targetManager.targetdir .. "/runtime/updatepkgs/".. pkgname .. " && tar -cvzf ".. pkgname ..  ".tar * && mv " .. pkgname .. ".tar " .. targetManager.targetdir .. "/runtime/update/drop/updatepackage.tar"
--         os.execute(command)
--     else
--         os.execute("cp " .. targetManager.targetdir .. "/runtime/updatepkgs/".. pkgname .. " " .. targetManager.targetdir .. "/runtime/update/drop/updatepackage.tar")
--     end
    
    -- Start the Agent and get a RPC connection on it
    targetManager:start()
    sched.wait(2)
    local rpcclient = targetManager:getrpc(false)
  
    local f = rpcclient:newexec(getUpdateResult)
    local updateresult,err = f()
  
    targetManager:stop()
  
    u.assert_not_nil(updateresult,err)
    u.assert_equal(expectedresult, updateresult)
end

-- Function:    createPackage
-- Description: Create the tarball package corresponding to the pkgname. The pkgname should be the directory name of package.
--              It creates a tarball from the directory content and then move it to the update/drop folder.
-- Return:      Nothing
local function createPackage(pkgname)
    local command = "cd " .. targetManager.targetdir .. "/runtime/updatepkgs/".. pkgname .. " && tar -cvzf ".. pkgname ..  ".tar * && mv " .. pkgname .. ".tar " .. targetManager.targetdir .. "/runtime/update/drop/updatepackage.tar"
    os.execute(command)
end

-- Function:    usePackage
-- Description: Copy the package to the update/drop folder
-- Return:      Nothing
local function usePackage(pkgname)
    os.execute("cp " .. targetManager.targetdir .. "/runtime/updatepkgs/".. pkgname .. " " .. targetManager.targetdir .. "/runtime/update/drop/updatepackage.tar")
end
    
local function localupdate_test()
end

function t:test_UpdateOk()
    setup_test("200-updatepackage", 200, createPackage)
end


-- Cannot parse the update file name correctly
function t:test_Error451()
    --localupdate_test("451-updatepackage.tar", 451)
end

-- Cannot create folder to extract update package
function t:test_Error452()
    --setup_test("452-updatepackage.tar", 452)
end

-- Cannot extract update package
function t:test_Error453()
    setup_test("453-updatepackage.tar", 453, usePackage)
end

-- Cannot open manifest file
function t:test_Error454()
    setup_test("454-updatepackage", 454, createPackage)
end

-- Cannot read manifest file : Not able to test it (change of properties on file)
-- function t:test_Error455()
--     
-- end

-- Cannot load manifest chunk
function t:test_Error456()
    setup_test("456-updatepackage", 456, createPackage)
end

-- Manifest file is not valid lua file
function t:test_Error457()
    setup_test("457-updatepackage", 457, createPackage)
end

function t:test_Error458()
    setup_test("458-updatepackage", 458, createPackage)
end

-- Dependency error for a component
function t:test_Error459()
    setup_test("459-updatepackage", 459, createPackage)
end

-- function t:test_Error461()
-- end
-- 
-- function t:test_Error462()
-- end
-- 
-- function t:test_Error463()
-- end
-- 
-- function t:test_Error464()
-- end
-- 
-- function t:test_Error465()
-- end
-- 
-- function t:test_Error471()
-- end
-- 
-- function t:test_Error472()
-- end
-- 
function t:test_Error474()
     setup_test("474-updatepackage", 474, createPackage)
 end

function t:test_Error475()
     setup_test("475-updatepackage", 475, createPackage)
 end

function t:test_Error476()
     setup_test("476-updatepackage", 476, createPackage)
 end


return {init = init}
