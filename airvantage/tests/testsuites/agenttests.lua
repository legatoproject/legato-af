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

local system = require 'utils.system'


function loadinfwk(file)
  table.insert(loadedfiles, require(file))
end

function startAgentTests()
  print "starting agent tests"
  p(loadedfiles)
  u.run()
end

function AgentgettestsResults()
  return u.getStats()
end

function closeLua()
  os.exit(0)
end

