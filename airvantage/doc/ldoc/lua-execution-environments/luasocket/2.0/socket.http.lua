--[[----------------------------------------------------------------------------
 HTTP (Hyper Text Transfer Protocol) is the protocol used to exchange information between web-browsers and servers.

 The http namespace offers full
 support for the client side of the HTTP protocol (i.e., the facilities that
 would be used by a web-browser implementation). The implementation conforms to
 the HTTP/1.1 standard, [RFC 2616](http://www.ietf.org/rfc/rfc2616.txt).

 The module exports functions that provide HTTP functionality in different
 levels of abstraction. From the simple string oriented requests, through
 generic [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) based, down
 to even lower-level if you bother to look through the source code.

 URLs must conform to [RFC 1738](http://www.ietf.org/rfc/rfc1738.txt), that is,
 an URL is a string in the form:
	[http://][<user>[:<password>]@]<host>[:<port>][/<path>] 

 MIME headers are represented as a Lua table in the form:
	headers = {
		field-1-name = field-1-value,
		field-2-name = field-2-value,
		field-3-name = field-3-value,
		...
		field-n-name = field-n-value
	}

 Field names are case insensitive (as specified by the standard) and all
 functions work with lowercase field names. Field values are left unmodified.

 Note: MIME headers are independent of order. Therefore, there is no problem in
 representing them in a Lua table.

 The following constants can be set to control the default behavior of the HTTP
 module:

 - _PORT_: default port used for connections;
 - _PROXY_: default proxy used for connections;
 - _TIMEOUT_: sets the timeout for all I/O operations;
 - _USERAGENT_: default user agent reported to server.

 @module socket.http
 @usage
-- loads the HTTP module and any libraries it requires
local http = require("socket.http")
]]

--[[----------------------------------------------------------------------------
 The request function has two forms.
 
 The simple form downloads a URL using the _GET_ or _POST_ method and is based
 on strings. The generic form performs any HTTP method and
 [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) based.

 Note: When sending a POST request, simple interface adds a
 _"Content-type: application/x-www-form-urlencoded"_ header to the request.
 This is the type used by HTML forms. If you need another type, use the generic
 interface.

 Note: Some URLs are protected by their servers from anonymous download.
 For those URLs, the server must receive some sort of authentication along with
 the request or it will deny download and return status
 "401 Authentication Required".

 The HTTP/1.1 standard defines two authentication methods: the Basic
 Authentication Scheme and the Digest Authentication Scheme,
 both explained in detail in [RFC 2068](http://www.ietf.org/rfc/rfc2068.txt).

 The Basic Authentication Scheme sends _<user>_ and _<password>_ unencrypted to
 the server and is therefore considered unsafe. Unfortunately, by the time of
 this implementation, the wide majority of servers and browsers support the
 Basic Scheme only. Therefore, this is the method used by the toolkit whenever
 authentication is required.

	-- load required modules
	http = require("socket.http")
	mime = require("mime")
	
	-- Connect to server "www.example.com" and tries to retrieve
	-- "/private/index.html", using the provided name and password to
	-- authenticate the request
	b, c, h = http.request("http://fulano:silva@www.example.com/private/index.html")
	
	-- Alternatively, one could fill the appropriate header and authenticate
	-- the request directly.
	r, c = http.request {
	  url = "http://www.example.com/private/index.html",
	  headers = { authentication = "Basic " .. (mime.b64("fulano:silva")) }
	}

 @function [parent=#socket.http] request
 @param  url If the first argument of the request function is a _#string_,
  it should be an url. If the first argument is instead a _#table_, the most
  important fields are the url and the simple
  [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) _sink_ that will
  receive the downloaded content. Any part of the url can be overridden by
  including the appropriate field in the request table. If authentication
  information is provided, the function uses the Basic Authentication Scheme
  to retrieve the document. If _sink_ is _#nil_, the function discards the
  downloaded data. The optional parameters are the following:

  - _method_: The HTTP request method. Defaults to "GET";
  - _headers_: Any additional HTTP headers to send with the request;
  - _source_: simple LTN12 source to provide the request body. If there is a
   body, you need to provide an appropriate _"content-length"_ request header
   field, or the function will attempt to send the body as _"chunked"_
   (something few servers support). Defaults to the empty source;
  - _step_: [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) pump step
   function used to move data. Defaults to the LTN12 _pump.step_ function.
  - _proxy_: The URL of a proxy server to use. Defaults to no proxy;
  - _redirect_: Set to **false** to prevent the function from automatically
   following 301 or 302 server redirect messages;
  - _create_: An optional function to be used instead of @{socket#socket.tcp}
   when the communications socket is created. 
 @param #string body (optional) If provided as a string and if the first
  argument of the request function is a string, the function will perform a
  _POST_ method in the url. Otherwise, it performs a _GET_ in the _url_.
 @return #nil, #string In case of failure, the function returns nil followed by
  an error message.
 @return #string, #number, #string, #string If successful, the simple form
  returns the response body as a string, followed by the response status code,
  the response headers and the response status line. The generic function
  returns the same information, except the first return value is just the number
  1 (the body goes to the sink).

  Even when the server fails to provide the contents of the requested URL
  (URL not found, for example), it usually returns a message body (a web page
  informing the URL was not found or some other useless page). To make sure the
  operation was successful, check the returned status _code_. For a list of the
  possible values and their meanings, refer to
  [RFC 2616](http://www.ietf.org/rfc/rfc2616.txt). 
 @usage
-- load the http module
local io = require("io")
local http = require("socket.http")
local ltn12 = require("ltn12")

-- connect to server "www.cs.princeton.edu" and retrieves this manual
-- file from "~diego/professional/luasocket/http.html" and print it to stdout
http.request{ 
    url = "http://www.cs.princeton.edu/~diego/professional/luasocket/http.html", 
    sink = ltn12.sink.file(io.stdout)
}

-- connect to server "www.example.com" and tries to retrieve
-- "/private/index.html". Fails because authentication is needed.
b, c, h = http.request("http://www.example.com/private/index.html")
-- b returns some useless page telling about the denied access, 
-- h returns authentication information
-- and c returns with value 401 (Authentication Required)

-- tries to connect to server "wrong.host" to retrieve "/"
-- and fails because the host does not exist.
r, e = http.request("http://wrong.host/")
-- r is nil, and e returns with value "host not found"

-- load the http module
http = require("socket.http")

-- Requests information about a document, without downloading it.
-- Useful, for example, if you want to display a download gauge and need
-- to know the size of the document in advance
r, c, h = http.request {
  method = "HEAD",
  url = "http://www.tecgraf.puc-rio.br/~diego"
}
-- r is 1, c is 200, and h would return the following headers:
-- h = {
--   date = "Tue, 18 Sep 2001 20:42:21 GMT",
--   server = "Apache/1.3.12 (Unix)  (Red Hat/Linux)",
--   ["last-modified"] = "Wed, 05 Sep 2001 06:11:20 GMT",
--   ["content-length"] = 15652,
--   ["connection"] = "close",
--   ["content-Type"] = "text/html"
-- }
]]

return nil
