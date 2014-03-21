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

local param = ...
local sched = require("sched")

if not param then error("install script param is nil") end

--this signal will trigger test success
sched.signal("update_test_02_installscript", "ok")

--else success