ConfigStore
===========
 
The following configuration elements are accessible through different
APIs/Protocols: remotely from the server, through an RPC in Lua
applications connected to the Agent process, and locally in the Agent
process (e.g. through the telnet shell).

#### From the Server through M3DA

The configuration table can be read (M3DA Command *ReadNode*) and
written (Data writing).

The path of the elements is prefixed by the path `"@sys.config"`.

#### From The Agent's Lua Shell

In the Lua Shell you can access the configuration table thanks to the
`agent.config` module:

~~~{.lua}
-- Load/retrieve the config module
c = require 'agent.config'
-- print all config from the rest sub category
p(c.rest)
-- assign a new parameter (de-activate rest stuff)
c.rest.activate = false
~~~

#### From an Asset Application

See `DeviceTree` library API. A couple of function for setting and reading
variables are provided. 

Generated doc is available here:

* [Lua DeviceTree API](http://download.eclipse.org/mihini/api/lua/devicetree.html)
* [C DeviceTree API](http://download.eclipse.org/mihini/api/c/swi__devicetree_8h.html)

Most likely, user will need the following command:

~~~{.lua}
dt = require "devicetree"
dt.init()
:dt.get("config")
-- user may append strings after config in order to browse deeper into the tree, e.g. ":dt.get("config.server.autoconnect")"
~~~

How the Config Store works
==========================


#### Default configuration

The Agent comes with a default configuration that can be altered
and restored at any time. \

The default configuration of the Agent is provided by thespecified
into the fil `agent/defaultconfig.lua` file, which ships with each
porting of the Agent to a target. Beware that changes to
`defaultconfig` are only taken into account at first run, or after a
`make clean`: the Agent makes a read/write copy of the config on the
filesystem, which can be modified through the APIs described below,
and won't detect a modification of `defaultconfig`.

#### Persisted configuration

The preferred way to modify an Agent instance's configuration is to
alter its persisted configuration, copied from `defaultconfig` the
first time the Agent runs.  On Linux-like operating systems, the
config is stored in a custom database format, on a read-write file
system (often flash in embedded devices). By default, it goes in the
is stored into `"./persist/ConfigStore"` file, in the Agent's
read+write directory.relative to the directory
where the Agent is executed.

#### Configuration loading workflow

When the Agent starts, it looks for an existing stored configuration
(ConfigStore). If none is found, it is created from the template
provided by `agent.defaultconfig`.

**This means that changing `agent.defaultconfig` after the first start
of the Agent will not change the current configuration of the Agent. A
call to `agent.config.default()` is necessary to reload the default
configuration.**

The configuration tree is stored in persisted memory (flash). Any change
to the configuration is written synchronously, meaning that the settings
are persisted as soon as returning from a `set()` operation.

#### ConfigStore API

The Configuration module has an API to manipulate the configuration;
in a Lua shell, you can type:

~~~{.lua}
agent.config.default(path)   -- reloads the configuration subtree 'path' from the defaultconfig.lua file.
agent.config.diff(path)      -- returns a list of items that are different from the default config.
agent.config.pdiff(path)     -- pretty print the above list
agent.config.set(path, value)-- set a value in the configuration
agent.config.get(path)       -- gets a value for the configuration subtree
~~~

Configuration parameters that can be applied to Mihini
======================================================

#### Agent generic settings

~~~{.lua}
--Defines the local port on which the agent is listening in order to communicate with the assets
agent.assetport = 9999

--Address on which the agent is accepting connection in order to communicate with the assets
--Pattern is accepted: can be set to "*" to accept connection from any address, by default shell accepts only localhost connection.
agent.assetaddress = "*"

--Devices ID used to communicate with the platform server
agent.deviceId = "012345678901234"

--Defines the local port on which the agent is listening in order to receive LUASIGNAL from external applications (Linux only)
agent.signalport = 18888

~~~

#### Server connection settings

~~~{.lua}
--URL on which the agent will try the server connection.
-- Determines the protocol, host, port, and optionally other things such
-- as path, user, password
server.url = "tcp://airvantage.net"

--When the device is behind a proxy this settings defines a HTTP proxy.
-- This parameter is only relevant for HTTP transport protocol
--server.proxy must be a URL starting by "http://".
server.proxy = "some.proxy.server"

--Agent auto connection policy
server.autoconnect = {}
server.autoconnect.onboot = true -- connect a few seconds after the agent started
server.autoconnect.period = 5 -- period in minute (connect every 5 minutes)
server.autoconnect.cron = "0 0 * * *" -- cron entry (connect once a day at midnight)

~~~

#### Communication security settings

~~~{.lua}
-- Security:
-- * authentication is either "hmac-md5" or nil (prevents attackers from forging fake messages,
--   doesn't ensure secrecy).
-- * encryption is only available when authentication is enabled.
--   The format is "<cipher>-<chaining>-<length>", the only officially supported configurations are
--   "aes-cbc-128" and nil.
-- Both settings must match those on the server, and crypto keys on device and server must also match.
-- 
-- server.authentication = 'hmac-md5'
-- server.encryption = 'aes-cbc-128'
~~~

<!--- Commented out for now, Mediation not officially supported
#### Mediation protocol settings

~~~{.lua}
--Activate or de-activate the mediation client on the device
mediation.activate = true
--Connection timeout (in seconds)
mediation.timeout = 5
--Defines the ordered list of mediation server to connect to
mediation.servers = { {addr = "srv1.com", port = 2048}, {addr = "srv2.com", port = 2048} }
--this is equivalent to
--mediation.servers[1].addr = "srv1.com"
--mediation.servers[1].port = 2048
--mediation.servers[2].addr = "srv2.com"
--mediation.servers[2].port = 2048

--Defines the polling period (in seconds) of the mediation protocol according to each bearer
--When period is set to 0, it will do a one time polling when the bearer is mounted.
--If a bearer is absent from this list, mediation protocol is not used on that bearer
mediation.pollingperiod = { ETH=0, GPRS=10}
--this is equivalent to
--mediation.pollingperiod.ETH=0
--mediation.pollingperiod.GPRS=10

--Defines the number of attempts before considering current mediation servers as dead
mediation.servers.retries = 5

--Mediation restart after failure delay (in seconds), default value (if not set) is 1800 seconds (30 minutes)
mediation.retrydelay = 300
~~~
-->


#### Shell related settings

~~~{.lua}
--Activate the Lua Shell
shell.activate = true
--Local port on which the Lua Shell server is listening
shell.port = 2000

--Address on which the Lua Shell server is accepting connection
--Pattern is accepted: can be set to "*" to accept connection from any address, by default shell accepts only localhost connection.
shell.address = "*"

shell.editmode = "edit" -- can be "line" if the trivial line by line mode is wanted
shell.historysize = 30 -- only valid for edit mode
~~~

#### REST related settings

~~~{.lua}
rest = {}
rest.activate = true
rest.port = 8357


-- http digest authentication
rest.authentication = {}
rest.authentication.realm = "username@localhost"
-- HA1 is the MD5 sum of the string "username:realm:password"
rest.authentication.ha1 = "your hash here"


rest.restricted_uri = {}
-- Either globally
rest.restricted_uri["*"] = true

-- Or per URI
rest.restricted_uri["^devicetree"] = true
rest.restricted_uri["^server/?$"] = true
rest.restricted_uri["^application/?$"] = true
rest.restricted_uri["^application/[%w%.%-_]+/?$"] = true
rest.restricted_uri["^application/[%w%.%-_]+/start/?$"] = true
rest.restricted_uri["^application/[%w%.%-_]+/stop/?$"] = true
rest.restricted_uri["^application/[%w%.%-_]+/configure/?$"] = true
rest.restricted_uri["^application/[%w%.%-_]+/uninstall/?$"] = true
rest.restricted_uri["^update/?$"] = true
~~~

#### Time related settings

~~~{.lua}
-- activate Time Services (see ntppolling config param): sync can be done on demand using synchronize API
time.activate = true

--timezone: signed integer representing quarter(s) of hour to be added
--to UTC time (examples: -4 for UTC-1, 23 for UTC+5:45, ...)
time.timezone = 0
-- daylight time saving: signed integer (1, -1) to be added to UTC
time.dst = 0

-- NTP params
time.ntpserver = "pool.ntp.org"

--polling period for auto time sync
--whatever ntppolling value, time sync is done on Agent boot if Time and NetworkManager are activated
--if ntppolling is set to 0 or nil value, no periodic time sync is done
--if set to string value, it will be interpreted as a cron entry (see timer.lua doc)
--else positive number representing minutes is expected to specify periodic time sync
time.ntppolling = 0
~~~

#### Modem configuration

~~~{.lua}
--activate
modem.activate = true
--SIM pin code
modem.pin = ""

--AT Serial interface (Linux Only)
modem.atport = "/dev/ttyS0"
--PPP Serial port
modem.pppport = "/dev/ttyS2"
--export sms api to assets
modem.sms = true
~~~

#### Network connectivity settings

~~~{.lua}
-- Activate / deactivate the NetworkManager
network.activate = true

-- FakeNetman Signal
-- When non nil and network.activate==false then the signal("NETMAN", "CONNECTED", network.initsignal) is emitted when the Agent is initialized
network.initsignal = "Default"

--Maximum failures on bearer selection
--network.maxfailure = 2

--List of supported bearers and ordered priority
 network.bearerpriority = {"ETH","GPRS"}
 --this is equivalent to
 --network.bearerpriority[1] = "ETH"
 --network.bearerpriority[2] = "GPRS"

--amount of time to wait before going back to the preferred bearer if connected bearer is not the first of bearerpriority list.
--if set to nil or equals to 0 netman will never go back automatically to first bearer
network.maxconnectiontime = 30

--SMS fallback - Activate the SMS fallback: if the network is unavailable, an sms is sent instead of making an http connection
--network.smsfallback = "+33102345879" --address to send outgoing smsto (e.g. server SMS reception number)
--network.pinghost --host for tcpping
--network.pingport --port for tcpping

-- Bearer configuration
network.bearer = {}
-- GPRS configuration
-- retry is the number of retries before switching to the next bearer, MANDATORY
-- retryperiod is the time (in seconds on linux) between 2 retries on the same bearer, MANDATORY
network.bearer.GPRS = { retry = 2, retryperiod = 50, apn = "yourapn", username="orange", password="orange"} -- username and password can be not set if not mandatory by the operator

-- If network.bearer.XXX.retry and / or network.bearer.XXX.retryperiod are not specified the NetworkManager will try to use those 'global' setting
network.retry=5
network.retryperiod=50

-- ETHERNET configuration
-- ETHERNET with DHCP
-- retry is the number of retries before switching to the next bearer, MANDATORY
-- retryperiod is the time (in seconds on linux) between 2 retries on the same bearer, MANDATORY
network.bearer.ETH = {retry = 2, retryperiod = 5, mode = "dhcp" }
-- ETHERNET with static IP
-- network.bearer.ETH = {retry = 2, retryperiod = 5, mode = "static", address = "10.0.2.87", netmask = "255.255.0.0", broadcast = "10.0.255.255", gateway= "10.0.0.254", nameserver1 = "10.6.0.224", nameserver2 = "10.6.0.225"}
~~~

#### Device Management 

~~~{.lua}
-- Activate the Device Management module
device.activate = true
-- ip address or host name and port number of the server hosting the ServerAppSide for the TCPRemoteConnect command
-- device.tcprconnect ={addr = '10.41.51.50', port = 2065 }
~~~

#### Logging framework

~~~{.lua}
--default log level: can be one of NONE, ERROR, INFO, DETAIL, DEBUG, ALL. See log.lua for details
log.defaultlevel = "ALL"
-- per module log level
log.moduleslevel.GENERAL = "ALL"
log.moduleslevel.SERVER = "INFO"
-- formating options
log.enablecolors = true
-- change default format for all logs
log.format = "%t %m-%s: %l"
-- timestampformat specifies strftime format to print date/time
-- timestampformat is useful only if %t% is in formatter string
log.timestampformat = "%F %T"
~~~

<!--- Commented out: Log policies needs to be reworked
-- Log policies config : nothing activated by default

-- Log policy is activated only if log.policy is not nil
-- log.policy.name can take 3 values for now: "buffered_all", "sole" or"context"
-- log.policy= {}
-- log.policy.name = "buffered_all"
-- log.policy.name = "sole"
-- log.policy.name = "context"

-- log.policy.params = {}
-- params.level can be used to configure trigger level for some policies
-- log.policy.params.level="WARNING"
-- When activated, log policy needs a config for low level module logstore
-- Shared config for Ram logstore: ramlogger with size param to rammaxsize
-- log.policy.params.ramlogger = {size = 2048}
-- Regular config for Flash logstore for Linux: flashlogger with size param to flashmaxsize and path param to set logs location
-- log.policy.params.flashlogger ={size = 15 * 1024, path="logs/" }
-- Parameter for Log upload command
-- log.policy.ftpuser = ""
-- log.policy.ftppwd = ""
-->


#### Update framework

~~~{.lua}
--Update module settings
--Activate the Update Agent
update.activate = false

-- Update process settings
-- retries number per component, default value:2
update.retries = 2
-- timeout in seconds for component update response, default value:40
update.timeout = 40

-- dwlnotifperiod: number of seconds between update notification during downloads, default value is 2s
update.dwlnotifperiod = 30
~~~

~~~{.lua}
-- Activate Application Container
appcon.activate = false
-- Tcp Port to connect to appmon_daemon.
-- No need to use this config value if using appmon_daemon default port (4242)
appcon.port = 4243

appcon.envvars = {}
-- @LUA_AF_RO_PATH@ is a pattern which is replaced by the runtime path LUA_AF_RO_PATH
-- when the appcon component is loaded.
appcon.envvars.PYTHONPATH = "@LUA_AF_RO_PATH@/python"
appcon.envvars.CLASSPATH = "@LUA_AF_RO_PATH@/java"
~~~

#### Monitoring system

~~~{.lua}
-- activate the monitoring
monitoring.activate = true
-- gives access to the global environment into the monitoring scripts
monitoring.debug = true
~~~

#### Lua RPC server

~~~{.lua}
-- activate LuaRPC server
rpc.activate = true
-- optional: define the address to bind the server socket to. default value is 'localhost'
rpc.address = 'localhost'
-- optional: define the port to bind the server socket to. default value is 1999
rpc.port = 1999
~~~

#### Data policies

~~~{.lua}
-- activate data policies
data.activate = true
data.policy = { } -- list of available Data policies
-- Each data queue correspond to a data sending policy, there are five types of policies:
-- manual: data sending is triggered by the user. Set to true when using this trigger.
-- cron: data sending is triggered on cron events. Set the value to string describing the cron trigger (look into timer.cron API for the syntax).
-- latency: data sending is triggered some times after a data push. Set to the maximum number of seconds to trigger the data sending after the data has been pushed.
-- period: data sending is triggered at a given period. Set to the number of second period.
-- onboot: data sending is triggered after the agent boots. Set to the number of seconds after the agent has boot up.
data.policy.default = { latency = 5, onboot = 30 } -- default data queue to use when no policy name is used when sending data
data.policy.hourly = { latency = 60 * 60 }
data.policy.daily = { latency = 24 * 60 * 60 }
data.policy.never = { manual = true }
data.policy.now = { latency = 5, onboot = 30 }
data.policy.onboot = { onboot = 30 }
~~~
