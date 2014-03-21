Device Management
=================

#### Commands

This page references all generic commands used by a M3DA server. The
commands are serialized and sent using M3DA Protocol.

This is an "applicative level" specification opposed to the
"serialization level" specification from the M3DA Protocol document.

In order to send command from the server to the device, or from the
device to the server, an **M3DA** data writing is to be done on a node
path, encapsulated into an **M3DA::****Message** object as stated into
the protocol specification.

#### Main

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Command Name                Command Argument\ name -> description (type)            Comments
------------                --------------------------------------------            -----------------------------------------------------------------------
ReadNode                    "1" -\> path.of.the.node.to.read (string),\             Read a node or several nodes and its/their children
                            ...\                                                    children (if any) from a tree. The node can be either
                            "n" -\> path.of.the.node.to.read (string)               terminal (a leaf: to get the value of a property) or 
                                                                                    have sub nodes. In the later case the content of the node
                                                                                    and all its sub nodes will be transmitted. \
                                                                                    This command provokes the sending of Data Messages that 
                                                                                    that will contain the data associated to that node. 
                                                                                    The data of the message is composed of a map that hold the 
                                                                                    properties as key, and their value as value. \
                                                                                    If a node contains a child, Data Messages will be sent 
                                                                                    recursively in order to present all the data of the sub 
                                                                                    nodes as well. If the specified node does not exist or 
                                                                                    contains no data, a nil value will be sent. \
                                                                                    **Special value ""** (empty string) means reading the **whole** tree.

Connect                                                                             Ask the Agent to connect to the platform server.\
                                                                                    The command must be addressed to the device
                                                                                    (M3DA::Message.path = "@sys").

Reboot                                                                              Ask the device or one of its asset to reboot. \
                                                                                    The device or the asset is designated thanks to the path 
                                                                                    of the M3DA::Message that wrap this command.

ResetToFactoryDefault          "restart" -> once cleared the agent                  Reset agent settings to factory defaults. All persisted 
                               reboots after a given number of seconds              data about agent settings and installed software are lost. \
                               or defaults to 6 seconds if this param               \
                               is equal to "true" or "1"                            \
                               (boolean, number)                                    **Impacted** modules/functionalities: \
                                                                                    - Agent persisted **config** is reset to **defaultconfig
                                                                                      **( depending on the differences between defaultconfig.lua and
                                                                                      persisted config, this operation may impact: server url,
                                                                                      hearbeat, ..., or any Agent config parameter) \
                                                                                    - Treemgr mapping are reset: it will be regenerated from .map
                                                                                      files on next boot \
                                                                                    -  **Persisted M3DA data**: asset and device data are reset \
                                                                                    -  **Applications** installed in ApplicationContainer are erased \
                                                                                    -  Update module: any update in progress, **software version list** \
                                                                                       is cleared. \
                                                                                    **Not impacted** modules: M3DA security credentials
-----------------------------------------------------------------------------------------------------------------------------------------------------------


#### Software update

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Command Name                Command Argument\ name -> description (type)          Comments
--------------              --------------------------------------------          -----------------------------------------------------------------------
ExecuteScript               ( "url" | "1" ) -> url to retrieve the Lua            -> url to retrieve the Lua script \
                            script (string) \                                     The Lua script can be either Lua source file or precompiled Lua
                            \                                                     bytecode file. \
                            \                                                     \
                            ( "signature" | "2" ) -> signature of the script      -> signature of Lua Script \
                            (string)                                              The signature will fit the security level defined within the
                                                                                  Agent. First step: Signature will be **MD5 hash**, and will be sent in
                                                                                  **hexa** in an ascii string.

SoftwareUpdate              ( "url" | "1" ) -> url to download the package        -> url provided by the server where the Software Update Package can be
                            (string)                                                 downloaded. \
                            \                                                        Must **not** end by a trailing "/" character, unless archive name 
                            \                                                        contains one (not recommended) \
                            ( "signature" | "2" ) -> signature of the package     -> signature of the Software Update Package \
                            (string)                                                 The signature will fit the security level defined within the
                                                                                     Agent. \
                                                                                     \
                                                                                     First step: Signature will be **MD5 hash**, and will be sent in
                                                                                     **hexa** in an ascii string which size must be 32 chars (prefixing zeros
                                                                                     chars must be sent!), and in lower case only.
-----------------------------------------------------------------------------------------------------------------------------------------------------------


#### TCP Remote Connection

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Command Name                Command Argument\ name -> description (type)          Comments
--------------              --------------------------------------------          -------------------------------------------------------------------------
TCPRemoteConnect                                                                  Install a TCP tunnel
-----------------------------------------------------------------------------------------------------------------------------------------------------------


#### Application Container

-----------------------------------------------------------------------------------------------------------------------------------------------------------
Command Name                Command Argument\ name -> description (type)          Comments
--------------              --------------------------------------------          -------------------------------------------------------------------------
appcon.start                ( "appname" | "1" ) -> application name               Start an application 
                            (string)

appcon.stop                 ( "appname" | "1" ) -> application name               Stop an application
                            (string)

appcon.autostart            ( "appname" | "1" ) -> application name               Configure an application to start automatically or not
                            (string)\
                            \
                            ( "autostart" | "2" ) -> autostart flag
                            (boolean)
------------------------------------------------------------------------------------------------------------------------------------------------------------


#### Variables

-----------------------------------------------------------------------------------------------------------------------------------------
Variable                                    Read/Write          Description
------------------------------------        ----------          -------------------------------------------------------------------------
@sys.appcon.list                            RO                  List of all applications currently managed by appcon, as a single string
                                                                of space-separated names

@sys.appcon.apps.\<appname\>.started          RW                Whether the application is currently started (Boolean)

@sys.appcon.apps.\<appname\>.autostart        RW                Whether the application starts automatically (Boolean)

@sys.appcon.apps.\<appname\>.runnable         RO                Whether it is a runnable application

@sys.appcon.apps.\<appname\>.\<daemonattr\>     RO              The current value of every daemon attribute \<daemonattr\>.
                                                                Current attributes include: \
                                                                appname, priviledged, prog, wd, pid, startcount, lastexittype,
                                                                lastexitcode
-----------------------------------------------------------------------------------------------------------------------------------------

