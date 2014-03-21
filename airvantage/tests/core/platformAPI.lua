-- This file implements an abstraction layer for communicating with the platform.
-- It allows to connect to the platform, create jobs, etc.

local p      = p
local print  = print
local assert = assert
local sched  = require 'sched'
local yajl   = require 'yajl'
local http   = require "socket.http"
local ltn12  = require "ltn12"
local mime   = require "mime"
local socketurl = require("socket.url")

-- public API definition --------------------------------
local platformAPI = {}

-- function definitions ----------------------------------------------------------------------
-- 1- internal functions (not published to the rest of Serenity)
-- 2- public functions
----------------------------------------------------------------------------------------------
-- 1- internal functions (not published to the rest of Serenity) -----------------------------
----------------------------------------------------------------------------------------------
-- The function hTTPRequest is an internal function used to
-- factorize code (avoiding printing the URL and return values every time).
-- Here the basic http method is used: http.request(url [, body]).
-- The inputs and outputs are exactly the same than socket.http.request function
-- but result is transformed into lua objects (instead of JSON string)
local function hTTPRequest(url,body)
   local errmess = ""
   if body then print("with body: "..body) end -- The body is optional
   --url = socketurl.escape(url)
   local resultJSON, statusCode, headers, statusLine = http.request(url, body)
   local result = {}
   if resultJSON then
     result  = yajl.to_value(resultJSON)
     errmess = statusCode..statusLine
   else
      p("HTTP request error: statusCode=",statusCode,"headers=",headers,"statusLine=",statusLine)
      result = nil
      if statusLine then
	errmess = statusCode..statusLine
      else
	errmess = statusCode
      end
   end
   return result, errmess
end
-----------------------------------------------------------------------------------------------------------------------
-- The function advancedHTTPRequest is an internal function used to
-- factorize code (avoiding printing the URL and return values every time).
-- Here the generic http method is used: http.request{
--                                                    url = string,
--                                                    [sink = LTN12 sink,]
--                                                    [method = string,]
--                                                    [headers = header-table,]
--                                                    [source = LTN12 source],
--                                                    [step = LTN12 pump step,]
--                                                    [proxy = string,]
--                                                    [redirect = boolean,]
--                                                    [create = function]}
-- The inputs and outputs are exactly the same than socket.http.request function
-- but result is transformed into lua objects (instead of JSON string)
local function advancedHTTPRequest(platformTable, httpUrl, httpBodyJSON)
  local errmess = ""
  local returnValue = {}
  local httpResponse = {}
  local myRequest = {
    url     = httpUrl.."?access_token="..platformTable.accessToken,
    sink    = ltn12.sink.table(httpResponse),
    method  = "POST",
    headers = { ["content-Type"]   = "application/json",
                ["content-length"] = #httpBodyJSON}, -- The length is needed because the body is sent chunked
    source  = ltn12.source.string(httpBodyJSON) }
  local result, statusCode, headers, statusLine =  http.request(myRequest)
  local returnValueJSON = table.concat(httpResponse) -- concatenate httpResponse because it is chunked 
  local returnValue = {}
  if returnValueJSON then
      returnValue = yajl.to_value(returnValueJSON)
      errmess = statusCode..statusLine
  else
      p("HTTP advanced request error: statusCode=",statusCode,"headers=",headers,"statusLine=",statusLine)
      returnValue = nil
      if statusLine then
	errmess = statusCode..statusLine
      else
	errmess = statusCode
      end
  end
    return returnValue, errmess
end
--------------------------------------------------------------------------------------------------------------------
-- The function keepAlive() allows to keep alive the connection
-- between the the platform and the host machine where this code is executed.
-- This function should be called only after a 1st connection was made.
-- This function never ends.
local function keepAlive(platformTable)
  while true do -- refresh token before it expires
    sched.wait(platformTable.accessTokenExpiresIn-1000)
    local result, statusCode, headers, statusLine =  
	    http.request(platformTable.serverURL.."oauth/token?grant_type=password&username="..platformTable.userName..
	    "&password="..platformTable.password.."&client_id="..platformTable.clientId.."&client_secret="..platformTable.clientSecret)
    if result then
      platformTable.accessToken          = result["access_token"]
      platformTable.tokenType            = result["token_type"]
      platformTable.refreshToken         = result["refresh_token"]
      platformTable.accessTokenExpiresIn = result["expires_in"]
    end
  end 
end
-----------------------------------------------------------------------------------------------------------------
-- The function getApplicationID() retrieves the uid of an application
-- from the given platform. The application is searched by its name blablalba
-- Arguments: platformTable:      table describing the platform on which look for the application
--                                (the application name and revision is given in that platformTable 
--            dUTplatform:        device table on which to find relevant information
--            jobIsFWupdate:      boolean defining if the job is a FW update (the application is not installed on 
--                                on the device) or a readData/applySetting (the appID is on the device table)
--            firmwareToInstall:  string containing the firmware revision/version to install (used only if it is a FW update job)
-- Retrun value: The uid that identifies the application


local function getApplicationID(platformTable, dUTplatform, jobIsFWupdate, firmwareToInstall)
  local result, errmess = nil, nil
  -- if the job is a firmware update => the application to identify is listed within the platform applications
  if jobIsFWupdate then
    -- ask to the platform the ID of the application having, for example, "ALEOS GX400" in its name
    local url = platformTable.serverURL.."v1/applications?access_token="..platformTable.accessToken.."&name="..
                string.gsub(firmwareToInstall.modelDevice,"%s+",'%%20').."&revision="
    -- depending on the firmware currently install, choose between firmwareToInstall.upgradeversion and firmwareToInstall.downgradeversion
    -- sting.find() function used in order to handle both "4.3.2a.002" and "4.3.2a.002-I" cases
    if not string.find(firmwareToInstall.currentVersion,firmwareToInstall.upgradeVersion) then
      url = url..firmwareToInstall.upgradeVersion
    else
      url = url..firmwareToInstall.downgradeVersion
    end
    result, errmess = hTTPRequest(url)
    -- if the application answers a non empty list, take the ID of the first element
    if result and result.items and result.items[1] then
      return result.items[1].uid
    else
      return nil,"CreateJob - Software Update ERROR: the application (revision "..
        firmwareToInstall.upgradeversion..") to install on the device was not found, maybe it is not installed on the platform"
    end
    
  -- if the job is not a firmware update => the application to identify is already installed on the device
  else 
    for k,v in pairs(dUTplatform.applications) do
      if string.find(v.name,"ALEOS") then
	return v.uid
      end
    end
  end
end
---------------------------------------------------------------------------------------------------------------------
-- The function createJob handles creation of different job types: Apply Settings, Retrieve Data, etc.
-- Depending on the job type, it will contact the platform on the right URL with the correct parameters.
-- This function does not wait for the job to finishes, but returns the operationID that will allows another
-- function waitJobEnd() to do that.
-- Arguments: platformTable:       table describing the platform on which look for the application
--            jobType:             string defining the type of job. Possible values are "RetrieveData" and "ApplySettings"
--            dUTplatform:         device object
--            MSCIsAndValuesTable: table containing the MSCIs as string keys and values as values,
--                                 e.g. {["1034"] = 60123, ["1035"] = 13}
--            firmwareToInstall:   table containing the firmware version to be installed. Format is
--                                 local firmwareToInstall = {
--                                    ["downgradeversion"] = "4.3.2.001"
--                                    ["upgradeversion"]   = "4.3.2.002"}

-- TODO: handle device initiated vs server initiated

local function createJob(platformTable, jobType, dUTplatform, MSCIsAndValuesTable, firmwareToInstall)
  local httpBody       = {}
  local httpBodyJSON   = {}
  local uRL            = ""
  local operationID    = ""
  local iteration      = 0
  local  appID,errmess = "","" -- identify the application to which the job relates to

  -- Depending on the job type, the http request will be different, 
  -- i.e. different URLs and bodies have to be defined
  -----------------------------------------------------------------------------------------------
  if jobType == "RetrieveData" then
    appID,errmess = getApplicationID(platformTable,dUTplatform,false)
    if appID then
      uRL = platformTable.serverURL.."v1/operations/systems/data/retrieve"
      httpBody = {
        --   applicationUid = device.applications[1].uid, 
	applicationUid  = appID,
	applicationName = "ALEOS GX400",
	systems         = {uids = {dUTplatform.uid}},
	data            ={}}
      iteration = 1
      for k,v in pairs(MSCIsAndValuesTable) do
	httpBody.data[iteration] = tostring(k)
	iteration = iteration + 1
      end
    else
      return nil,errmess
    end
 -----------------------------------------------------------------------------------------------
    
  elseif jobType == "ApplySettings" then
    appID,errmess = getApplicationID(platformTable,dUTplatform,false)
    if appID then
      uRL = platformTable.serverURL.."v1/operations/systems/settings"
      httpBody = {
        applicationUid  = appID, 
        applicationName = "ALEOS GX400",
        systems         = {uids = {dUTplatform.uid}},
        settings        = {}} --platform API request a list of tables here
      iteration = 1
      for k,v in pairs(MSCIsAndValuesTable) do
        httpBody.settings[iteration] = {key = tostring(k), value = tostring(v)}
        iteration = iteration + 1 
      end
    else
      return nil,errmess
    end
------------------------------------------------------------------------------------------------------
    
  elseif jobType == "SoftwareUpdate" then
    appID,errmess = getApplicationID(platformTable,dUTplatform,true,firmwareToInstall)
    if appID then  
      uRL = platformTable.serverURL.."/v1/operations/systems/applications/install"
      httpBody = {
        application = appID,
        systems = {uids = {dUTplatform.uid}}}
    else
      return nil,errmess
    end

----------------------------------------------------------------------------------------------------------------------------------   
 ---------job wake up
  elseif jobType == "WakeUp" then
      uRL = platformTable.serverURL.."/v1/operations/systems/wakeup"
      httpBody = {
	action= 'READYAGENT_DM_CONNECT',   
	systems = {uids = {dUTplatform.uid}}}
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------    
  --job sendSMS
  elseif jobType == "SendSMS" then
    uRL= platformTable.serverURL.."/v1/operations/systems/sms" 
     httpBody = {systems = {uids = {dUTplatform.uid}}}
      
-------------------------------------------------------------------------------------------------------------------------
  elseif jobType == "Reboot" then
      uRL = platformTable.serverURL.."/v1/operations/systems/reboot"
      httpBody = 
    {
      action  = "ALEOS_REBOOT",
      systems = {uids = {dUTplatform.uid}}
    }
  
  ----------------------------------------------------------------------------------------------------------------------
  else
    return nil, "jobType was unrecognized"
  end
-------------------------------------------------------------------------------------------------------------------------

  -- After URLs and bodies are defined, send the HTTP request
  httpBodyJSON = yajl.to_string(httpBody)
  local operationTable,errmess =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
  
  if operationTable and operationTable.operation then
    operationID = operationTable.operation
  else
    return nil,"createJob(): HTTP request returned "..errmess
  end
  return operationID
end
----------------------------------------------------------------------------------------------
-- 2- public functions -----------------------------------------------------------------------
----------------------------------------------------------------------------------------------

-- The function new() allows to configure the attributes for connection to plateform
-- Return value: platform table
local function new(configuration)
  local platformTable = {}
  for k,v in pairs(configuration) do
    platformTable[k] = v
  end
  return platformTable
end
------------------------------------------------------------------------------------------------------------------------------------------------------
-- The function getAccessToken() allows to connect to the platform.
-- This function has to be called before any other action on the platform.
-- This function creates a new thread in order to keep alive the token.
local function getAccessToken(platformTable)
 
  local result, errmess =  
        hTTPRequest(platformTable.serverURL.."oauth/token?grant_type=password&username="..
		    platformTable.userName.."&password="..platformTable.password.."&client_id="..
		    platformTable.clientId.."&client_secret="..platformTable.clientSecret,nil)
  -- Set platformAPI attributes with the return values from HTTP.
  local returnToCaller = {}
  if result then
    platformTable.accessToken          = result["access_token"]
    platformTable.tokenType            = result["token_type"]
    platformTable.refreshToken         = result["refresh_token"]
    platformTable.accessTokenExpiresIn = result["expires_in"]
    -- Call keepAlive() internal function to maintain a valid connection with the platform
    -- This call creates a new thread.
    sched.run(keepAlive, platformTable)
    returnToCaller = true
  else
    returnToCaller = false
  end
 
  return returnToCaller,errmess
end
-----------------------------------------------------------------------------------------------------------------------------------------------------
-- The function getDevice browses the platform and asks for the concerned device.
-- The device is identified by its serial number.
-- Moreover, this function sets the deviceUnderTest attribute of platform objetc if the device was found.
-- Arguments: platformTable: the platform instance returned by new() function
-- Return value: device: device object on succes. nil on error cases.
local function getDevice(platformTable)
 -- print("platformAPI.getDevice() called")
  -- request to the platform the status/URL/etc of the device
  local result, errmess =  
	hTTPRequest(platformTable.serverURL.."v1/systems?access_token="..
		    platformTable.accessToken.."&gateway=serialNumber:"..platformTable.deviceSN)
  if not result then
     return nil,"platformAPI.getDevice() cannot find the device: "..errmess
  else
     platformTable.deviceUnderTest = result.items[1]
  end
  return platformTable.deviceUnderTest

end
---------------------------------------------------------------------------------------------------------------------------------------------
-- The function waitJobEnd() aims to wait for the end of an operation. Once the operation finishes,
-- this function returns the results of the operation.
-- Argument: operationID: string containing the operation ID. This ID is generated by createJob() function.
-- Return: status: task/operation status
local function waitJobEnd(platformTable, operationID, timeout)
-- Once the operation is created, loop until the operation is executed by the platform (i.e. FINISHED status)
  local uRL = platformTable.serverURL.."v1/operations/?access_token="..platformTable.accessToken.."&uid="..operationID
  local status = ""
  local start = os.time()
  
   if timeout == nil then
    timeout = 20*60 --default parameter 20 mn(20*60) timeout for the other job
   end
   
  repeat
    sched.wait(10)
    local statusTable, errmess =  hTTPRequest(uRL)
    if statusTable then status = statusTable.items[1].state end
    local current = os.time()
  until ((current - start >= timeout) or (status == "FINISHED"))

  if status == "FINISHED" then
    status = "" -- reset the string as it will be used for loging successes or failures
    --Once the operation is FINISHED, test the outcome of the operation against any errors
    uRL = platformTable.serverURL.."v1/operations/"..operationID.."/report?format=JSON&access_token="..platformTable.accessToken
   local reportTable, errmess =  hTTPRequest(uRL)
    if not reportTable then
      return errmess
    else
      -- if there is no failed task
      if reportTable.counters.failure == 0 then
	status = reportTable.tasks[1].state --"SUCCESS"
      else
      -- if at least one task failed, retrieve the related error messages
      -- the error messages are stored on different locations
      -- depending on the job type (and may be the error type too)
	if reportTable.type == "RETRIEVE_DATA" then
	  for k,v in pairs(reportTable.tasks) do
	    if v.state == "FAILURE" then 
	      for i,j in pairs(v.messages) do
	        if j.error then status = j.error.." - "..status end
	      end
	    end
	  end
	  
	elseif reportTable.type == "APPLY_SETTINGS" then
	  for k,v in pairs(reportTable.parameters) do
	    for i,j in pairs(v) do
	      if ((j.setting) and (not j.setting.value)) then status = j.setting.." - "..status end
	    end
	  end
	  
	elseif reportTable.type == "INSTALL" then
        p(reportTable)
	  status = reportTable.tasks[1].messages[2].error.." - ".. reportTable.tasks[1].messages[1].system.name..
	           " - "..reportTable.parameters[1][1].application.revision
 
        -- TODO: handle more error cases:
	else
	  status = "unknown FAILURE"
	end
      end 
    end
  else status = "manual TIMEOUT from AVATAR"
  end

  return status
end
-------------------------------------------------------------------------------------------------------------------------------------------------
--The function retrieveData triggers an read job on the platform by calling createJob
--Arguments: dUTplatform: device object previously returned by getDevice() function
--           id:          integer containing the MSCI ID to read

local function retrieveData(platformTable, dUTplatform, MSCIsAndValuesTable)
  
  local operation,err = createJob(platformTable, "RetrieveData", dUTplatform, MSCIsAndValuesTable, nil)
 
  return operation,err
end
-------------------------------------------------------------------------------------------------------
-- The function applySettings triggers an apply settings job on the platform by calling createJob.
-- Arguments: dUTplatform:         device object previously returned by getDevice() function
--            MSCIsAndValuesTable: table containing the MSCIs as string keys and values as values,
--                                 e.g. {["1034"] = 60123, ["1035"] = 13}
local function applySettings(platformTable, dUTplatform, MSCIsAndValuesTable)
 
  local operation,err= createJob(platformTable, "ApplySettings", dUTplatform, MSCIsAndValuesTable, nil)
 
  return operation,err
end
-------------------------------------------------------------------------------------------------------
--this functions is used to do an upgrade or an downgrade of the veriosn of firmware on the platform.
local function softwareUpdate(platformTable,dUTplatform,firmwareToInstall)

  local operation,err = createJob(platformTable, "SoftwareUpdate", dUTplatform, nil, firmwareToInstall)
 
  return operation,err
  
end
--------------------------------------------------------------------------------------------------
--this function is used to do a wakeup on the platform to permit the platform to communicate with the device
local function wakeup(platformTable, dUTplatform)
  
  local operation,err = createJob(platformTable, "WakeUp", dUTplatform, nil, nil)
  
  return operation,err
  
end
-------------------------------------------------------------------
local function sendsms(platformTable, dUTplatform)
  
  local operation,err = createJob(platformTable, "SendSMS", dUTplatform, nil, nil)  
  
  return operation,err
  
end
----------------------------------------------------------------------------------------------------------------------------------------
local function reboot (platformTable, device)
  
  local operation,err = createJob (platformTable,"Reboot", device, nil, nil)
  
  return operation,err
  
end
-----------------------------------------------------------------------------------------------------------------------------------------
-- This function retrieves a table containing the latest messages
-- between the platform and a given device.
-- Arguments:     device: device object on which look for messages
-- Return values: latestMessages: a table containing the latest message between the platform and the given device
local function getLatestMessages(platformTable, device)
  local uRL = platformTable.serverURL.."v1/systems/"..device.uid.."/messages"..
        "?format=JSON&access_token="..platformTable.accessToken
  local latestMessages, errmess = hTTPRequest(uRL)
  if not latestMessages then
     return nil,"platformAPI.getLatestMessages(): error during HTTP request: "..errmess
  end
  return latestMessages, errmess
end
-----------------------------------------------------------------------------------------------------------------------------------------
-- This function retrieves a table containing all information on a single message
-- between the platform and a given device.
-- Arguments: platformTable: table describing the platform to request     
--            device       : device object on which look for messages
--            messageUID   : string containing the ID of the message (to get it request the platform
--                                                                    by using getLatestMessages())
-- Return values: messageTable: a table containing the information on the given message
local function getMessageInformation(platformTable, device, messageUID)
  local uRL = platformTable.serverURL.."v1/systems/"..device.uid.."/messages/"..messageUID..
        "?format=JSON&access_token="..platformTable.accessToken
  local messageTable, errmess = hTTPRequest(uRL)
  if not messageTable then
     return nil,"platformAPI.getMessageInformation(): error during HTTP request: "..errmess
  end
  return messageTable, errmess
end
--------SECOND VERSION OF TEST--------------------------------------------------------------------------------
-- dataValues() function returns the MSCI values stored on a platform.
-- Arguments: platformTable:       table identifying which platform should be requested
--            device:              device object for which the data should be retrieved
--            MSCIsAndValuesTable: table containing the MSCI data to retrieve. If nil, all system data will be retrieved
-- Returned values: msciRead:      table containing every data of this system on the plateform
--                                 keys are MSCIID and values are the values on the plateform
local function dataValues(platformTable, device, MSCIsAndValuesTable)
  local uRL, result, errmess,buffer= nil, nil, nil,nil
  -- if some specific data is to be retrieved, then fill in buffer with the data
  -- if not, buffer will stay an empty string and all data will be retrieved
if MSCIsAndValuesTable ~= nil then
  for k,v in pairs(MSCIsAndValuesTable) do
    if k ~= "0" then
       if buffer then
	   buffer=buffer..","..k  --ID
       else
	   buffer=""..k   
       end
    
   else buffer = ""
   end
  end
end 

  if (buffer) then
    uRL = platformTable.serverURL.."v1/systems/"..device.uid.."/data?ids="..buffer.."&access_token="..platformTable.accessToken
  else
    uRL = platformTable.serverURL.."v1/systems/"..device.uid.."/data?access_token="..platformTable.accessToken
  end
  
  result, errmess =  hTTPRequest(uRL)
  -- if the HTTP request is answered correctly,
--   -- parse the answer to extract a table containing the values
  if result then
    local msciRead = {}
    for k,v in pairs(result) do
      msciRead[k] = v[1].value
    end
    return msciRead
  else
    return nil,errmess
  end
end
-----------------------------------------------------------------------------------------------------------
--this function is use to count the number of ID in the table of retrieve or apply
function countElementTable(table)
 local numberID =0
for k,v in pairs(table) do
  numberID=numberID+1
end
return numberID
 end
 -------------------------------------------------------------------------------------------------------------------------------
--function to delete a system on a device with the gateway and the subscription
local function delDevice (platformTable, sN)
  local plt =""
  local sysuid = ""
  if not sN then sN = "CA1196149301002" end --serial number of the device to delete on the platform
--this part permit to find the serial Number of the device and compare that with the sN defined.
 local result, statusCode, headers, statusLine =  
    hTTPRequest(platformTable.serverURL.."v1/systems?access_token="..platformTable.accessToken.."&gateway=serialNumber:"..sN.."&fields=uid")
 if result and result.items and result.items[1] then
   plt = result.items[1]
sysuid=plt.uid
 else
  local operation, err = platformAPI.register(platformTable)
  -- return nil,"Platform did not found a device whose serial number is "..sN
 end
--delete the system with the gateway
local httpResponse = {}
local request= {
  url = platformTable.serverURL.."v1/systems/"..sysuid.."?access_token="..platformTable.accessToken.."&deleteGateway=true".."&deleteSubscriptions=true",
  method="DELETE",
 sink = ltn12.sink.table(httpResponse)
 }
local result,err = http.request(request)

return result,err

end
----------------------------------------------------------------------------------
--function to register a device on the platform
local function register(platformTable)
  local httpBody       = {}
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/gateways"
  httpBody = 
  {
  serialNumber = "CA1196149301002"
   }
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to create a system on the platform
local function registerSystem(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/systems"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to activate a list of systems on the platform
local function activateSystems(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/activate"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to synchronize a list of systems on the platform
local function syncSystems(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/synchronize"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to reset to factory values a list of systems on the platform
local function resetSystems(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/reset"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to retreave data of generic systems on the platform
local function retreiveSystemData(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/data/retrieve"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
return result,err

end

----------------------------------------------------------------------------------
--function to retreave data of generic systems on the platform
local function applySystemSettings(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  --uRL = platformTable.serverURL.."v1/operations/systems/data/settings"
  uRL = platformTable.serverURL.."v1/operations/systems/settings"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
  return result,err
end

----------------------------------------------------------------------------------
--function to create a custom defined command on the platform
local function customCommand(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/command"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
  return result,err
end

----------------------------------------------------------------------------------
--function to create an install job on the platform
local function installApplication(platformTable, httpBody)
  assert(httpBody)
  local httpBodyJSON   = {}
  local uRL            = ""
  uRL = platformTable.serverURL.."v1/operations/systems/applications/install"
  httpBodyJSON = yajl.to_string(httpBody)
  local result,err =  advancedHTTPRequest(platformTable, uRL, httpBodyJSON) 
  return result,err
end

local function getApplicationUID(platformTable, name, revision)
  local result, errmess = nil, nil
  -- ask to the platform the ID of the application having, for example, "ALEOS GX400" in its name
  local url = platformTable.serverURL.."v1/applications?access_token="..platformTable.accessToken.."&name="..name.."&revision="..revision
  
  result, errmess = hTTPRequest(url)
p("Application search",result)

  -- if the application answers a non empty list, take the ID of the first element
  if result and result.items and result.items[1] then
    return result.items[1].uid
  else
    return nil,"GetApplicationID ERROR: the application "..name.."(revision "..revision..") was not found, please check platform"
  end

end
 
 ----------------------------------------------------------------------------------------------------------------------------
-- list of public functions --------------------------------
platformAPI.new                     = new
platformAPI.getAccessToken          = getAccessToken
platformAPI.getDevice               = getDevice
platformAPI.retrieveData            = retrieveData
platformAPI.applySettings           = applySettings
platformAPI.waitJobEnd              = waitJobEnd
platformAPI.softwareUpdate          = softwareUpdate
platformAPI.wakeup                  = wakeup
platformAPI.getLatestMessages       = getLatestMessages
platformAPI.getMessageInformation   = getMessageInformation
platformAPI.dataValues              = dataValues
platformAPI.sendsms                 = sendsms
platformAPI.reboot                  = reboot
platformAPI.delDevice               = delDevice
platformAPI.countElementTable       = countElementTable
platformAPI.register                = register
platformAPI.registerSystem          = registerSystem
platformAPI.activateSystems         = activateSystems
platformAPI.syncSystems             = syncSystems
platformAPI.resetSystems            = resetSystems
platformAPI.retreiveSystemData      = retreiveSystemData
platformAPI.applySystemSettings     = applySystemSettings
platformAPI.customCommand           = customCommand
platformAPI.installApplication      = installApplication
platformAPI.getApplicationUID       = getApplicationUID

return platformAPI
