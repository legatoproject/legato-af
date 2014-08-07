-------------------------------------------------------------------------------
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
--     Romain Perier for Sierra Wireless - initial API and implementation
--     Guillaume Terrissol for Sierra Wireless - function mktree
-------------------------------------------------------------------------------

local lfs = require 'lfs'

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

--------------------------------------------------------------------------------
--- Creates a folder, checking the path all the way, creating intermediate
--- folders if required.
-- @param path to build (absolute, or relative to current directory).
-- @retval true on success,
-- @retval nil, followed by an error message otherwise.
--------------------------------------------------------------------------------

function M.mktree(path)

    local checkingPath = ""                 -- Is path relative (by default),
    if string.sub(path, 1, 1) == "/" then
        checkingPath = "/"                  -- or absolute ?
    end

    for dir in string.gfind(path, "[^/]+") do

        checkingPath = checkingPath..dir    -- Do not add a trailing slash yet, because dir could be a file (this could confuse lfs.attributes).
        local res, err = lfs.attributes(checkingPath, "mode")

        if res then 
            if res ~= "directory" then      -- "directory" is defined into lfs, function mode2string().
                return nil, checkingPath.." isn't a directory"
            end
        else                                -- Couldn't retrieve info, because checkingPath doesn't exist yet : makes it.
            res, err = lfs.mkdir(checkingPath)
            if not res then
                return nil, err
            end
        end

        checkingPath = checkingPath..'/'    -- checkingPath is a directory, can add '/'.

    end

    return true

end

return M
