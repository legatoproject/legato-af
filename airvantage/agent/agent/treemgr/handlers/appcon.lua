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

local upath  = require 'utils.path'
local utable = require 'utils.table'
local appcon = require 'agent.appcon'

-- Access to application attributes are cached, to avoid too many daemon requests.
local cache_longevity = 3 -- How many seconds before atributes are considered stall
local cache_name = nil    -- name of the app whose attributes are cached
local cache_date = 0      -- date when the cache was last updated
local cache_content = nil -- actual cache content

-- Retrieve an application's attributes, either from the appcon daemon,
-- or from the cache if it's fresh enough.
-- Also adds a `"Started"` Boolean entry, which will be writable through `:set()`.
--
-- @return the attributes table or `nil`.
local function get_attrs(appname)
    local now = os.time()
    if appname ~= cache_name or now-cache_date>cache_longevity then
        cache_content = appcon.attributes(appname)
        if not cache_content then cache_name=nil; return nil end
        cache_content.started=cache_content.status=='STARTED'
        cache_name = appname
        cache_date = now
    end
    return cache_content
end

local M = { }

function M :get(hpath)
    local root, appname, method = unpack(upath.segments(hpath))
    if not root then
        return nil, { list=true, apps=true }
    elseif root == 'list' then
        if appname then return nil, "invalid path" end
        return table.concat(utable.keys(appcon.list()), ' ')
    elseif root == 'apps' then
        if not appname then -- list all applications in a table's keys
            return nil, appcon.list()
        end
        local attrs = get_attrs(appname)
        if method==nil then -- list all methods for this app, in a table's keys
            return nil, get_attrs(appname)
        end
        local attr = attrs[method]
        if attr~=nil then return attr else return nil, "no such attribute" end
    else -- starts neither with 'list' nor 'apps'
        return nil, "invalid path"
    end
end

function M :set(hmap)
    for hpath, val in pairs(hmap) do
        local apps, appname, method = unpack (upath.segments(hpath))
        if apps~='apps' then return nil, 'invalid path'
        elseif method=='started' then
            if val then return appcon.start(appname)
            else return appcon.stop(appname) end
        elseif method=='autostart' then
            return appcon.configure(appname, val)
        else return nil, 'Cannot write this attribute' end
    end
end

return M
