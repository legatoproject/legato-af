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

-- this is not a test for system function, but a specific version of system API used in tests...
-- this version used non-blocking system command not very reliable on targets...

local sched         = require 'sched'
local systemutils   = require 'utils.system'

local M={}

M.pexec = systemutils.pexec
M.execute = os.execute
M.popen = io.popen

return M
