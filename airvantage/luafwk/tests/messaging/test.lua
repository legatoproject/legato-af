-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require 'sched'
local at = require 'at'
local sms = require 'messaging'
require 'print'
require 'shell'


local function test()

    at.init("/dev/ttyS0")
    sms.init()
    print("Started")

end

sched.run(shell.telnet_server, 2000)


sched.run(test)

sched.loop()
