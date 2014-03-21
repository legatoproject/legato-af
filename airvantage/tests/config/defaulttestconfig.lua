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

local tests = {
  --luafwk tests
  --
  bysant      = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  luatobin    = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  posixsignal = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  rpc         = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  sched       = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  --socket      = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},
  --logstore    = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- deprecated module for now
  persist     = { target = {"native"}, environment = {"agent" , "luafwk"}, TestPolicy = "OnCommit"},
  timer       = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},


  --luafwk racon tests
  --
  --system      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},  -- Pas au bon endroit
  --devicetree  = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},  -- Pas au bon endroit
  --sms         = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, need stub? + activate SMS in test config ?
  --racon       = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- "ref to hessian: deprecated, update by using bysant/m3da"
  --airvantage_perf = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- not available in test build for now
  --asset_tree  = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, --asset_tree.lua:17 module 'hessian.awtda' not found
  --emp         = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- not available in test build for now


  --agent tests
  --
  treemgr     = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  config      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  update      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  stagedb     = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  monitoring  = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- Disabled
  --time        = { target = {"native"}, environment = "agent", TestPolicy = "OnCommit"}, --needs root user rights
  --appcon      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, --needs root user rights
  extvars     = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  
  -- integration tests
  crypto      = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  rest        = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  restdigest  = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  srvcon      = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  updatedwl   = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},

  -- end to end tests
  --Legacy simulated target tests
  --
  racoon_serialization = { target = {"linux_x86", "linux_amd64"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  racoon_endtoend = { target = {"linux_x86", "linux_amd64", "BoxPro"}, environment = {"agent"}, TestPolicy = "Daily"}
}

return tests;
