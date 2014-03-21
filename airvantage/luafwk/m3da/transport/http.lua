-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

-- `M.new(url)` returns a transport instance, which honors the M3DA transport API:
--
-- * `:send(src)` sends data bytes, given as an LTN12 source, over the network;
--   returns true upon success, nil+errmsg upon failure
--
-- * Data bytes coming from the network are pushed into `self.sink`,
--   which must have been set externally (normally by an `m3da.session` instance).


local log   = require "log"
local http  = require "socket.http"
local ltn12 = require "ltn12"

local M  = { }
local MT = { __index=M, __type='m3da.transport' }

function M.new(url)
    checks('string')
    local self = { url=url, sink=false, proxy=nil }
    return setmetatable(self, MT)
end

function M :send (src)
    checks('m3da.transport', '!')
    log("M3DA-TRANSPORT", "DETAIL", "http request %q...", self.url)
    local headers = { }
    headers["Content-Type"] = "application/octet-stream"
    headers["Transfer-Encoding"] = "chunked"

    local body, code, headers, statusline = http.request {
      url     = self.url,
      sink    = assert(self.sink, 'Unconfigured session sink in transport module'),
      method  = "POST",
      headers = headers,
      source  = src,
      proxy   = self.proxy,
    }

    log("M3DA-TRANSPORT", "DETAIL", "http response status: %s", tostring(code))

    if not body then return nil, code
    elseif (type(code) == "number" and (code < 200 or code >= 300)) then
        return nil, code
    else
        return code
    end
end

return M