-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- This file implements all function used by the template of target manager for a linux 32 bits Ready Agent

local os = require 'os'
local sched = require 'sched'
local rpc = require 'rpc'
local os = require 'os'
local system = require 'utils.system'


local M = {}
local rpcclient = nil


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
  assertconfig(config)

  -- Compile the Agent using the toolchain for QEmu x68 machine
  --print(svndir.."/bin/build.sh -t ".. config.compil.toolchain.." -C " ..targetdir)
  local result = os.execute(svndir.."/bin/build.sh -t ".. config.compil.toolchain.." -C " ..targetdir)
  if result ~= 0 then error("Compilation error for module: "..config.ConfigModule) end

  local result = os.execute("cp "..svndir.."/tests/defaultconfig.lua "..targetdir.."/runtime/lua/agent")
  if result ~= 0 then error("Can't copy defaultconfig file for"..config.ConfigModule) end

  return result
end

-- Function: install
-- Description: install a fresh Agent on the DuT
-- Return: "success" if the RA is correctly installed in the VM. nil, error otherwise
function install(config, svndir, targetdir)
  assert(targetdir)

  -- clear the destination if exists
  local fd, err = os.execute("rm -rf "..targetdir.."/vm")

  -- copy the VM to the target directory (the whole VM directory)
  local fd, err = os.execute("cp -R /opt/".. config.compil.toolchain .. "/vm " .. targetdir)

  -- set execution flag on the scripts
  local fd, err = os.execute("cd ".. targetdir.."/vm && chmod +x *.sh")

  --scp -i ./id_dsa -r -p -P 2222 ../runtime embedded@localhost:/home/embedded/runtime

  -- configure the VM with the newly compiled RA
  local fd, err = os.execute(targetdir.."/vm/installRA.sh "..targetdir.."/runtime")

  if err then error("Error while installing RA in VM: "..err) end
  return fd, err
end


-- Function: start
-- Description: start the Agent software/DuT
function start(config, svndir, targetdir)
  assertconfig(config)
  --local fd, err = os.execute("cd ".. config.path .."/runtime && bin/agent")
  local fd, err = os.execute("expect -c 'spawn ssh -o StrictHostKeyChecking=no -p 2222 embedded@localhost ; expect assword ; send \"\\n\"; expect $; send \"runtime/start.sh &\\n\"; expect $; send \"exit\\n\"'")

  if err then error("starting lua error: "..err) end
  return fd, err
end


-- Function: startlua
-- Description: start the lua framework program
function startlua(config, luafile)
  local fd, err
  if luafile then
    fd, err = system.popen("cd ".. config.path .."/runtime && bin/lua ".. luafile)
  else
    fd, err = system.popen("cd ".. config.path .."/runtime && bin/lua")
  end

  if err then error("starting lua error: "..err) end
  return fd, err
end

-- Function: reboot
-- Description: reboot the DuT
function reboot(rpcclient)
  rpcclient:call('os.reboot')
  sched.wait(8)
end

-- Function: stop
-- Description: stop the DuT
function stop(config, svndir, targetdir)
  print ("stopping VM")
  local fd, err = os.execute(targetdir.."/vm/stop.sh")
  --rpcclient:call('system.popen', "su -c poweroff")
  if err then error("stopping DuT error: "..err) end

  sched.wait(8)
  return fd, err
end

M.stop=stop; M.reboot=reboot; M.startlua=startlua; M.start=start; M.compile=compile; M.install=install;

return M