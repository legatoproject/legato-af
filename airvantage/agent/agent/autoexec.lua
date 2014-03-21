-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local M = { }

M.defaultfilename = '/tmp/autoexec.lua'

function M.init()
    local filename = require 'agent.config' .get 'autoexec.filename'
        or M.defaultfilename
    local f = io.open(filename, 'r')
    if f then
        log('AUTOEXEC', 'WARNING', "Executing file %s", filename)
        local src = f :read '*a'
        f :close()
        assert(loadstring(src))()
    else
        log('AUTOEXEC', 'INFO', "No file %s found", filename)
    end
    return 'ok'
end

return M