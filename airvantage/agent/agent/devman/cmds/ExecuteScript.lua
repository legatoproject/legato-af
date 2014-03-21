-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local ltn12 = require "ltn12"
local http = require "socket.http"
local md5 = require "crypto.md5"
local config = require "agent.config"
local loader = require "utils.loader"

local function ExecuteScript(sys_asset, args)
    local url = args and (args.url or args[1])
    local signature = args and (args.signature or args[2])
    assert(url and signature, "wrong param")

    local script={}
    local b, c, h, s
    local function httpgetscript ()
        b, c, h, s = http.request{
            url = url,
            sink = ltn12.sink.table(script),
            method = "GET",
            step = ltn12.pump.step,
            proxy = config.server.proxy
        }
        return b, c
    end
    local wnet = agent.netman.withnetwork
    if wnet then wnet(httpgetscript) else httpgetscript() end
    assert(b and type(c)=='number' and c>=200 and c<300, string.format("error while doing http request, error: %s", b and s or tostring(c)))

    local h = md5()
    for k,v in ipairs(script) do h:update(v) end
    local hex_checksum = h:digest(false)
    signature = string.lower(signature)
    assert(hex_checksum == signature, string.format("corrupted script content, received %s, computed %s", signature, hex_checksum))

    local s, err = loader.loadbuffer(script, "@"..url, true)
    assert(s, "loading err="..(err or "unknown"))
    s()
    return "ok"
end

return ExecuteScript
