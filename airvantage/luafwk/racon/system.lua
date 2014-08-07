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
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

---
-- System related tasks
--
-- @module system
--

local common = require 'racon.common'
local checks = require 'checks'

local M = { initialized=false }

--------------------------------------------------------------------------------
-- Initialize the module. No service provided by this module will work unless
-- this initializer function has been called first.
--
-- @function [parent=#system] init
-- @return a true value upon success
-- @return `nil` followed by an error message otherwise.
--
function M.init()
    if M.initialized then return "already initialized" end
    local a, b = common.init()
    if not a then return a, b end
    M.initialized=true
    return M
end

--------------------------------------------------------------------------------
-- Requests a reboot of the system, with an optional reason passed as a string,
-- which will be logged.
-- @function [parent=#system] reboot
-- @param reason the (logged) reason why the reboot is requested
-- @return non-nil if the request has been received.
-- @return `nil` followed by an error message otherwise.
--
function M.reboot(reason)
    checks('?string')
    if not M.initialized then error "Module not initialized" end
    return common.sendcmd("Reboot", reason or "")
end


return M