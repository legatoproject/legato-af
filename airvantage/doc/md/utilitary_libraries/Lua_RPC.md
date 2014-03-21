Lua RPC
=======

This Lua modules add Remote Procedure Calls facilities in the Lua VM.

It is biderectionnal so it is possible to execute RPC on the local VM
and on the remote VM.

Lua RPC API
===========

The Lua module is called **rpc**. To load it:

~~~~{.lua}
 require 'rpc' 
~~~~

##### Create a Lua RPC bidirectional listening socket

~~~~{.lua}
 client, err = rpc.newserver(address, port) 
~~~~

*address* and *port* are optional, and default values are 'localhost',
1999.\
 This function **blocks** until a client connect and return an instance
of a the remote peer (can do RPC on)

> **WARNING**
>
> The server socket is then closed and no other client will be able to
> reconnect

#### Create a Lua RPC listening socket (server only)

~~~~{.lua}
 status, err = newserveronly(address, port) 
~~~~

*address* and *port* are optional, and default values are 'localhost',
1999.\
 Returns the server socket on success or nil followed by an error in
case of failure.

> **INFO**
>
> The server socket stays listening, and thus several clients can connect
> to that server (in parallel or successively)

#### Create a Lua RPC client

~~~~{.lua}
 client, err = rpc.newclient(address, port) 
~~~~

*address* and *port* are optional, and default values are 'localhost',
1999.\
 This function connects to a listening socket and return an instance of
a remote peer (can do RPC on)

#### Execute an RPC

~~~~{.lua}
 results... = client:call(procedureID, procedureParams...) 
~~~~

This function block until the remote procedure call is finished on the
remote peers. It returns the results of the remote execution.\
 In addition:

-   this function raises an error if the remote call raised an error
-   this function returns nil, "function not existing" if nothing exist
    with the given ID
-   this function returns nil, "not a func" is the procedureID is not a
    function object

#### Evaluate a variable

A specific function is added into and rpc object in order to be able to
evaluate variable.\
 It is then easy to evaluate a variable remotely:

~~~~{.lua}
 value = client:eval(somevariablename) 
~~~~

where *somevariablename* is a string representing the name of the
variable

#### Declare and execute a remote function

A very convenient method helps you to declare and seamlessly call a
function on the remote peer.

~~~~{.lua}
 f = client:newexec(script) 
~~~~

where *script* can be either

-   a string representing Lua code
-   a string buffer (i.e. Lua table containing string fragment as an
    array) representing Lua code
-   an actual (local) function that is going to be exported remotely,
    provided that this function does not have upvalues

Some examples:

~~~~{.lua}
local f = client:newexec("print(...)")
f("toto") -- will call the remote function print giving "toto" as argument

local f = client:newexec({"return", " ... + 1"})
return f(1) -- will return the computation of 1+1 executed remotly



local function g(x)
   print(x)
   return x+1
end
local f = client:newexec(g)
return f(1) -- will return the computation equivalent to the function g but executed remotely
~~~~

RPC protocol
============

The RPC is done though serialization over a socket (TCP/IP) link. The
two peers exchange messages in order to create a RPC and to retrieve the
response. Multiple RPC can be executed on the same link asynchronously
since the header of each message contains a sequence number. However the
sequence number is coded on a single byte, it means that two RPC should
be executed concurrently if they have the same sequence number.

There are two types of messages: Call message and Response message. The
two messages share the same header.\
 Messages are bytes aligned in the stream.

#### Common Header

------    ----------------------------------------------------------------
1 byte    Message type: 0=Call, 1=Response

1 byte    Sequence number. Should start at 0 and increase by one for each 
          successive command. It wraps at 256

4 bytes    Message payload size coded as an unsigned integer
------    ----------------------------------------------------------------

#### Call message

-------           ---------------------------------------------------------------
6 bytes           Header (see above)

x bytes           SerializedObject: Remote procedure ID (usually a string)

x bytes           SerializedObject x n: Remote procedure params
-------           ---------------------------------------------------------------

#### Response message

-------  --------------------------------------------------------------------
6 bytes  Header (see above)

1 byte   Procedure call status: 0=OK, 1=Exception occurred, 2=Procedure ID is
         nil(=does not exist), 3=Procedure ID is not a procdeure (callable
         function)

x bytes  SerializedObject x n: Remote procedure results (when an exception
         occurred it is the description of the exception)
-------  --------------------------------------------------------------------

#### Specific RPC names

Lua RPC defines specific function names that can be used to extend the
RPC capabilities.

-   **__rpceval**: take one parameter that must be a string
    representing a global variable. This call will return the value of
    that variable
-   **rpcbuildexec**: take once parameter that can be either a
    string or string buffer representing some Lua code, or a luatobin
    serialized Lua function. This call return an _execid_ that will be
    necessary to actually call this newly declared function
-   **rpcrunexec**: take one mandatory parameter: the _execid_
    of the remote function to call (see __rpcbuildexec), and some
    optional parameters that will be provided to the function remotly
    called. This call returns the returned values of the executed
    function.

