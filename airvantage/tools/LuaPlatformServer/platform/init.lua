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

require 'strict'

-- persist.qdbm cannot support more than onw
WORKING_DIR='/tmp/'

local config = {
    --url='httpserver://*/device/com',
    url='tcpserver://*:44900',
    --authentication = 'hmac-sha1',
    localid = 'fakesrv'
}

local options = {
    p = 10000,
    h = 'localhost',
    k = 'luaidekey'
}



if os.getenv 'LUADEBUG' then options.d = true end

if options.d~=nil then require 'debugger' end -- must be loaded before sched

require 'sched'
require 'checks'
require 'socket.url'
local m3da = require 'm3da.bysant'

local m3da_deserialize = m3da.deserializer()

platform = { }

require 'platform.backend'
require 'platform.pages'
require 'platform.favicon'
local telnet = require 'shell.telnet'

WEB_PORT = 8070
TELNET_PORT = 2323

local function start_web_server(port)
    checks('number')
    web.start(port)
end

local function start_telnet_server(port)
    checks('number')
    telnet.init{
        address     = '0.0.0.0',
        port        = port,
        editmode    = "edit",
        historysize = 100 }
end

local function color_logs()
    require 'log'
    local f =
        {ERROR="\027[31;1m", WARNING="\027[33;1m", DEBUG="\027[37;1m", INFO="\027[32;2m", DETAIL="\027[36;2m"}
    function log.displaylogger(module, severity, log)
        local f = f[severity] or "\027[0m"
        if f then print(f..log.."\027[0m") else print(log) end
    end
end

local function from_device(message)
    table.insert(platform.history, { os.date(), false, message })
end

local function main()
    color_logs()
    log.setlevel("ALL")
    log.setlevel("INFO", "SCHED", "FLASH", "BYSANT-M3DA")
    if options.d~=nil then
        print "*** Connecting to debugger ***"
        require 'debugger' (options.h, tonumber(options.p), options.k)
    end
    start_web_server (WEB_PORT)
    start_telnet_server (TELNET_PORT)
    platform.backend.init(config, from_device)
end

sched.run(main)
sched.loop()
