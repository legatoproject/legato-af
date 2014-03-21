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
--     Guilhem Saurel     for Sierra Wireless - default configuration
-------------------------------------------------------------------------------

-- Default configuration file for Linux targets

    local config = {}
    setfenv(1, config)

    server = {}

    server.serverId = "AIRVANTAGE"
    server.retrytimes = 10
    server.retryperiod = 60

    -- Determines the protocol, host, port, and optionally other things such
    -- as path, user, password
    server.url = "tcp://m2m.eclipse.org:44900"
    --server.url = "http://localhost:8070/device/com"
    --server.url = "http://webplt-qa.anyware-tech.com/device/com"
    --server.url = "http://webplt-m2m.anyware-tech.com/device/com"
    --server.url = "http://m2mop.net/device/com"

    --When the device is behind a proxy this settings defines a HTTP proxy. This parameter is only relevant for HTTP transport protocol
    --server.proxy must be a URL starting by "http://".
    --server.proxy = "http://some.proxy.server:port"

    -- Security: authentication is one of "hmac-sha1" or "hmac-md5" (or nil)
    -- Encryption cannot be enabled without authentication. It's of the form
    -- "<cipher>-<chaining>-<length>", where cipher must be "aes", chaining is
    -- either "ctr" or "cbc", length is either "128" or "256".
    -- server.authentication = 'hmac-sha1'
    -- server.encryption = 'aes-cbc-128'

    -- Agent auto connection policy
    server.autoconnect = { }
    -- server.autoconnect.onboot = true -- connect a few seconds after the Agent started
    -- server.autoconnect.period = 5 -- period in minute (connect every 5 minutes)
    -- server.autoconnect.cron = "0 0 * * *" -- cron entry (connect once a day at midnight)


    agent = {}
    agent.assetport = 9999 -- connection port, used to communicate with all the local clients
    --Address on which the agent is accepting connection in order to communicate with the assets
    --Pattern is accepted: can be set to "*" to accept connection from any address, by default shell accepts only localhost connection.
    --agent.assetaddress = "*"
    agent.deviceId = ""  --set deviceId to nil/empty string -> deviceId will be generated using agent.platform.getdeviceid()

    agent.signalport = 18888 -- port used for LUASIGNAL fwk (Linux only)


    -- Shell related settings
    shell = {}
    shell.activate = true
    shell.port = 2000
    shell.editmode = "edit" -- can be "line" if the trivial line by line mode is wanted
    shell.historysize = 30  -- only valid for edit mode,
    shell.address = "*"

    -- Rest related settings
    rest = {}
    rest.activate = true
    rest.port = 8080

    -- Time related settings
    time = {}
    -- activate Time Services: periodic polling only for now, sync can always be done using synchronize API on demand.
    time.activate = false
    --timezone: signed integer representing quarter(s) of hour to be added to UTC time (examples: -4 for UTC-1, 18 for UTC+5:45, ...)
    time.timezone = 0
    -- daylight saving time: signed integer representing quarter(s) of hour to be added to UTC time
    time.dst = 0

    time.ntpserver = "pool.ntp.org"
    --polling period for auto time sync
    --irrespective of `ntppolling`'s value, time sync is performed when the Agent boots if both `time` and `network` are activated
    --if `ntppolling` is set to 0 or `nil`, no periodic time sync is done
    --if set to a string value, it will be interpreted as a cron entry (cf. `timer` doc)
    --otherwise, a positive number representing minutes is expected, to specify periodic time sync.
    time.ntppolling = 0

    -- Modem configuration
    modem = {}
    --modem.activate = true
    --modem.pin = ""
    modem.atport = "/dev/ttyS0"
    modem.pppport = "/dev/ttyS0"


    network = {}
    network.activate = false
    --network.maxfailure = 2
    network.bearerpriority = {"ETH","GPRS"}
    network.smsfallback = false
    -- amount of time to wait before going back to the preferred bearer if connected bearer is not the first of bearerpriority list.
    -- if set to nil or equals to 0 netman will never go back automatically to first bearer.
    network.maxconnectiontime = 30
    --network.retry = 5 --use only if network.bearer.XXX.retry not defined
    --network.retryperiod = 10 --use only if network.bearer.XXX.retryperiod not defined
    --network.smsfallback = "+33102345879" -- address to send outgoing sms to (e.g. server SMS reception number)
    network.bearer = {}
    network.bearer.GPRS = {apn = "internet-entreprise", retry = 2, retryperiod = 10, automount = true}
    network.bearer.ETH = {mode = "dhcp", retry = 2, retryperiod = 10, automount = true}
    --network.bearer.ETH = {mode = "static", retry = 2, retryperiod = 50, automount = true, address = "10.0.2.87", netmask = "255.255.0.0", broadcast = "10.0.255.255", gateway= "10.0.0.254", nameserver1 = "10.6.0.224", nameserver2 = "10.6.0.225"}


    log = {}
    log.defaultlevel = "INFO" -- default log level: can be one of NONE, ERROR, WARNING, INFO, DETAIL, DEBUG, ALL. See log.lua for details
    log.moduleslevel = { }
    --log.moduleslevel.GENERAL = "ALL"    -- per module log level
    --log.moduleslevel.SERVER  = "INFO"   -- per module log level
    log.moduleslevel.SCHED     = "INFO"   -- per module log level
    log.moduleslevel.TREEMGR   = "DETAIL" -- per module log level
    log.enablecolors = false

    -- change default format for all logs
    --log.format = "%t %m-%s: %l"
    -- timestampformat specifies strftime format to print date/time.
    -- timestampformat is useful only if %t% is in formater string
    log.timestampformat = "%F %T"


    update = {}
    update.activate = true
    --update package file name to use for local update file detection
    --update.localpkgname="updatepackage.tar.lzma"
    --dwlnotifperiod: number of seconds between update notification during downloads, default value is 2s.
    --update.dwlnotifperiod = 30

    -- Application Container
    appcon={}
    appcon.activate = true
    -- Tcp Port to connect to appmon_daemon.
    -- No need to use this config value if using appmon_daemon default port (4242)
    --appcon.port = 4243

    --appcon.envvars = {}

    -- @LUA_AF_RO_PATH@ is a pattern which is replaced by the runtime path LUA_AF_RO_PATH
    -- when the appcon component is loaded.
    --appcon.envvars.PYTHONPATH = "@LUA_AF_RO_PATH@/python"
    --appcon.envvars.CLASSPATH = "@LUA_AF_RO_PATH@/java"


    -- Device Management Application/Commands
    device = {}
    device.activate = true
    --device.tcprconnect = {addr = '10.41.51.50', port = 2065}

    -- Monitoring system
    monitoring = {}
    monitoring.activate = false

    -- Lua RPC server
    rpc = {}
    --rpc.port = 1999
    rpc.activate = true
    rpc.address = '*'

    data = { }
    data.activate = true
    data.policy = { }
    data.policy.default  = { latency = 5, onboot=30 }
    data.policy.hourly   = { latency = 60*60 }
    data.policy.daily    = { latency = 24*60*60 }
    data.policy.never    = { 'manual' }
    data.policy.now      = { latency = 5, onboot=30 }
    data.policy.on_boot  = { onboot=30 }
    return config
