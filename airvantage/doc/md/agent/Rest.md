Agent REST service
==================

The Agent REST service exposes other Agent services by a REST API.

### 1. Specifications

#### Returned values
Each REST function returns the same value as the function to which it corresponds, except that it is serialized with JSON

### 2. Agent Services

See below a list of the current Lua API with their corresponding REST API:

Notes: 

-    REST URL are given relative to the base URL like  `http://localhost:8357/` (see `Configuration` section for port configuraion).
-    URL parameters are given using this syntax: `http://localhost:8357/server/connect?latency=42`.

#### Server Connector

------------------------------------------------------------------------------------------------------------
Module API              REST URL                   URL parameter          HTTP method        HTTP payload   
----------              ------------------         -------------          ---------------    ---------------
server.connect(latency) server/connect             latency=X (optional, \ GET                Nothing        
                                                   with X an integer)
------------------------------------------------------------------------------------------------------------
\


#### Devicetree

---------------------------------------------------------------------------------------------------------------------
Module API                                       REST URL                       HTTP method     HTTP payload
----------                                       ------------------             -------------   ---------------------
devicetree.get("node.subnode.leaf")              devicetree/node/subnode/leaf    GET            Nothing

devicetree.set("node.subnode.leaf", "myvalue")   devicetree/node/subnode/leaf    PUT            The new value of "node.subnode.leaf" serialiazed with JSON
---------------------------------------------------------------------------------------------------------------------
\


#### Application container

---------------------------------------------------------------------------------------------------------------------
Module API                          REST URL                       HTTP method     HTTP payload
----------                          ------------------             -------------   ---------------------
appcon.list()                       application                    GET             Nothing

appcon.status("id")                 application/id                 GET             Nothing

appcon.start("id")                  application/id/start           PUT             Empty JSON table ({ }) or empty JSON string ""

appcon.stop("id")                   application/id/stop            PUT             Empty JSON table ({ }) or empty JSON string ""

appcon.configure("id", autostart)   application/id/configure       PUT             A boolean value corresponding to "autostart" and serialized with JSON

appcon.uninstall("id")              application/id/uninstall       PUT             Nothing
---------------------------------------------------------------------------------------------------------------------
\

Notes:

- for `application/id/uninstall` REST API, if Agent Update module is activated, the application uninstallation will be done using Update uninstall job before being forwarded to appcon.uninstall API.


#### Update

----------------------------------------------------------------------------------------------------------------------------------
Module API                      REST URL        URL parameter                                   HTTP method        HTTP payload   
----------                      ------------    -------------                                   ---------------    ---------------
update.localupdate(path, sync)  update          sync=[0-1] (optional, asynchronous by default)  POST               The tar archive sent as binary data in a http chunked request

update.getstatus()              update          sync=[0-1] (optional, asynchronous by default)  GET                Nothing
----------------------------------------------------------------------------------------------------------------------------------
\


### 3. Configuration

Agent REST parameters can be changed in the Agent config store.

defaultconfig.lua:

~~~~{.lua}
-- REST related settings
rest = {}
rest.activate = true
rest.port = 8357
~~~~

Lua shell:

~~~~{.lua}
$ telnet localhost 2000
> :agent.config.rest.activate
true
> :agent.config.rest.port
8357
~~~~
