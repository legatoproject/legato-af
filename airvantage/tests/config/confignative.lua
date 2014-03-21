-- This file implements all function used by the template of target manager for a Native Ready Agent

local os = require 'os'
local sched = require 'sched'
local rpc = require 'rpc'
local system = require 'utils.system'
--local systemtool = require 'utils.system'
local error = error
local print = print

local M = {}


local function assert(val, msg)
  if not val then
    error(msg)
  end
end

-- Function: assertconfig
-- Description: assert that the provided parameter is a valid config structure
local function assertconfig(config)
  assert(config, "Config: table expected")
  assert(config.path, "Config path is empty")
end

-- Function: compile
-- Description: use the compile toolchain to compile the target test
function compile(config, svndir, targetdir)
  print("\tCompiling")
  assertconfig(config)

  -- Compile the Agent using the default toolchain
  --local result = os.execute(svndir.."/bin/build.sh -n -C " ..targetdir)
  local result = os.execute(svndir.."/bin/build.sh -n -C " ..targetdir)
  if result ~= 0 then error("Compilation error for module: "..config.ConfigModule) end

  --result = os.execute("cd " .. targetdir.. " && make all embeddedtestsluafwk luafwktests readyagenttests lua embeddedteststools modbusserializertests readyagentconnectortests")
  -- Compiling Ready Agent
  result = os.execute("cd " .. targetdir.. " && make all lua agent_provisioning")
  if result ~= 0 then error("Make Agent error for module: "..config.ConfigModule) end

  -- Remove defaultconfig
  result = os.execute("rm -f " .. targetdir.. "/runtime/lua/agent/defaultconfig.lua")
  if result ~= 0 then error("Make Tests error for module: "..config.ConfigModule) end

  -- Compiling tests
  result = os.execute("cd " .. targetdir.. " && make testsauto test_luafwk test_agent test_racon test_fwkemb agent_treemgr_ramstore")
  if result ~= 0 then error("Make Tests error for module: "..config.ConfigModule) end

  result = os.execute("cp " .. targetdir.. "/runtime/lua/agent/defaultconfig.lua "  .. targetdir.. "/runtime/lua/agent/defaultconfigtemplate.lua")
  --result = os.execute("cp "..svndir.."/tests/defaultconfig.lua "..targetdir.."/runtime/lua/agent")
  --if result ~= 0 then error("Can't copy defaultconfig file for"..config.ConfigModule) end

  print("\tNative build complete")
  return result
end

-- Function: install
-- Description: install a fresh firmware+Agent on the DuT
-- Return: "success" if the RA is correctly installed in the VM. nil, error otherwise
function install(config, svndir, targetdir)
   assert(targetdir)

   -- delete all stored objects for a clean install
   local result = os.execute("rm -rf "..targetdir .. "/runtime/persist/ && rm -rf "..targetdir .. "/runtime/crypto/")
   if result ~= 0 then error("Installation error for module: "..config.ConfigModule) end

end


-- Function: start
-- Description: start the Agent software/DuT
function start(config, svndir, targetdir)
  assertconfig(config)
  assert(targetdir)
  sched.run(function() os.execute("cd ".. targetdir .."/runtime && bin/agent") end)

  print("\tAgent Started")
  wait(5)
end


-- Function: startlua
-- Description: start the lua framework program
function startlua(config, targetdir, luafile)
   assertconfig(config)
   if luafile then
     sched.run(function() os.execute("cd ".. targetdir .."/runtime && bin/lua ".. luafile) end)
   else
     sched.run(function() os.execute("cd ".. targetdir .."/runtime && bin/lua ") end)
   end

   print("lua vm started")

   sched.wait(3)
end

-- Function: reboot
-- Description: reboot the DuT
function reboot(rpcclient)
  assert(rpcclient)

  rpcclient:call('os.reboot')
  sched.wait(8)
end

-- Function: stop
-- Description: stop the DuT
function stop(config, svndir, targetdir, rpcclient)
  print ("Stopping target")
  assert(rpcclient)
  local res, err = rpcclient:call('os.exit')

  sched.wait(3)
  if not res then return "Success" else return nil, "Stop failed" end
end

M.stop=stop; M.reboot=reboot; M.startlua=startlua; M.start=start; M.compile=compile; M.install=install;

return M