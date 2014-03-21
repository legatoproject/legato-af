-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local core = require "agent.platform.core"

local M = {}
local logFunction = { ["DEBUG"] = core.LE_DEBUG, ["DETAIL"] = core.LE_DEBUG, ["INFO"] = core.LE_INFO, ["WARNING"]  = core.LE_WARN, ["ERROR"] = core.LE_ERROR }


-- The displaylogger is defined here because it must be set before the first message is logged
-- otherwise few messages might come to stdout and other ones to the legato framework
log.displaylogger = function(module, severity, message)
    checks("string", "string", "string")
    logFunction[severity](module, message)
end

function M.init()
    return "ok"
end

return M
