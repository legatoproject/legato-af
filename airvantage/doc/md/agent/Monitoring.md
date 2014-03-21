Monitoring
==========

The Monitoring module is a Rule Engine that allows writing simple scriplet to
implement basic logic, based on a set of events (called triggers), executing some
function (called actions).

The general idea of the Monitoring Script Engine is to instanciate your triggers and
actions and then connect them. The rule language is actually based on Lua which make easy
to write clean and simple rules to implement your logic. Scriptlets can be loaded and unloaded dynamically
in the system (See monitoring module APIs).

The Monitoring Script Engine comes with a set of predefined triggers and actions that can be
used in a scriptlet.




## Predefined triggers

### onboot

~~~~{.lua}
onboot()
~~~~

Activated when the agent is started, usually when the device boots (depends on how it is ported).

### onconnect

~~~~{.lua}
onconnect()
~~~~

Activated before a connection with the Device Management Server is done.
        
> **Note** this trigger is temporarily unavailable (need to work on the server connector module of the agent)

### onperiod

~~~~{.lua}
onperiod(p)
~~~~

Activated on a timer:

* `p` > 0 means a periodic timer.
* `p` <= 0 means a one shot timer.
* `p` is in seconds.


### ondate

~~~~{.lua}
ondate(cron)
~~~~

Activated on a date. This date is specified with a cron entry (see timer.cron documetnation for format).

Ex: With the compatible Unix CRON syntax, `"22 7 * * *"` means every day at 7:22 am

### onchange

~~~~{.lua}
onchange(...)
~~~~

Activated when one of the variable from the list changes.
        
`...` is a variable number of variable to monitor, named as string: ex `"system.batterylevel"`, `"user.somevar"`, etc.
  
The first argument can also be a table containing all the variables, in that case the remaining args are ignored.

> **Note** the action functions will be given a table containing the changed variables and their value as first argument

### onhold

~~~~{.lua}
onhold(timeout, ...)
~~~~

Activated when none of the listed variables change for the `timeout` amount of time.

- if `timeout` > 0 then once activated, the trigger is re-armed only when a variable changes.
- if `timeout`timeout < 0 then the trigger is automatically re-armed.
- `...` is a list of variable as strings.

> **Note** the action functions will be given a table containing the variables and the value that caused the last rearm of the holding timer, as first argument


### onthreshold

~~~~{.lua}
onthreshold(threshold, var, edge)
~~~~

Activated when a value traverse a `threshold` (previous and new value are opposite side of the `threshold` value)
when `edge` is specified it can be one of `"up"`, `"down"`, `"both"` meaning only triggering on rising edge or falling edge, or on both.

- an `"up"` edge is detected when `oldval` < `threshold` and `newval` >= `threshold`
- a `"down"` edge is detected when `oldval` >= `threshold` and `newval` < `threshold`
- the default is `"both"` edge



### ondeadband

~~~~{.lua}
ondeadband(deadband, var)
~~~~

Activated when a value goes outside the `deadband` range: activated if `abs(newval-oldval) >= deadband`.
        
`oldval` is updated when the trigger is activated.



## Predefined actions

Actions are regular Lua functions. Most of the framework API can be used for that purpose.
        
For typical actions, see the [Racon Lua library](../agent_connector_libraries/Racon_Lua_library.html).


## Filtering and connecting rules

The system provides two additional functions to write the rule scriplets.
        
### connect

~~~~{.lua}
connect(trigger, action)
~~~~

Connects a `trigger` to an `action`. The action will be executed each time the trigger is activated.

Optionally some triggers will pass some parameters to the action function. See the trigger documentation for more details.


### filter

~~~~{.lua}
filter(test, trigger)
~~~~

Filter a trigger given a test function.

 * `test` is a function that returns a boolean value: `true`  means the actions are executed
 * `trigger` output of a trigger function.

In filter returns a new trigger that has the original trigger behavior filtered by the `test` function.

Example:

~~~~{.lua}
connect(
    filter(
           function() return not system.externalpower end,
           onchange("system.batterylevel")),
    function() print("Battery Level Changed") end)
~~~~

will only print the battery level when it changes *and* the power cord is unplugued.



## Monitoring variables

Each monitoring script has its own execution environment. It means that
a global lua variable will not be visible from another monitoring
script.

Scriptlets can register events on device tree variable changes. The whole device tree is accessible,
provided the scriptlet has the correct permissions.

The device tree structure depends heavily on what [Tree Manager](Tree_Manager.html) handlers (extensions) has
been ported on the device.

The following list is a guideline to name variables in an homogenous way across devices. Some of there variables might
not be available depending on your hardware and how Mihini was ported on the device.


### Cellular

------------------------------------------------------------------------------------------------------------------------------------------------------
Variable name                Variable Type        Variable description                    Available on
-------------                -------------        --------------------                    ------------------------------------------------------------
cellurar.rssi                integer              Cellular RSSI level                     On any device that support the standard AT+CSQ AT command

cellular.ber                 integer              Cellular BER level                      On any device that support the standard AT+CSQ AT command

cellular.imei                string               Cellular IMEI                           On any device that support the standard AT+CGSN AT command

cellular.imsi                string               SIM IMSI number                         On any device that support the standard AT+CIMI AT command
------------------------------------------------------------------------------------------------------------------------------------------------------

### Power

------------------------------------------------------------------------------------------------------------------------------------------------------
Variable name                Variable Type        Variable description                    Available on
-------------                -------------        ------------------------------          ------------------------------------------------------------
batterylevel                 integer              Level of charge of the device
                                                  battery

externalpower                boolean              true if the device is powered 
                                                  by an external source
------------------------------------------------------------------------------------------------------------------------------------------------------

### Memory / CPU

--------------------------------------------------------------------------------------------------------------------------------------------------------------
Variable name                Variable Type        Variable description                            Available on
-------------                -------------        --------------------------------------          ------------------------------------------------------------
luaramusage                  integer              Quantity of memory used by the                  All (included in standard lua)
                                                  Lua VM (the one running the agent) \
                                                  The value is the one returned by 
                                                  collectgarbage("count") preceded by
                                                  a collectgarbage("collect") in order 
                                                  to provided consistent numbers. 
                                                  The direct consequence is that reading 
                                                  this variable has a non null CPU cost.
                                                  Use sparingly.

totalramavailable            integer              Total quantity of RAM available on the          Linux
                                                  system

totalramused                 integer              Total quantity of RAM used on the system        Linux

cpuload                      integer              Average CPU load                                Linux

totalflashavailable          integer              Total quantity of flash available on the 
                                                  system

totalflashused               integer              Total quantity of flash used on the 
                                                  system
---------------------------------------------------------------------------------------------------------------------------------------------------------------

### NetworkManager


--------------------------------------------------------------------------------------------------------------------------------------------------------------
Variable name                    Variable Type        Variable description                            Available on
-------------                    -------------        ----------------------------------------        --------------------------------------------------------
netman.BEARERNAME.connected      boolean              connection state of the bearer

netman.BEARERNAME.ipaddr         string               ip address of the bearer

netman.BEARERNAME.hwaddr         string               MAC address of the bearer                       Ethernet bearer

netman.BEARERNAME.netmask        string               netmask address of the bearer

netman.BEARERNAME.gw             string               gateway address of the bearer

netman.BEARERNAME.nameserver1    string               dns address #1 (there can be several 
                                                      dns address, usually 2)

netman.BEARERNAME.mountdate      number               timestamp of the last successful mount

netman.BEARERNAME.mountretries   number               number of retries used for the last 
                                                      successful mount

netman.BEARERNAME.RX             number               number of bytes received by this bearer         **Linux only**

netman.BEARERNAME.TX             number               number of bytes transmitted by this             **Linux only** 
                                                      bearer

netman.defaultbearer             string               Default (=selected) bearer: bearer used 
                                                      as default route, variable value is the 
                                                      BEARERNAME
--------------------------------------------------------------------------------------------------------------------------------------------------------------

## Monitoring Engine API

The Monitoring Engine can install and load scripts dynamically.

### Load scriptlets

~~~~{.lua}
-- Load a monitoring script in memory.
-- name is a string name
-- script can be either
--              a string containing a monitoring script
--              a table string buffer (list of chunk of string)
--              or an actual Lua function containing the script
load(name, script)


-- Unload (and stop) a monitoring script from memory.
-- name is a string name, given at loading time (see load() )
unload(name)
~~~~

### Install scriptlets

~~~~{.lua}
-- Install a monitoring script from a file. Installing a script with an already used name will cause the
-- initial script to be unloaded and uninstalled (replaced by this script)
--      name is the name given to the script
--      filename is the path of the file to install
--      enable is a boolean. true means that the script will enabled (started)
install(name, filename, enable)


-- Uninstall a monitoring script.
-- This function automatically stop the given script if it was enabled (running).
--      name is the name given at installation time.
uninstall(name)
~~~~


### Manage scriptlets

~~~~{.lua}
-- Set the enable flag on an already installed script.
-- Enabling a disabled script will cause a loading, and similarly, disabling an enabled
-- script will cause an unloading.
--      name is the name of the script to configure
--      set is a boolean: true will enable the script, false will disable the script
enable(name, set)


-- List installed or loaded scripts.
--      type: if type is set to "installed", list the installed scripts
--            if type is set to "loaded", list the loaded scripts
list(type)
~~~~



## Scriptlets Examples

### Flip a GPIO every 10 seconds
~~~~{.lua}
connect(
    onperiod(10),
    function() system.gpio[4] = not system.gpio[4] end)
~~~~


### Activate a GPIO when battery is low

~~~~{.lua}
connect(
    onthreshold(10, "system.batterylevel", "down"),
    function() system.gpio[4] = true end)
~~~~

### Send an alarm on high temperature

~~~~{.lua}
local t = onthreshold(55, "system.temperature", "up")
local a = function()
    pushdata("alarm.temperature", {value = system.temperature, timestamp = time()})
end
connect(t, a)
~~~~

### Print when battery level changes

~~~~{.lua}
connect(
    filter(
           function() return not system.externalpower end,
           onchange("system.batterylevel")),
    function() print("Battery Level Changed") end)
~~~~

