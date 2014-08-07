--[[----------------------------------------------------------------------------
 The url namespace provides functions to parse, protect, and build URLs, as well as functions to compose absolute URLs from base and relative URLs, according to [RFC 2396](http://www.ietf.org/rfc/rfc2396.txt).

 To obtain the url namespace, run:

	-- loads the URL module 
	local url = require("socket.url")

 An URL is defined by the following grammar:

	<url> ::= [<scheme>:][//<authority>][/<path>][;<params>][?<query>][#<fragment>]
	<authority> ::= [<userinfo>@]<host>[:<port>]
	<userinfo> ::= <user>[:<password>]
	<path> ::= {<segment>/}<segment>
 @module socket.url

]]

--[[0
]]

--[[----------------------------------------------------------------------------
 Builds an absolute URL from a base URL and a relative URL.

 Note: The rules that govern the composition are fairly complex, and are
 described in detail in [RFC 2396](http://www.ietf.org/rfc/rfc2396.txt). The
 example bellow should give an idea of what the rules are.

	http://a/b/c/d;p?q
	
	+
	
	g:h      =  g:h
	g        =  http://a/b/c/g
	./g      =  http://a/b/c/g
	g/       =  http://a/b/c/g/
	/g       =  http://a/g
	//g      =  http://g
	?y       =  http://a/b/c/?y
	g?y      =  http://a/b/c/g?y
	#s       =  http://a/b/c/d;p?q#s
	g#s      =  http://a/b/c/g#s
	g?y#s    =  http://a/b/c/g?y#s
	;x       =  http://a/b/c/;x
	g;x      =  http://a/b/c/g;x
	g;x?y#s  =  http://a/b/c/g;x?y#s
	.        =  http://a/b/c/
	./       =  http://a/b/c/
	..       =  http://a/b/
	../      =  http://a/b/
	../g     =  http://a/b/g
	../..    =  http://a/
	../../   =  http://a/
	../../g  =  http://a/g

 @function [parent=#socket.url] absolute
 @param #string base The base URL or a parsed URL table.
 @param #string relative The relative URL.
 @return #string The absolute URL. 

]]

--[[----------------------------------------------------------------------------
 Rebuilds an URL from its parts.

 @function [parent=#socket.url] build
 @param #table parsed_url With same components returned by
 @{#(socket.url).parse}. Lower level components, if specified, take precedence
  over high level components of the URL grammar.
 @return #string The built URL.

]]

--[[----------------------------------------------------------------------------
 Builds a _<path>_ component from a list of _<segment>_ parts.

 Before composition, any reserved characters found in a segment are escaped into
 their protected form, so that the resulting path is a valid URL path component.

 @function [parent=#socket.url] build_path
 @param #table segments A list of strings with the _<segment>_ parts.
 @param unsafe If is anything but nil, reserved characters are left untouched.
 @return #string With the built _<path>_ component. 

]]

--[[----------------------------------------------------------------------------
 Applies the URL escaping content coding to a string each byte is encoded as a percent character followed by the two byte hexadecimal representation of its integer value.

 @function [parent=#socket.url] escape
 @param #string content The _#string_ to be encoded.
 @return #string The encoded string.
 @usage
-- load url module
url = require("socket.url")

code = url.escape("/#?;")
-- code = "%2f%23%3f%3b"

]]

--[[----------------------------------------------------------------------------
 Parses an URL given as a string into a Lua table with its components.

 @function [parent=#socket.url] parse
 @param #string url The URL to be parsed.
 @param #table default If present, it is used to store the parsed fields. Only
  fields present in the URL are overwritten. Therefore, this table can be used
  to pass default values for each field.
 @return #table All the URL components:
	parsed_url = {
	  url = string,
	  scheme = string,
	  authority = string,
	  path = string,
	  params = string,
	  query = string,
	  fragment = string,
	  userinfo = string,
	  host = string,
	  port = string,
	  user = string,
	  password = string
	}
 @usage
-- load url module
url = require("socket.url")

parsed_url = url.parse("http://www.example.com/cgilua/index.lua?a=2#there")
-- parsed_url = {
--   scheme = "http",
--   authority = "www.example.com",
--   path = "/cgilua/index.lua"
--   query = "a=2",
--   fragment = "there",
--   host = "www.puc-rio.br",
-- }

parsed_url = url.parse("ftp://root:passwd@unsafe.org/pub/virus.exe;type=i")
-- parsed_url = {
--   scheme = "ftp",
--   authority = "root:passwd@unsafe.org",
--   path = "/pub/virus.exe",
--   params = "type=i",
--   userinfo = "root:passwd",
--   host = "unsafe.org",
--   user = "root",
--   password = "passwd",
-- }

]]

--[[----------------------------------------------------------------------------
 Breaks a _<path>_ URL component into all its _<segment>_ parts.

 @function [parent=#socket.url] parse_path
 @param #string path With the path to be parsed.
 @return Since some characters are reserved in URLs, they must be escaped
  whenever present in a <path> component. Therefore, before returning a list
  with all the parsed segments, the function removes escaping from all of them. 

]]

--[[----------------------------------------------------------------------------
 Removes the URL escaping content coding from a string.

 @function [parent=#socket.url] unescape
 @param #string content The string to be decoded.
 @return #string The decoded string. 

]]

return nil
