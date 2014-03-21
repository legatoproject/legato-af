-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------
local testname="update_test_02_installscript"
local param = ...
local sched = require("sched")

--check param of component
if not param or param.name~="update.test1" or param.version~="1" or not param.parameters.autostart
or param.parameters.bar ~=42 or param.parameters.foo~='test' then
    log(testname, "ERROR", "%s : param in package are not valid or nil", testname)
else
    --this signal will trigger test success
    sched.signal(testname, "ok")
end
