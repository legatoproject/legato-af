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

local config = require"agent.config"
-- if update is not activated
if not config.get("update.activate") then
    return require 'agent.treemgr.handlers.constant' 'update not activated'
end

local pathutils  = require 'utils.path'
local ucommon  = require 'agent.update.common'

local M = { }

-- handler api for childs listing is: The children must be in the table's keys e.g. {{ { x=true, y=true } }}
-- The path toward the children must be relative to the hpath argument.
function M :get(hpath)
    local root, subtree = pathutils.split(hpath, 1)
    --note that ucommon.data is proxy table for cache implementation, it is not possible to index it content directly
    --subtable are, however, regular table.
    if not root or root == "" then --list update root path childs: only 2 subtables are accessible
        return nil, { swlist=true, currentupdate=(ucommon.data.currentupdate ~= nil or nil)}
    else
        if not ucommon.data[root] then return nil, "BAD_PARAMETER" -- ckeck sub path is valid
        else
            local data =  pathutils.get(ucommon.data[root], subtree)
            if not data or type(data) ~="table" then return data --leaf value, return i
            else --node reading, return list of sub vars.
                local res = {}
                for k,v in pairs(data) do
                    res[k]=true
                end
                return nil, res
            end
        end
    end
end

--set not allowed on update subtree
function M :set(hmap)
    return nil, "NOT_PERMITTED"
end

return M
