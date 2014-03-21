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

require 'strict'
local sched = require 'sched'
local u = require 'unittest'
local os = require 'os'
local rpc   = require 'rpc'
local loadedfiles = {}
local table = table

sched.listen(4001)
--local system = require 'tests.tools.systemtest'
local system = require 'utils.system'


function loadinfwk(file)
  table.insert(loadedfiles, require(file))
end

function startLuaFwkTests()
  p(system.pexec("pwd"))
  print "starting lua framework tests"
  u.run()
end

function LuaFWKgettestsResults()
  return u.getStats()
end

function closeLua()
  os.exit(0)
end

sched.run(system.pexec, 'lua lua/tests/tools/sockettestsvr.lua')
sched.run(function() rpc.newmultiserver('localhost', 7300) end) -- start the Agent
sched.loop() -- main loop: run the scheduler

