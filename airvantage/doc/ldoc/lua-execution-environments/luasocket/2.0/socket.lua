--[[-----------------------------------------------------------------------------
 The socket namespace contains the core functionality of LuaSocket.

 To obtain the socket namespace, run:
    -- loads the socket module 
    local socket = require("socket")
@module socket
]]

--[[------------------------------------------------------------------------------
 This function is a shortcut that creates and returns a TCP server object bound to a local address and port, 
 ready to accept client connections. 
 Modified from the original Luasocket function to support an optional hook argument.

 @function [parent=#socket] bind
 @param host local address to bind to
 @param port local port to bind
 @param backlog_or_hook  Optionally, user can also specify the backlog argument to the listen method (defaults to 32).
  This third parameter can also be the hook if no backlog parameter  is given.
-@param hook is an additional parameter (not compatible with luasocket original socket.bind()
 @return TCP server object
 @usage socket.bind("someaddress", 4242, function hook(...) end)
 @usage socket.bind("someaddress", 4242, 32,function hook(...) end)
]]

--[[------------------------------------------------------------------------------
 This function is a shortcut that creates and returns a TCP client object 
 connected to a remote host at a given port. 

 Optionally, the user can also specify the local address and port to bind 
 (`locaddr` and `locport`).

 @function [parent=#socket] connect
 @param address address to connect to
 @param port port
 @param locaddr optional local address to bind
 @param locport optional local port to bind
 @return TCP client object
]]

--[[------------------------------------------------------------------------------
 This constant is set to `true` if the library was compiled with debug support.
 @field [parent=#socket] #boolean _DEBUG
]]

--[[------------------------------------------------------------------------------
 Creates and returns a clean @{#socket.try} function that allows for cleanup
 before the exception is raised.
 
 `finalizer` is a function that will be called before try throws the exception. 
 It will be called in protected mode.

 The function returns your customized @{#socket.try} function.
 
 Note: This idea saved a lot of work with the implementation of protocols 
 in LuaSocket:
     foo = socket.protect(function()
         -- connect somewhere
         local c = socket.try(socket.connect("somewhere", 42))
         -- create a try function that closes 'c' on error
         local try = socket.newtry(function() c:close() end)
         -- do everything reassured c will be closed 
         try(c:send("hello there?\r\n"))
         local answer = try(c:receive())
         ...
         try(c:send("good bye\r\n"))
         c:close()
      end)

 @function [parent=#socket] newtry
 @param finalizer function that will be called before try throws the exception.
 @return a customized @{#socket.try} function
]]

--[[------------------------------------------------------------------------------
 Converts a function that throws exceptions into a safe function.
 
 This function only catches exceptions thrown by the @{#socket.try} 
 and @{#socket.newtry} functions. 
 It does not catch normal Lua errors.
 
 `func` is a function that calls @{#socket.try} (or `assert`, or `error`) 
  to throw exceptions.

 Returns an equivalent function that instead of throwing exceptions, 
 returns `nil` followed by an error message.

 Note: Beware that if your function performs some illegal operation that raises an error, the protected function will catch the error and return it as a string. This is because the try function uses errors as the mechanism to throw exceptions.

 @function [parent=#socket] protect
 @param func function that calls @{#socket.try} (or `assert`, or `error`) 
  to throw exceptions.
 @return an equivalent function that instead of throwing exceptions, 
  returns `nil` followed by an error message
]]

--[[------------------------------------------------------------------------------
 Waits for a number of sockets to change status.

 `Recvt` is an array with the sockets to test for characters available 
 for reading.
 Sockets in the `sendt` array are watched to see if it is OK 
 to immediately write on them.
 `Timeout` is the maximum amount of time (in seconds) to wait for a change
 in status. A `nil`, negative or omitted `timeout` value allows the function
 to block indefinitely.
 `Recvt` and `sendt` can also be empty tables or `nil`.
 Non-socket values (or values with non-numeric indices) in the arrays
 will be silently ignored.
 
 The function returns a list with the sockets ready for reading,
 a list with the sockets ready for writing and an error message.
 The error message is "timeout" if a timeout condition was met and `nil` otherwise.
 The returned tables are doubly keyed both by integers and also by the sockets 
 themselves, to simplify the test if a specific socket has changed status.
 
 * **Important note**: a known bug in WinSock causes select to fail 
 on non-blocking TCP sockets. The function may return a socket as writable 
 even though the socket is not ready for sending.
 
 * **Another important note**: calling select with a server socket
 in the receive parameter before a call to @{#server.accept} does not 
 guarantee accept will return immediately. 
 Use the settimeout method or accept might block forever.
 
 * **Yet another note**: If you close a socket and pass it to select, 
 it will be ignored.
 
 @function [parent=#socket] select
 @param recvt an array with the sockets to test for characters available 
  for reading. Can also be empty table or nil. Non-socket values 
  (or values with non-numeric indices) in the array will be silently ignored.
 @param sendt an array of sockets that are watched to see if it is OK
  to immediately write on them. Can also be empty table or nil. Non-socket values 
  (or values with non-numeric indices) in the array will be silently ignored. 
 @param timeout optional maximum amount of time (in seconds) 
  to wait for a change in status.
  A nil, negative or omitted timeout value allows the function to block indefinitely. 
 @return a list with the sockets ready for reading,
 a list with the sockets ready for writing and an error message.
]]

--[[------------------------------------------------------------------------------
 Creates an [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) sink from a stream socket object.

 @function [parent=#socket] sink
 @param mode Defines the behavior of the sink. The following options are available:

 - **"http-chunked"**: sends data through socket after applying the chunked transfer coding, closing the socket when done;
 - **"close-when-done"**: sends all received data through the socket, closing the socket when done;
 - **"keep-open"**: sends all received data through the socket, leaving it open when done.
 @param socket The stream socket object used to send the data.
 @return A sink with the appropriate behavior.
]]

--[[------------------------------------------------------------------------------
 Drops a number of arguments and returns the remaining.
 Note: This function is useful to avoid creation of dummy variables:

    -- get the status code and separator from SMTP server reply 
    local code, sep = socket.skip(2, string.find(line, "^(%d%d%d)(.?)"))

 @function [parent=#socket] skip
 @param d The number of arguments to drop. 
 @param vararg The arguments.
 @return  ret<sub>d+1</sub> to ret<sub>n</sub>.
]]

--[[------------------------------------------------------------------------------
 Freezes the program execution during a given amount of time.

 @function [parent=#socket] sleep
 @param #number time Number of seconds to sleep for. The function truncates time down to the nearest integer.
]]

--[[------------------------------------------------------------------------------
Creates an [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) source from a stream socket object.

 @function [parent=#socket] source
 @param mode Defines the behavior of the source. The following options are available:

 - **"http-chunked"**: receives data from socket and removes the chunked transfer coding before returning the data;
 - **"by-length"**: receives a fixed number of bytes from the socket. This mode requires the extra argument length;
 - **"until-closed"**: receives data from a socket until the other side closes the connection.
 @param socket The stream socket object used to receive the data.
 @return A source with the appropriate behavior.
]]

--[[------------------------------------------------------------------------------
 Gives time
 @usage
 t = socket.gettime()
 -- do stuff
 print(socket.gettime() - t .. " seconds elapsed")
	
 @function [parent=#socket] gettime
 @return The time in seconds, relative to the origin of the universe. 
 You should subtract the values returned by this function to get meaningful values.
]]

--[[------------------------------------------------------------------------------
 Throws an exception in case of error. 
 The exception can only be caught by the protect function. It does not explode into an error message.
 @usage
 -- connects or throws an exception with the appropriate error message
 c = socket.try(socket.connect("localhost", 80))

 @function [parent=#socket] try
 @param ... Can be arbitrary arguments, but are usually the return values of a function call nested with try.
 @returns ... arguments form arg<sub>1</sub> to arg<sub>N</sub>, if arg<sub>1</sub> is not nil. Otherwise, it calls error passing arg<sub>2</sub>.
]]

--[[------------------------------------------------------------------------------
 This constant has a string describing the current LuaSocket version.
 @field [parent=#socket] #string _VERSION
]]

--[[------------------------------------------------------------------------------
 Creates and returns a TCP master object. A master object can be transformed into a server object with the method listen (after a call to bind) or into a client object with the method connect. The only other method supported by a master object is the close method.

 @function [parent=#socket] tcp
 @return #master In case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 A TCP Server object.
 @type server 
]]

--[[------------------------------------------------------------------------------
 Waits for a remote connection on the server object and returns a @{#client} object representing that connection.
 Note: calling @{#(socket).select} with a @{#server} object in the recvt parameter before a call to accept does not guarantee accept will return immediately.
 Use the @{#(socket).settimeout} method or _accept_ might block until _another_ client shows up.

 @function [parent=#server] accept
 @param self
 @return #client  If a connection is successfully initiated.
 @return #nil, #string  Given error string can be:
 
 - **'timeout'**: If a timeout condition is met.
 - Message describing error, for other errors.
]]

--[[------------------------------------------------------------------------------
Closes a TCP object.

 The internal socket used by the object is closed and the local address to which
 the object was bound is made available to other applications. No further
 operations (except for further calls to the close method) are allowed on a
 closed socket.

 Note: It is important to close all used sockets once they are not needed,
 since, in many systems, each socket uses a file descriptor, which are limited
 system resources. Garbage-collected objects are automatically closed before
 destruction, though.
 @function [parent=#server] close
 @param self
]]

--[[------------------------------------------------------------------------------
 Returns the local address information associated to the object.

 @function [parent=#server] getsockname
 @param self
 @return #string Local IP address and a number with the port.
 @return #nil In case of error.
]]

--[[------------------------------------------------------------------------------
 Returns accounting information on the socket, useful for throttling of bandwidth.

 @function [parent=#server] getstats
 @param self
 @return #number, #number, #number The _number_ of bytes received, the _number_
 of bytes sent, and the _age of the socket_ object in seconds.
]]

--[[----------------------------------------------------------------------------
 Sets options for the TCP object.
 
 Options are only needed by low-level or time-critical applications. You should
 only modify an option if you are sure you need it.

 Note: The given descriptions come from the man pages.

 @function [parent=#server] setoption
 @param self
 @param #string option The option name:
 
 - **'keepalive'**: Setting this option to true enables the periodic
  transmission of messages on a connected socket. Should the connected party
  fail to respond to these messages, the connection is considered broken and
  processes using the socket are notified;
 - **'linger'**: Controls the action taken when unsent data are queued on a
  socket and a close is performed. The value is a table with a boolean entry 'on'
  and a numeric entry for the time interval 'timeout' in seconds. If the 'on'
  field is set to true, the system will block the process on the close attempt
  until it is able to transmit the data or until 'timeout' has passed. If 'on'
  is false and a close is issued, the system will process the close in a manner
  that allows the process to continue as quickly as possible. I do not advise
  you to set this to anything other than zero;
 - **'reuseaddr'**: Setting this option indicates that the rules used in
  validating addresses supplied in a call to bind should allow reuse of local
  addresses;
 - **'tcp-nodelay'**: Setting this option to true disables the Nagle's algorithm
  for the connection.
 @param value Depends on the option being set. 
 @return #number 1 in case of success.
 @return #nil Otherwise.
]]

--[[----------------------------------------------------------------------------
 Resets accounting information on the socket, useful for throttling of bandwidth.

 @function [parent=#server] setstats
 @param self
 @param #number received Number with the new number of bytes received.
 @param #number sent Number with the new number of bytes sent.
 @param #number age is the new age in seconds.
 @return #number 1 in case of success.
 @return #nil Otherwise.
]]

--[[----------------------------------------------------------------------------
 Changes the timeout values for the object.

 By default, all I/O operations are blocking. That is, any call to the methods
 @{#client.send}, @{#client.receive}, and @{#server.accept} will block
 indefinitely, until the operation completes. The _settimeout_ method defines a
 limit on the amount of time the I/O methods can block. When a timeout is set
 and the specified amount of time has elapsed, the affected methods give up and
 fail with an error code.

 Note: although timeout values have millisecond precision in LuaSocket, large
  blocks can cause I/O functions not to respect timeout values due to the time
  the library takes to transfer blocks to and from the OS and to and from the
  Lua interpreter. Also, function that accept host names and perform automatic
  name resolution might be blocked by the resolver for longer than the specified
  timeout value.

 Note: The old timeout method is deprecated. The name has been changed for sake
  of uniformity, since all other method names already contained verbs making
  their imperative nature obvious.

 @function [parent=#server] settimeout
 @param self
 @param #number value The amount of time to wait is specified, in seconds.
 The _#nil_ timeout value allows operations to block indefinitely. Negative
 timeout values have the same effect.
 @param #string mode There are two timeout modes and both can be used together
  for fine tuning:

 - **'b'**: block timeout. Specifies the upper limit on the amount of time
  LuaSocket can be blocked by the operating system while waiting for completion
  of any single I/O operation. This is the default mode;
 - **'t'**: total timeout. Specifies the upper limit on the amount of time
  LuaSocket can block a Lua script before returning from a call.
]]

--[[----------------------------------------------------------------------------
 A TCP Master object.
 
 @type master 
]]

--[[------------------------------------------------------------------------------
 As in @{#server.bind}.

 @function [parent=#master] close
 @param self
]]

--[[------------------------------------------------------------------------------
 Attempts to connect a master object to a remote host, transforming it into a
 client object.

 Client objects support methods @{#client.send}, @{#client.receive},
 @{#server.getsockname},  @{#client.getpeername},  @{#server.settimeout}, and
 @{#server.close}.
 
 Note: The function socket.connect is available and is a shortcut for the
 creation of client sockets.

 Note: Starting with LuaSocket 2.0, the settimeout method affects the behavior
 of connect, causing it to return with an error in case of a timeout. If that
 happens, you can still call socket.select with the socket in the sendt table.
 The socket will be writable when the connection is established.

 @function [parent=#master] connect
 @param self
 @param address Can be an IP address or a host name.
 @param port Port must be an integer number in the range [1..64K).
]]

--[[------------------------------------------------------------------------------
 As in @{#server.getsockname}.
 
 @function [parent=#master] getsockname
 @param self 
]]

--[[------------------------------------------------------------------------------
 As in @{#server.getstats}.

 @function [parent=#master] getstats
 @param self
]]

--[[------------------------------------------------------------------------------
 Specifies the socket is willing to receive connections, transforming the object into a server object.
 
 Server objects support the @{#server.accept}, @{#server.getsockname},
 @{server.setoption}, @{#server.settimeout}, and @{#server.close} methods.

 @function [parent=#master] listen
 @param self
 @param backlog Specifies the number of client connections that can be queued waiting for service.
 If the queue is full and another client attempts connection, the connection is refused.
 @return #number 1, in case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 As in @{#server.setstats}.

 @function [parent=#master] setstats
 @param self
 @param #number received
 @param #number sent
 @param #number age
]]

--[[----------------------------------------------------------------------------
 As in @{#server.settimeout}.

 @function [parent=#master] settimeout
 @param self
 @param #number value
 @param #string mode
]]

--[[----------------------------------------------------------------------------
 A TCP Client object.
 
 @type client 
]]

--[[------------------------------------------------------------------------------
 As in @{#server.bind}.

 @function [parent=#client] close
 @param self
]]

--[[------------------------------------------------------------------------------
 Returns information about the remote side of a connected client object.

 Note: It makes no sense to call this method on @{#server} objects.
 
 @function [parent=#client] getpeername
 @param self
 @return #string The IP address of the peer, followed by the port number that peer is using for the connection.
 @return #nil In case of error.
]]

--[[------------------------------------------------------------------------------
 As in @{#server.getsockname}.
 
 @function [parent=#client] getsockname
 @param self 

]]

--[[------------------------------------------------------------------------------
 As in @{#server.getstats}.

 @function [parent=#client] getstats
 @param self
]]

--[[------------------------------------------------------------------------------
 Reads data from a client object, according to the specified read pattern.
 
 Patterns follow the Lua file I/O format, and the difference in performance
 between all patterns is negligible.
 
 **Important note**: This function was changed severely. It used to support
 multiple patterns (but I have never seen this feature used) and now it doesn't
 anymore. Partial results used to be returned in the same way as successful
 results. This last feature violated the idea that all functions should return
 nil on error. Thus it was changed too.

 @function [parent=#client] receive
 @param self
 @param pattern can be any of the following:
 
 - '*a': reads from the socket until the connection is closed. No end-of-line translation is performed;
 - '*l': reads a line of text from the socket. The line is terminated by a LF character (ASCII 10), optionally preceded by a CR character (ASCII 13). The CR and LF characters are not included in the returned line. In fact, all CR characters are ignored by the pattern. This is the default pattern;
 - _@{#number}_: causes the method to read a specified number of bytes from the socket.
 @param #string prefix (optional) An optional string to be concatenated to the beginning of any received data before return.
 @return If successful, the method returns the received pattern.
 @return #nil, #string In case of error, the method returns nil followed by an error message which can be:

 - **'closed'**: In case the connection was closed before the transmission was completed.
 - **'timeout'**: In case there was a timeout during the operation.

  Also, after the error message, the function returns the partial result of the
  transmission.
]]

--[[----------------------------------------------------------------------------
 Sends data through client object.

 The optional arguments i and j work exactly like the standard
 [#string.sub](http://www.lua.org/manual/5.1/manual.html#pdf-string.sub) Lua
 function to allow the selection of a substring to be sent.

 Note: Output is not buffered. For small strings, it is always better to
 concatenate them in Lua (with the '..' operator) and send the result in one
 call instead of calling the method several times.
 
 @function [parent=#client] send
 @param self
 @param data The string to be sent.
 @param #number i (optional) As in [#string.sub](http://www.lua.org/manual/5.1/manual.html#pdf-string.sub).
 @param #number j (optional) As in [#string.sub](http://www.lua.org/manual/5.1/manual.html#pdf-string.sub).
 @return If successful, the method returns the index of the last byte within
  [i, j] that has been sent. Notice that, if i is 1 or absent, this is
  effectively the total number of bytes sent.
 @return #nil, #string, #number In case of error, the method returns nil,
  followed by an error message, followed by the index of the last byte within
  [i, j] that has been sent. You might want to try again from the byte following
  that.
 The error message can be:

 - **'closed'**: In case the connection was closed before the transmission was
  completed.
 - **'timeout'**: In case there was a timeout during the operation.
]]

--[[----------------------------------------------------------------------------
 As in @{#server.setoption}.

 @function [parent=#client] setoption
 @param self
 @param #string option
 @param value
]]

--[[----------------------------------------------------------------------------
 As in @{#server.setstats}.

 @function [parent=#client] setstats
 @param self
 @param #number received
 @param #number sent
 @param #number age
]]

--[[----------------------------------------------------------------------------
 As in @{#server.settimeout}.

 @function [parent=#client] settimeout
 @param self
 @param #number value
 @param #string mode
]]

--[[----------------------------------------------------------------------------
 Shuts down part of a full-duplex connection.

 @function [parent=#client] shutdown
 @param self
 @param #string mode Tells which way of the connection should be shut down and
  can take the value:

 - "both": disallow further sends and receives on the object. This is the default mode;
 - "send": disallow further sends on the object;
 - "receive": disallow further receives on the object.
 @return #number This function returns 1.
]]

--[[----------------------------------------------------------------------------
 Creates and returns an unconnected UDP object.

 @{#unconnected} objects support the @{#unconnected.sendto},
 @{#unconnected.receive}, @{#unconnected.receivefrom},
 @{#unconnected.getsockname}, @{#unconnected.setoption},
 @{#unconnected.settimeout}, @{#unconnected.setpeername},
 @{#unconnected.setsockname}, and @{#unconnected.close}. The 
 @{#unconnected.setpeername} is used to connect the object.

 @function [parent=#socket] udp
 @return In case of success, a new unconnected UDP object returned.
 @return #nil, #string In case of error, nil is returned,
 followed by an error message.
]]

--[[----------------------------------------------------------------------------
 A connected UDP object.
 
 @type connected 
]]

--[[----------------------------------------------------------------------------
 As in @{#unconnected.close}.
 
 @function [parent=#connected] close
 @param self
]]

--[[----------------------------------------------------------------------------
 Retrieves information about the peer associated with a connected UDP object.

 Note: It makes no sense to call this method on unconnected objects.
 
 @function [parent=#connected] getpeername
 @param self
 @return The IP address and port number of the peer.
]]

--[[----------------------------------------------------------------------------
 As in @{#unconnected.getsockname}.
 
 @function [parent=#connected] getsockname
 @param self
]]

--[[----------------------------------------------------------------------------
 As in @{#unconnected.receive}.
 
 @function [parent=#connected] receive
 @param self
 @param size
]]

--[[----------------------------------------------------------------------------
 Sends a datagram to the UDP peer of a connected object.

 Note: In UDP, the send method never blocks and the only way it can fail is if
 the underlying transport layer refuses to send a message to the specified
 address (i.e. no interface accepts the address).
  
 @function [parent=#connected] send
 @param self
 @param datagram A _#string_ with the datagram contents. The maximum datagram
  size for UDP is 64K minus IP layer overhead. However datagrams larger than the
  link layer packet size will be fragmented, which may deteriorate performance
  and/or reliability.
 @return #number 1, in case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 Changes the peer of a UDP object.

 This method turns an unconnected UDP object into a connected UDP object or vice
 versa. Outgoing datagrams will be sent to the specified peer, and datagrams
 received from other peers will be discarded by the OS. Connected UDP objects
 must use the @{unconnected#send} and @{connected#receive} methods instead of
 @{unconnected#sendto} and @{#unconnected.receivefrom}.

 Note: Since the address of the peer does not have to be passed to and from the
 OS, the use of connected UDP objects is recommended when the same peer is used
 for several transmissions and can result in up to 30% performance gains.
 
 @function [parent=#connected] setpeername
 @param self
 @param #string address Can be an IP address or a host name. If address is
  **'*'** and the object is connected, the peer association is removed and the
  object becomes an unconnected object again. In that case, the port argument is
  ignored.
 @param #number port The port number.
 @return #number 1, in case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
  As in @{#unconnected.setoption}.
 
 @function [parent=#connected] setoption
 @param self
 @param value
]]

--[[----------------------------------------------------------------------------
 As in @{#unconnected.settimeout}.

 @function [parent=#connected] settimeout
 @param self
 @param #number value
]]

--[[----------------------------------------------------------------------------
 A unconnected UDP object.
 
 @type unconnected 
]]

--[[----------------------------------------------------------------------------
 Closes a UDP object.

 The internal socket used by the object is closed and the local address to which
 the object was bound is made available to other applications. No further
 operations (except for further calls to the close method) are allowed on a
 closed socket.

 Note: It is important to close all used sockets once they are not needed, since
 , in many systems, each socket uses a file descriptor, which are limited system
 resources. Garbage-collected objects are automatically closed before
 destruction, though.
 
 @function [parent=#unconnected] close
 @param self
]]

--[[----------------------------------------------------------------------------
 Returns the local address information associated to the object.

 Note: UDP sockets are not bound to any address until the
 @{#unconnected.setsockname} or the @{#unconnected.sendto} method is called for
 the first time (in which case it is bound to an ephemeral port and the
 wild-card address).

 @function [parent=#unconnected] getsockname
 @param self
 @return #string With local IP address and a number with the port.
 @return #nil In case of error.
]]

--[[----------------------------------------------------------------------------
 Receives a datagram from the UDP object.
 
 If the UDP object is connected, only datagrams coming from the peer are
 accepted. Otherwise, the returned datagram can come from any host.

 @function [parent=#unconnected] receive
 @param self
 @param #number size optional Specifies the maximum size of the datagram to be
  retrieved. If there are more than size bytes available in the datagram, the
  excess bytes are discarded. If there are less then size bytes available in the
  current datagram, the available bytes are returned. If size is omitted,
  the maximum datagram size is used (which is currently limited by the
  implementation to 8192 bytes).
 @return The received datagram, in case of success.
 @return #nil, #string _#nil_ followed by the string **'timeout'**, in case of
  timeout.
]]

--[[----------------------------------------------------------------------------
 Works exactly as the @{#unconnected.receive} method, except it returns the IP
 address and port as extra return values (and is therefore slightly less
 efficient).

 @function [parent=#unconnected] receivefrom
 @param self
 @param #number size
 @return address, port
]]

--[[----------------------------------------------------------------------------
 Sends a datagram to the specified IP address and port number.

 Note: In UDP, the send method never blocks and the only way it can fail is if
 the underlying transport layer refuses to send a message to the specified
 address (i.e. no interface accepts the address).
 
 @function [parent=#unconnected] sendto
 @param self
 @param datagram A _#string" with the datagram contents. The maximum datagram
  size for UDP is 64K minus IP layer overhead. However datagrams larger than the
  link layer packet size will be fragmented, which may deteriorate performance
  and/or reliability.
 @param ip The IP address of the recipient. Host names are not allowed for
  performance reasons. 
 @param port The port number at the recipient.
 @return #number 1, in case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 As in @{#connected.setpeername}.

 @function [parent=#unconnected] setpeername
 @param self
 @param #string address
 @param #number port
]]

--[[----------------------------------------------------------------------------
 Binds the UDP object to a local address.

 Note: This method can only be called before any datagram is sent through the
 UDP object, and only once. Otherwise, the system automatically binds the object
 to all local interfaces and chooses an ephemeral port as soon as the first
 datagram is sent. After the local address is set, either automatically by the
 system or explicitly by setsockname, it cannot be changed.
 
 @function [parent=#unconnected] setsockname
 @param self
 @param address Can be an IP address or a host name. If address is **'*'** the
  system binds to all local interfaces using the constant INADDR_ANY.
 @param #number port If port is **0**, the system chooses an ephemeral port.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 Sets options for the UDP object.
 
 Options are only needed by low-level or time-critical applications. You should
 only modify an option if you are sure you need it.

 Note: The descriptions above come from the man pages.
 
 @function [parent=#unconnected] setoption
 @param #string option The option name:

 - **'dontroute'**: Setting this option to true indicates that outgoing messages
  should bypass the standard routing facilities;
 - **'broadcast'**: Setting this option to true requests permission to send
  broadcast datagrams on the socket.
 @param value Depends on the option being set.
 @return #number 1, in case of success.
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 Changes the timeout values for the object.
 
 By default, the @{#unconnected.receive} and @{#unconnected.receivefrom}
 operations are blocking. That is, any call to the methods will block
 indefinitely, until data arrives. The _settimeout_ function defines a limit on
 the amount of time the functions can block. When a timeout is set and the
 specified amount of time has elapsed, the affected methods give up and fail
 with an error code.

 Note: In UDP, the @{#connected.send} and @{#unconnected.sendto} methods never
 block (the datagram is just passed to the OS and the call returns immediately).
 Therefore, the _settimeout_ method has no effect on them.

 Note: The old _timeout_ method is deprecated. The name has been changed for
 sake of uniformity, since all other method names already contained verbs making
 their imperative nature obvious.

 @function [parent=#unconnected] settimeout
 @param self
 @param #number value The amount of time to wait, in seconds. The _#nil_ timeout
  value allows operations to block indefinitely. Negative timeout values have
  the same effect.
]]

--[[----------------------------------------------------------------------------
 Name resolution functions return all information obtained from the resolver in a table of the form:

	resolved = {
		name = canonic-name,
		alias = alias-list,
		ip = ip-address-list
	}

Note that the alias list can be empty.
 @type dns
]]

--[[----------------------------------------------------------------------------
 DNS utilities.

 @field [parent=#socket] #dns dns 
]]

--[[----------------------------------------------------------------------------
 The standard host name 

 @function [parent=#dns] gethostname
 @return #string The standard host name for the machine.
]]

--[[----------------------------------------------------------------------------
 Converts from IP address to host name.

 @function [parent=#dns] tohostname
 @param #string address Can be an IP address or host name.
 @return #string The canonic host name of the given _address_, followed by a
  table with all information returned by the resolver. 
 @return #nil, #string In error cases, error string is provided.
]]

--[[----------------------------------------------------------------------------
 Converts from host name to IP address.

 @function [parent=#dns] toip
 @param #string address Can be an IP address or host name.
 @return #string, #table The first IP address found for address, followed by a
  table with all information returned by the resolver.
 @return #nil, #string In error cases, error string is provided.
]]

return nil
