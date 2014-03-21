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
require "print"

-- Arguments extraction
local strdate = os.date("%Y%m%d-%H%M%S")
local dirname = arg[1] or ("~/tests"..strdate)
local filter = arg[2] or "policy=OnCommit;target=native"
local testconfigfile = arg[3] or "defaulttestconfig"
local targetconfigfile = arg[4] or "defaulttargetconfig"


local function startup(dirname, filter, testconfigfile, targetconfigfile)

  if not arg[2] then
    print "No filter defined, running OnCommit tests as default"
  end

  -- display values that will be used in the program
  print("Using following parameters:")
  print("   - Working Directory: "..dirname)
  print("   - Filter: "..filter)
  print("   - Tests Config File: "..testconfigfile)
  print("   - Target Config File: "..targetconfigfile)

  local lfs = require 'lfs'

  local testmanager = require 'tester.testManager'
  local m = testmanager.new(dirname, filter, testconfigfile, targetconfigfile, lfs.currentdir())
  local retcode = m:run()

  os.exit(retcode)
end

require 'sched'

sched.listen(4000)
sched.run(function() startup(dirname, filter, testconfigfile, targetconfigfile) end) --function() require "tester.init" end)
sched.loop() -- main loop: run the scheduler