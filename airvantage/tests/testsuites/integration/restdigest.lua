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
local sys           = require 'utils.system'
local loader        = require 'utils.loader' 


local t = u.newtestsuite 'Rest Digest Integration'
local targetManager = nil
local defaultport = 8097
local baseurl = "http://localhost:"..defaultport.."/"

local function readServer(url)
  print("attempt to read "..url)
  --local fmt = [[curl -s -m 2 --digest -u testuser:custompassword --keepalive-time 10 -f --retry 3 %s]] 
  local fmt = [[curl -s --digest -u testuser:custompassword -f %s]] 
  
  local cmd = string.format(fmt, url) 
  local status, body = sys.pexec (cmd)
  p(body)

  local res = {}
  
  return status, body
end

local function sendToServer(url, method, message)
  --local fmt = [[curl -s -m 2 --digest -u testuser:custompassword --keepalive-time 10 --Retry 3 -f -X %s --header "Content-Type: application/json" --header "Content-Length:%d" -d "%s" %s]]
  local fmt = [[curl -s --digest -u testuser:custompassword -X %s --header 'Content-Type: application/json' --header "Transfer-Encoding: chunked" -d '%s' %s]] 
  local cmd = string.format(fmt, method, message, url)
  --local cmd = string.format(fmt, message, url) 
  print(cmd)
  local status, body = sys.pexec (cmd)

  local res = {}
  return status, body
end


local function modifyDefaultConfigFile(path)
  -- load the file
  local defconf = require "agent.defaultconfig"
  
  defconf.rest = {}
  defconf.rest.activate = true
  defconf.rest.port = defaultport
  defconf.rest.authentication = {}
  defconf.rest.authentication.realm = "localhost"
  defconf.rest.authentication.ha1 = "434748cecc3f8c0b0d5a2d2e9c82be5d"
  defconf.rest.restricted_uri = {}
  defconf.rest.restricted_uri["*"] = true
  
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
  
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/crypto")
  -- backup defaultconfig file
  os.execute("cp "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup")
  
  
end

function t:teardown()
  os.execute("mv "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua.backup "..targetManager.targetdir.."/runtime/lua/agent/defaultconfig.lua")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/persist")
  os.execute("rm -rf "..targetManager.targetdir.."/runtime/crypto")
  
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
  local status, body = readServer(url)
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is not a leaf
  u.assert_equal(0, status)
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
    
  local configtab = yajl.to_value(body)[2]
  
  -- compare reference to current content
  table.sort(confref)
  table.sort(configtab)
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
  local status, body = readServer(url)
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is not a leaf
  u.assert_equal(0, status)
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  
  local configtab = yajl.to_value(body)[2]
  
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")

    
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_equal(0, status)
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  
  local configtab = yajl.to_value(body)[1]
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_equal(0, status)
  u.assert_not_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  
  local configtab = yajl.to_value(body)[1]
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_equal(0, status)
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  
  
  local configtab = yajl.to_value(body)[2]
  
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, err = f("notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(err)
--  u.assert_not_nil(body, "HTTP request got an empty response")
  u.assert_equal(22, status)
  
--  u.assert_equal("handler not found", body)
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, err = f("nohandler.notexisting")
  
  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_not_nil(err)
  --u.assert_not_nil(body, "HTTP request got an empty response")
  u.assert_equal(22, status)
  
  --u.assert_equal("handler not found", body)
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
  local status, body = readServer(url)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.1234")

  rpcclient:call('os.exit', 0)
  
  -- config node is a leaf
  u.assert_equal(0, status)
  u.assert_nil(leaf)
  u.assert_not_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  
  local configtab = yajl.to_value(body)[2]

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
  local status, body = sendToServer(url, "PUT", httpBodyJSON)
  
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
  u.assert_equal(0, status, body)
  
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

  local status, body = sendToServer(url, "PUT", httpBodyJSON)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.server.url")

  rpcclient:call('os.exit', 0)

  u.assert_equal(0, status, body)
  
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
  local status, body = sendToServer(url, "PUT", httpBodyJSON)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.1234.my-leaf")

  rpcclient:call('os.exit', 0)

  u.assert_equal(0, status, body)
  
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
  local status, body = sendToServer(url, "PUT", httpBodyJSON)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local rootleaf, rootnodes = f("config.1234")
  local myleaf, myleafnodes = f("config.1234.my-leaf")
  local mynodeleaves, finalnodes = f("config.1234.my_node")

  rpcclient:call('os.exit', 0)

  -- compare reference to current content
  u.assert_equal(0, status, body)

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
  

  -- Request
  local url = baseurl.."devicetree/config/server/url"
  local status, body = sendToServer(url, "PUT", "")
  
  rpcclient:call('os.exit', 0)

  -- Expect an error from server
  u.assert_equal(0, status, body)
  u.assert_not_nil(body, "HTTP request got an empty response")
  u.assert_equal("Invalid JSON input: ", body)
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
  local status, body = sendToServer(url, "POST", httpBodyJSON)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.myserver")

  rpcclient:call('os.exit', 0)
  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  u.assert_equal(0, status, body)
  
  u.assert_equal("HTTP/POST is not supported for devicetree/config/myserver", body)
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
  local status, body = sendToServer(url, "UPDATE", httpBodyJSON)
  
  
  -- Get reference directly from device tree
  local f = rpcclient:newexec(getDT)
  local leaf, confref = f("config.myserver")

  rpcclient:call('os.exit', 0)

  -- config node is a leaf
  u.assert_nil(leaf)
  u.assert_nil(confref)
  u.assert_not_nil(body, "HTTP request got an empty response")
  u.assert_equal(0, status, body)
  
  u.assert_equal("HTTP/UPDATE is not supported for devicetree/config/myserver", body)
  end


return {init = init}