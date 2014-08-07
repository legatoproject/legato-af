--[[----------------------------------------------------------------------------
 FTP (File Transfer Protocol) is a protocol used to transfer files between hosts.

 The ftp namespace offers thorough support to FTP, under a simple interface.
 The implementation conforms to [RFC 959](http://www.ietf.org/rfc/rfc0959.txt).

 High level functions are provided supporting the most common operations. These
 high level functions are implemented on top of a lower level interface. Using
 the low-level interface, users can easily create their own functions to access
 any operation supported by the FTP protocol. For that, check the implementation.

 To really benefit from this module, a good understanding of
 [LTN012, Filters sources and sinks](http://lua-users.org/wiki/FiltersSourcesAndSinks)
 is necessary.

 URLs MUST conform to [RFC 1738](http://www.ietf.org/rfc/rfc1738.txt), that is,
 an URL is a string in the form:
 
	[ftp://][<user>[:<password>]@]<host>[:<port>][/<path>][type=a|i] 

 The following constants in the namespace can be set to control the default
 behavior of the FTP module:
 
 - _PASSWORD_: default anonymous password.
 - _PORT_: default port used for the control connection;
 - _TIMEOUT_: sets the timeout for all I/O operations;
 - _USER_: default anonymous user;
 @module socket.ftp
 @usage
  -- loads the FTP module and any libraries it requires
  local ftp = require("socket.ftp")
]]

--[[----------------------------------------------------------------------------
 Downloads the contents of a URL.

 The get function has two forms. The simple form has fixed functionality: it
 downloads the contents of a URL and returns it as a string. The generic form
 allows a lot more control, as explained below.
    -- call to simple form
    ftp.get(url)

    -- Call to generic form
    ftp.get{
      host = string,
      sink = LTN12 sink,
      argument or path = string,
      [user = string,]
      [password = string]
      [command = string,]
      [port = number,]
      [type = string,]
      [step = LTN12 pump step,]
      [create = function]
    }

 @function [parent=#socket.ftp] get
 @param url URL of content to download as a _#string_ or if the argument of the
  get function is a _#table_, the function expects at least the fields _host,
  sink,_ and one of _argument_ or _path_ (_argument_ takes precedence). _Host_
  is the server to connect to. _Sink_ is the simple
  [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) sink that will
  receive the downloaded data. _Argument_ or _path_ give the target path to the
  resource in the server. The optional arguments are the following:

  -  _user, password_ :User name and password used for authentication. Defaults
   to _"ftp:anonymous@anonymous.org"_;
  - _command_: The FTP command used to obtain data. Defaults to _"retr"_, but
   see example below;
  - _port_: The port to used for the control connection. Defaults to 21;
  - _type_: The transfer mode. Can take values "i" or "a". Defaults to whatever
   is the server default;
  - _step_: [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) pump step
   function used to pass data from the server to the sink. Defaults to the LTN12
   _pump.step_ function;
  - _create_: An optional function to be used instead of @{socket#socket.tcp}
   when the communications socket is created. 
 @return #string If successful, the simple version returns the URL contents as a
  _#string_.
 @return #number If successful, the generic function returns 1.
 @return #nil, #string In case of error, _#nil_ and an error message describing
  the error.
 @usage
-- load the ftp support
local ftp = require("socket.ftp")

-- Log as user "anonymous" on server "ftp.tecgraf.puc-rio.br",
-- and get file "lua.tar.gz" from directory "pub/lua" as binary.
f, e = ftp.get("ftp://ftp.tecgraf.puc-rio.br/pub/lua/lua.tar.gz;type=i")
 @usage
-- load needed modules
local ftp = require("socket.ftp")
local ltn12 = require("ltn12")
local url = require("socket.url")

-- a function that returns a directory listing
function nlst(u)
    local t = {}
    local p = url.parse(u)
    p.command = "nlst"
    p.sink = ltn12.sink.table(t)
    local r, e = ftp.get(p)
    return r and table.concat(t), e
end
]]

--[[----------------------------------------------------------------------------
 Uploads a string of content into a URL.

 @function [parent=#socket.ftp] put
 @param #string url Where to upload content.
 @param #string content Content to upload.
 @return #number 1 If successful
 @return #nil, #string In case of error, _#nil_ and an error message describing
  the error.
 @usage
-- load the ftp support
local ftp = require("socket.ftp")

-- Log as user "fulano" on server "ftp.example.com",
-- using password "silva", and store a file "README" with contents 
-- "wrong password, of course"
f, e = ftp.put("ftp://fulano:silva@ftp.example.com/README", 
    "wrong password, of course") 
]]

--[[----------------------------------------------------------------------------
 Uploads a string of content into a URL.

 @function [parent=#socket.ftp] put
 @param #table table The function expects at least the fields _host, source_,
  and one of _argument_ or _path_ (_argument_ takes precedence). _Host_ is the
  server to connect to. _Source_ is the simple
  [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) source that will
  provide the contents to be uploaded. _Argument_ or _path_ give the target path
  to the resource in the server. The optional arguments are the following:

  - _user, password_: User name and password used for authentication. Defaults
   to _"ftp:anonymous@anonymous.org"_;
  - _command_: The FTP command used to send data. Defaults to _"stor"_, but see
   example below;
  - _port_: The port to used for the control connection. Defaults to 21;
  - _type_: The transfer mode. Can take values _"i"_ or _"a"_. Defaults to
   whatever is the server default;
  - _step_: [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) pump step
   function used to pass data from the server to the sink. Defaults to the LTN12
   _pump.step_ function;
  - _create_: An optional function to be used instead of @{socket#socket.tcp}
   when the communications socket is created. 
 @return #number 1 If successful
 @return #nil, #string In case of error, _#nil_ and an error message describing
  the error.
 @usage
-- load the ftp support
local ftp = require("socket.ftp")
local ltn12 = require("ltn12")

-- Log as user "fulano" on server "ftp.example.com",
-- using password "silva", and append to the remote file "LOG", sending the
-- contents of the local file "LOCAL-LOG"
f, e = ftp.put{
  host = "ftp.example.com", 
  user = "fulano",
  password = "silva",
  command = "appe",
  argument = "LOG",
  source = ltn12.source.file(io.open("LOCAL-LOG", "r"))
}
]]

return nil
