-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--- Agent port to Linux : porting and wrappers functions


local M = { }

--------------------------------------------------------------------------------
-- platform.init must return non false/nil value for success
-- or nil+error message
function M.init()
    return 'ok'
end

function M.getupdateplatformcomponent()
        return {
            Agent    = _MIHINI_AGENT_RELEASE,
        }
    end

function M.getdeviceid()
    return nil
end


function M.init() return 'ok' end

return M

