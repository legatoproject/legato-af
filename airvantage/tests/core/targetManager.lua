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

local print   = print
local table   = table
local require = require
local setmetatable = setmetatable
local p = p
local sched = require 'sched'
local os = require 'os'
local rpc   = require 'rpc'
local assert = assert
local error = error

module(...)
targetmanagerapi = {}

-- Function: new
-- Description: create a new instance for the specified target
-- Return: the newly created instance if valid, nil otherwise
function new(name, specificfile, config, svndir, targetdir)
  if not specificfile then return nil end
  if not config then return nil end
  p(config)
  local instance = setmetatable({name = name, target = require(specificfile), config = config.config, testslist = config.tests, results = {} ,svndir = svndir, targetdir = targetdir, rpc=nil }, {__index=targetmanagerapi})
  return instance
end

-- Function: compile
-- Description: compile the Agent and the tests for the target
-- Return: the status of the compilation process
function targetmanagerapi:compile()
  local tar = self.target
  return tar.compile(self.config, self.svndir, self.targetdir)
end

-- Function: install
-- Description: install the Agent on the target
-- Return: the status of installation, nil and error if any error occurs
function targetmanagerapi:install()
  local tar = self.target
  return tar.install(self.config, self.svndir, self.targetdir)
end

-- Function: start
-- Description: start the Agent on the target
-- Return: the status of the run, ie the number of errors that occured during the run
function targetmanagerapi:start()
  self.target.start(self.config, self.svndir, self.targetdir)
end

function targetmanagerapi:stop()
  self.target.stop(self.config, self.svndir, self.targetdir, self:getrpc())
end

function targetmanagerapi:reboot()
  self.target.reboot(self:getrpc())
end

function targetmanagerapi:startlua()
  self.target.startlua(self.config, self.targetdir, "./lua/tests/luafwktests.lua")
end

local function createrpc(self, islua)
  local l_rpc = nil
  if islua then
    -- create rpc connection with the target
    print("Creating a RPC connection with "..self.config.Host..":7300")
    l_rpc = rpc.newclient(self.config.Host, 7300)
  else
    -- create rpc connection with the target
    print("Creating a RPC connection with "..self.config.Host..":"..self.config.RPCPort)
    l_rpc = rpc.newclient(self.config.Host, self.config.RPCPort)
  end

  return l_rpc
end

function targetmanagerapi:getrpc(islua)
  if not self.rpc then
    assert(self.config.Host)

    self.rpc = createrpc(self, islua)

  else
    -- Check RPC connection validity
    local f = self.rpc:call("require", "sched")

    -- If RPC is closed then create a new one.
    if not f then
      self.rpc = nil
      print("RPC reconnection")
      local cpt=0
      while cpt < 3 and not self.rpc do
        self.rpc = createrpc(self, islua)
        cpt = cpt + 1

        -- Wait 3 seconds between each try
        if not self.rpc then sched.wait(3) end
      end
    end
  end

  if not self.rpc then error("Can't connect to host: ".. self.config.Host) end

  return self.rpc
end
