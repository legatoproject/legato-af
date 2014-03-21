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

local T = { }

function T.date() return os.date() end
function T.timestamp() return os.time() end


function M :get (path)
    local f = T[path]
    if f then return f() else return nil, T end
end

-- Not supported
function M :set() return nil, "cannot write time" end

-- Not supported
function M :register() end

return M
