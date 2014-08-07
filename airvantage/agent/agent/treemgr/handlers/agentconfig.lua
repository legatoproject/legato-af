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

--- Agent config handler: present the persisted agent configuration,
--  managed by module `agent.config`, as a treemgr handler.
--
--  This handler is not based on `agent.treemgr.handlers.table` because
--  it handles notification.

local hname   = ...
local config  = require 'agent.config'
local path    = require 'utils.path'
local treemgr = require 'agent.treemgr'
-- local M = require 'agent.treemgr.handlers.table' (require 'agent.config')

local M = {
    registered = { }
}

--- Sort config hpaths between leaf and non-leaf nodes.
--  TODO: replace with handler.table?
function M :get (hpath)
    local val = config.get(hpath)
    --printf("AC.get %s -> %s", hpath, sprint(val))
    if type(val)=='table' then
        local children_set = { }
        for child, _ in pairs(val) do
            children_set[child] = true
        end
        return nil, children_set
    else return val end
end

function M :set (hmap)        return config.set('', hmap) end
function M :register(hpath)   M.registered[hpath] = true end
function M :unregister(hpath) M.registered[hpath] = nil end

return M