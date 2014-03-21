-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local logstore = require "log.store"
local config = require "agent.config"
local os = require "os"
local ftp = require "socket.ftp"
local skturl = require "socket.url"

local function LogUpload(sys_asset, args)
    local url = args and (args.url or args[1])
    local logtype = args and (args.logtype or args[2])
    assert(url and (logtype == "ram" or logtype == "flash"), "COMMANDS.LogUpload Invalid params")
    assert(config.log.policy, "Log policies not activated")
    local type = string.match(url, "(%a*)://")
    local src = logtype == "flash" and  logstore.logflashgetsource() or ltn12.source.string( logstore.logramget())
    local res, err
    if type == "http" then
        return -1, "HTTP Post method is not supported yet to upload logs !"
    elseif type == "ftp" then
        local logfilename = string.format("Agent_%s_%s_%d.log", config.agent.deviceId, logtype, os.date("%s") )
        local parsed_url = skturl.parse(url)
        res, err = ftp.put{
            host = parsed_url.host,
            user = config.log.policy.ftpuser,
            password = config.log.policy.ftppwd,
            command = "stor",
            argument =  logfilename,
            source = src
        }
        assert(res, "Error during ftp request: "..(err or "unknown") )
        log ('LOGUPLOAD', 'INFO', 'Successfully uploaded log %s to %s',logfilename, url)
        return "ok"
    else  return nil, "Unsupported url type to upload logs" end
end

return LogUpload
