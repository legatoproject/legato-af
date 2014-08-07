--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
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
  stagedb     = { target = {"native"}, environment = {"agent", "luafwk"}, TestPolicy = "OnCommit"},


  --agent tests
  --
  treemgr     = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  config      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  update      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  monitoring  = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, -- Disabled
  --time        = { target = {"native"}, environment = "agent", TestPolicy = "OnCommit"}, --needs root user rights
  --appcon      = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"}, --needs root user rights
  extvars     = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  data_policy = { target = {"native"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  raconwrapper= { target = {"native"}, environment = {"integration"}, TestPolicy = "OnCommit"},
  
  
  -- integration tests
  security    = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  rest        = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  restdigest  = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  srvcon      = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  updatedwl   = { target = {"native"}, environment = {"integration"}, TestPolicy = "Daily"},
  updateDaily   = { target = {"native"}, environment = {"agent"}, TestPolicy = "Daily"},

  -- end to end tests
  --Legacy simulated target tests
  --
  racoon_serialization = { target = {"linux_x86", "linux_amd64"}, environment = {"agent"}, TestPolicy = "OnCommit"},
  racoon_endtoend = { target = {"linux_x86", "linux_amd64", "BoxPro"}, environment = {"agent"}, TestPolicy = "Daily"}
}

return tests;
