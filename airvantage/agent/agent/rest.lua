-------------------------------------------------------------------------------
-- Copyright (c) 2013 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local rest = require 'web.rest'
local config = require 'agent.config'
local M = {}

function M.init()
    return rest.init(config)
end

return M
