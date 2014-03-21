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

local config = require 'agent.config'
if not config.get('appcon.activate') then error "Application container disabled in agent.config" end

local utable = require 'utils.table'

return function(asset, args, fullpath, ticket)
    local appcon = require 'agent.appcon'
    for subcommand, subargs in utable.sortedpairs(args) do
        log('APPCON-CMD', 'INFO', "Parsing Portal command appcon.%s with args %s", subcommand, sprint(subargs))
        local appname = subargs.appname or subargs[1] or error("bad app name")
        if subcommand=='start' then
            assert(appcon.start(appname))
        elseif subcommand=='stop' then
            assert(appcon.stop(appname))
        elseif subcommand=='autostart' then
            local autostart = subargs.autostart -- don't use and/or idiom, autostart might be 'false'
            if autostart==nil then autostart = subargs[2] end
            if type(autostart)~='boolean' then
                error("bad autostart parameter "..sprint(autostart))
            end
            assert(appcon.configure(appname, autostart))
        else
            error("unknown subcommand appcon."..tostring(subcommand))
        end
    end
    return "ok"
end