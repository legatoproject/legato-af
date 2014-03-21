------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------


local M = {}

--------------------------------------------------------------------------------
--- Performs popen, returns status and command outputs.
-- Command outputs are retrieved using read("*a").
-- @param cmd string containing the command to execute.
-- @return execution status as a number, output buffer as a string.
-- @return nil followed by an error message otherwise.
--------------------------------------------------------------------------------

function M.pexec(cmd)
    local fd = io.popen(cmd)
    if not fd then return nil, err end
    local output = fd:read("*a")
    local status = fd:close()
    return status, output
end

return M
