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
local http          = require 'socket.http'
local loader        = require 'utils.loader' 


local t = u.newtestsuite 'Rest Integration'
local targetManager = nil
local defaultport = 8097
local baseurl = "http://localhost:"..defaultport.."/"



local function modifyDefaultConfigFile(path)
  -- load the file
  local defconf = require "agent.defaultconfig"

  defconf.rest = {}
  defconf.rest.activate = true
  defconf.rest.port = defaultport
  
  os.execute("rm -rf "..path.."/runtime/persist")
  os.execute("rm -rf "..path.."/runtime/crypto")
  
  local s = "local config = " .. sprint(defconf) .. "\nreturn config"
  local file,err = io.open(path .. "/runtime/lua/agent/defaultconfig.lua", "w")
  u.assert_not_nil(file, err)
  file:write(s)
  file:flush()
  file:close()
  
  return defconf
end


local function getDT(key)
    local dt = require 'devicetree'
    dt.init()
    return dt.get(key)
end


function init(tm)
  targetManager = tm
end



function t:setup()
  u.assert_not_nil(targetManager)
  u.assert_not_nil(targetManager.config)
  
  -- backup defaultconfig file
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  os.execute("cp "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup")
  
end

function t:teardown()
  os.execute("mv "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  
  loader.unload("agent.defaultconfig")
end


function t:test_ReadDeviceTreeConfig()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  local url = baseurl.."devicetree/config"
  local r, c, h = http.request(url)
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is not a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(200, c)
    
  local configtab = yajl.to_value(r)[2]
  
  table.sort(confref)
  table.sort(configtab)
  -- compare reference to current content
  u.assert_clone_tables(confref, configtab)
end

function t:test_ReadDeviceTreeServerDot()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/config.server"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is not a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(200, c)
    
  local configtab = yajl.to_value(r)[2]
  
  table.sort(confref)
  table.sort(configtab)
  -- compare reference to current content
  u.assert_clone_tables(confref, configtab)
end

function t:test_ReadDeviceTreeServerUrlDot()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/config.server.url"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")

    
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(200, c)
  
  local configtab = yajl.to_value(r)[1]
  -- compare reference to current content
  u.assert_equal(leaf, configtab)
end

function t:test_ReadDeviceTreeServerUrlSlash()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/config/server/url"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  u.assert_equal(200, c)
  u.assert_not_nil(r, "HTTP request got an empty response")
  
  
  local configtab = yajl.to_value(r)[1]
  -- compare reference to current content
  u.assert_equal(leaf, configtab)
end

function t:test_ReadDeviceTreeNoLeafDot()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/config.notexisting"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(200, c)
  
  local configtab = yajl.to_value(r)[2]
  
  -- Result shall be nil
  u.assert_nil(configtab)
end

function t:test_ReadDeviceTreeNoHandlerSlash()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/notexisting"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, err = f("notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(err)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(500, c)
  
  u.assert_equal("handler not found", r)
end


function t:test_ReadDeviceTreeNoHandlerNoLeafDot()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/nohandler.notexisting"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, err = f("nohandler.notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(err)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(500, c)
  
  u.assert_equal("handler not found", r)
end

function t:test_ReadDeviceTreeCustomName()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
  
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  local function setCustomName(key, value)
    local dt = require 'devicetree'
    dt.init()
    return dt.set("config."..key, value)
end
  local s = rpcclient:newexec(setCustomName)
  local v = {}
  v["my-leaf"] = "valid leaf"
  s("1234", v)
  
  -- Request table
  -- Request
  local url = baseurl.."devicetree/config/1234"
  local r, c, h = http.request(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.1234")

  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(r, "HTTP request got an empty response")
  u.assert_equal(200, c)
  
  local configtab = yajl.to_value(r)[2]

  table.sort(confref)
  table.sort(configtab)
  -- compare reference to current content
  u.assert_clone_tables(confref, configtab)
end






function t:test_WriteDeviceTreeConfigServer()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/server"
  local exp_url = "tcp://mycustomurl.com:35784"
  local exp_port = 8080
  local exp_serverId = "MyCustomServerId"
  local exp_autocon_period = 10
  local testserver = {
    serverId = exp_serverId,
    url = exp_url,
    port = exp_port,
    autoconnect = { period = exp_autocon_period }}
    
  local httpBodyJSON = yajl.to_string(testserver)
  local httpHeaders = { ["content-type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "PUT",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local serverleaf, servernode = f("config.server")
  local serverIdleaf, serverIdnode = f("config.server.serverId")
  local urlleaf, urlnode = f("config.server.url")
  local portleaf, portnode = f("config.server.port")
  local autoconnectleaf, autoconnectnode = f("config.server.autoconnect")
  local periodleaf, periodnode = f("config.server.autoconnect.period")

  rpcclient:call('os.exit', 0)
  local exp_servernodes = { "config.server.serverId", "config.server.url", "config.server.port", "config.server.autoconnect" }
  local exp_autoconnectnodes = {"config.server.autoconnect.period" }

  -- Assert all the results for the "config.server" table
  u.assert_equal(200, c, httpResponse[1])
  
  u.assert_not_nil(servernode)
  u.assert_nil(serverleaf)
  
  u.assert_not_nil(serverIdleaf)
  u.assert_nil(serverIdnode)
  
  u.assert_not_nil(urlleaf)
  u.assert_nil(urlnode)
  
  u.assert_not_nil(portleaf)
  u.assert_nil(portnode)
  
  u.assert_not_nil(autoconnectnode)
  u.assert_nil(autoconnectleaf)
  
  u.assert_not_nil(periodleaf)
  u.assert_nil(periodnode)
  
  u.assert_equal(exp_url, urlleaf)
  u.assert_equal(exp_port, portleaf)
  u.assert_equal(exp_serverId, serverIdleaf)
  u.assert_equal(exp_autocon_period, periodleaf)
  
  table.sort(exp_servernodes)
  table.sort(servernode)
  table.sort(exp_autoconnectnodes)
  table.sort(autoconnectnode)
  u.assert_clone_tables(exp_servernodes, servernode)
  u.assert_clone_tables(exp_autoconnectnodes, autoconnectnode)
end

function t:test_WriteDeviceTreeConfigServerUrl()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/server/url"
  local testurl = "tcp://mycustomurl.com:35784"
  local httpBodyJSON = yajl.to_string(testurl)
  local httpHeaders = { ["content-type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "PUT",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
 
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")

  rpcclient:call('os.exit', 0)

  u.assert_equal(200, c, httpResponse[1])

  -- myleaf node is a leaf
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  
  u.assert_equal(leaf, testurl)
end

function t:test_WriteDeviceTreeConfigCustomName()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/1234"
  local tbody = {}
  tbody["my-leaf"] = "valid leaf"
  local httpBodyJSON = yajl.to_string(tbody)
  local httpHeaders = { ["content-type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "PUT",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.1234.my-leaf")

  rpcclient:call('os.exit', 0)

  u.assert_equal(200, c, httpResponse[1])

  -- myleaf node is a leaf
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  
  u.assert_equal(leaf, tbody["my-leaf"])
end

function t:test_WriteDeviceTreeConfigCustomTableName()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/1234"
  local tbody = {}
  tbody["my-leaf"] = "valid leaf"
  tbody["my_node"] = {firstleaf = "top leaf", intleaf = 238976}
  local httpBodyJSON = yajl.to_string(tbody)
  local httpHeaders = { ["content-type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "PUT",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local rootleaf, rootnodes = f("config.1234")
  local myleaf, myleafnodes = f("config.1234.my-leaf")
  local mynodeleaves, finalnodes = f("config.1234.my_node")

  rpcclient:call('os.exit', 0)

  -- compare reference to current content
  u.assert_equal(200, c, httpResponse[1])

  u.assert_nil(rootleaf)
  u.assert_not_nil(rootnodes)
  u.assert_not_nil(myleaf)
  u.assert_nil(myleafnodes)
  u.assert_nil(mynodeleaves)
  u.assert_not_nil(finalnodes)
  local exp_rootnodes = {"config.1234.my-leaf", "config.1234.my_node"}
  local exp_finalnodes = {"config.1234.my_node.intleaf", "config.1234.my_node.firstleaf"}
  
  
  table.sort(exp_rootnodes)
  table.sort(rootnodes)
  table.sort(exp_finalnodes)
  table.sort(finalnodes)
  u.assert_clone_tables(exp_rootnodes, rootnodes)
  u.assert_equal(myleaf, tbody["my-leaf"])
  u.assert_clone_tables(exp_finalnodes, finalnodes)
end


function t:test_WriteDeviceTreeConfigServerUrlNoPayload()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  
  local httpBodyJSON = " "

  -- Request
  local url = baseurl.."devicetree/config/server/url"
  local httpHeaders = { ["content-type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "PUT",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  rpcclient:call('os.exit', 0)

  -- Expect an error from server
  u.assert_equal(500, c, httpResponse[1])
  u.assert_not_nil(httpResponse[1], "HTTP request got an empty response")
end

function t:test_POSTDeviceTreeConfigServer()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/myserver"
  local tbody = {}
  tbody["url"] = "http://postfail/"
  local httpBodyJSON = yajl.to_string(tbody)
  local httpHeaders = { ["content-Type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "POST",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.myserver")

  rpcclient:call('os.exit', 0)

  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(httpResponse[r], "HTTP request got an empty response")
  u.assert_equal(404, c, httpResponse[r])
  
  u.assert_equal("HTTP/POST is not supported for devicetree/config/myserver", httpResponse[r])
end


function t:test_UPDATEDeviceTreeConfigServer()
  -- Change default config file to match the test requirement
  local path = targetManager.targetdir
  local refconf = modifyDefaultConfigFile(path)
   
  -- Start the Agent and get a RPC connection on it
  targetManager:start()
  sched.wait(2)
  local rpcclient = targetManager:getrpc(false)
  

  -- Request
  local url = baseurl.."devicetree/config/myserver"
  local tbody = {}
  tbody["url"] = "http://updatefail/"
  local httpBodyJSON = yajl.to_string(tbody)
  local httpHeaders = { ["content-Type"]   = "application/json",
                        ["content-length"] = tostring(#httpBodyJSON)} -- The length is needed because the body is sent chunked
  local httpResponse = {}

  local r, c, h = http.request{
    url = url,
    method = "UPDATE",
    headers = httpHeaders,
    source = ltn12.source.string(httpBodyJSON),
    sink = ltn12.sink.table(httpResponse)}
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.myserver")

  rpcclient:call('os.exit', 0)

  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(httpResponse[r], "HTTP request got an empty response")
  u.assert_equal(404, c, httpResponse[r])
  
  u.assert_equal("HTTP/UPDATE is not supported for devicetree/config/myserver", httpResponse[r])
  end


return {init = init}