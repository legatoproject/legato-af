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

local M = {}
local runtime_dir = nil
local running = true

local function webserver()
   local fd = io.popen("env - " .. _G.wrapper_env .. " " .. runtime_dir .. "/bin/lua -l platform")
   while running do
      print(fd:read("*l"))
      sched.signal("web_server", "running")
      sched.wait(0.1)
   end
end

function M.setup(dir)
   runtime_dir = dir
   sched.run(webserver)
   sched.wait("web_server", "running")
end

function M.teardown()
   running = false
   local status, output = require "utils.system".pexec("ps fux | grep '\_ " .. runtime_dir .."/bin/lua -l platform' | head -1 | cut -d ' ' -f3")
   os.execute("kill -9 " .. tostring(output))
end

return M
