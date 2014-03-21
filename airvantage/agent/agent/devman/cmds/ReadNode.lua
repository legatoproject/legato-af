-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local dev      = require 'agent.devman'
local tm       = require 'agent.treemgr'
local srvcon   = require 'agent.srvcon'
local racon    = require 'racon'
local upath    = require 'utils.path'
local niltoken = require 'niltoken'


local POLICY = 'now'

-- Send data recursively. The point of not reifying the table with
-- `agent.treemgr.table` is that the whole node might not fit in RAM.
-- However, it tries to send everything with the same path in a single
-- record, to limit sdb table creations.
-- @param path the devtree path to read, possibly recursively
-- @param error_messages list of error messages: any error msg will be inserted
--   at the end of this list.
local function recsend(path, error_messages)
    local value, children = tm.get(path)
    if not children then
        assert(dev.asset :pushdata (path, niltoken(value), POLICY))
    elseif type(children)=='table' then
        local record = { }
        for _, child_path in ipairs(children) do
            local value, children = tm.get(child_path)
            if not children then
                local _, leaf=upath.split(child_path, -1)
                record[leaf]=value -- group same-path items together
            else recsend(child_path, error_messages) end
        end
        if next(record) then
            assert(dev.asset :pushdata (path, record, POLICY))
        end
    else
        local msg = string.format("error for path [%s]:%s",
            tostring(path), tostring(children))
        table.insert(error_messages, msg)
    end
end

local function ReadNode(sys_asset, args, fullpath, ticketid)
    checks('racon.asset', 'table', 'string', '?number')
    local error_messages = { }
    local err_msg=""
    -- ReadNode parameters are put in a list, every parameter is a path to read
    -- No point in using parameter indexes/map keys
    for _, path in pairs(args) do
        if type(path)~='string' then return nil, 'ReadNode command expects string(s) parameter(s)' end
        recsend(path, error_messages)
    end
    if next(error_messages) then
       -- Concatenate all error messages into a single one.
       err_msg = table.concat(error_messages, ", ")
       return nil, err_msg
    end
    return "ok"
end

return ReadNode