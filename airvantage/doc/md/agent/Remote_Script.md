Remote Script
=============

This agent feature enables to run **Lua scripts** automatically on
the device.

#### 1. Script Delivery

The script is sent by the server using :

-   Dedicated M3DA '**ExecuteScript**' command that provides access to Lua bytecode using url.

So it takes advantage of:

-   Task automation on server
-   Task scheduling on server
-   Task acknowledgment
-   ...

#### 2. Security

The script will be executed with **no restriction**, so the content of
the script is very **delicate**.\
 We need to check integrity and authenticate the sender of the script
(i.e. the server).

The script will have to be signed and the signature will have to be sent
with the script.\
 The choice in the security technique and how the signature will be sent
is highly related to M3DA security usage.

#### 3. Script API


The only constraint about a remote script is that it must **throw** an
error to report an failure during the execution.\
 The error will be caught and reported to the server.\
 The error can be thrown:

-   using an API that throw errors like assert()
-   manually call error() function

#### 4. M3DA Command Description

See **ExecuteScript** command description in [Device
Management](Device_Management.html)

#### 5. Some interesting purposes

##### 5.1 Light Update

<<<<<<< HEAD
This Remote Script can be used to update small software parts. It is **not** intended to perform **big software updates**,
but rather to allow remote command execution, small code update...

This feature can make patching of Lua parts very easy for remote debug/hack, but it is not meant to create a new whole update 
mechanism framework.
To update large amount of code and take advantage of software version management, please use [Software Update
    Module](Software_Update_Module.html)
 
##### 5.2 Monitoring Script Update

One interesting use of Remote Script is to remotely install a new script in agent [Monitoring](Monitoring.html) engine.

~~~~{.lua}
local m = require "agent.monitoring"
local script = "local function action() local data = {var1 = { timestamps = {time()}, data = {system.var1} }}; sendtimestampeddata('system', data); end; connect(onchange('system.var1'),action)"
local res, err = m.install("script1",script)
if not res then error(err) end
~~~~

