--[[----------------------------------------------------------------------------
 The ltn12 namespace implements the ideas described in [LTN012, Filters sources and sinks](http://lua-users.org/wiki/FiltersSourcesAndSinks).

 This manual simply describes the functions. Please refer to the LTN for a
 deeper explanation of the functionality provided by this module.
 
 @module ltn12
 @usage
-- loads the LTN12 module
local ltn12 = require("ltn12")
]]

--[[----------------------------------------------------------------------------
 LTN12 implementation.
 
 @type ltn12
 @field #filter	filter	Contains @{#filter} related functions.
 @field #pump	pump	Contains @{#pump} related functions.
 @field #sink	sink	Contains @{#sink} related functions.
 @field #source	source	Contains @{#source} related functions.
]]

--[[----------------------------------------------------------------------------
 Provides filters.

 @type filter
]]

--[[----------------------------------------------------------------------------
 Returns a filter that passes all data it receives through each of a series of given filters.

 The nesting of filters can be arbitrary. For instance, the useless filter below
 doesn't do anything but return the data that was passed to it, unaltered.
 
 @function [parent=#filter] chain
 @param ... Simple filters.
 @return The chained filter.
 @usage
-- load required modules
local ltn12 = require("ltn12")
local mime = require("mime")

-- create a silly identity filter
id = ltn12.filter.chain(
  mime.encode("quoted-printable"),
  mime.encode("base64"),
  mime.decode("base64"),
  mime.decode("quoted-printable")
)
]]

--[[----------------------------------------------------------------------------
 Returns a high-level filter that cycles though a low-level filter by passing it each chunk and updating a context between calls.

 @function [parent=#filter] cycle
 @param low The low-level filter to be cycled.
 @param ctx (optional) The initial context.
 @param extra (optional) Any extra argument the low-level filter might take.
 @return The high-level filter.
 @usage
-- load the ltn12 module
local ltn12 = require("ltn12")

-- the base64 mime filter factory
encodet['base64'] = function()
    return ltn12.filter.cycle(b64, "")
end
]]

--[[----------------------------------------------------------------------------
 Provide pumps.

 @type pump
]]

--[[----------------------------------------------------------------------------
 Pumps _all_ data from a _source_ to a _sink_.
 
 @function [parent=#pump] all
 @param #source source Source to pump.
 @param #sink sink Sink to fill.
 @return If successful, the function returns a value that evaluates to **true**.
 @return #nil, #string In case of error, the function returns a **false** value,
  followed by an error message.
]]

--[[----------------------------------------------------------------------------
 Pumps _one_ chunk of data from a _source_ to a _sink_. 
 
 @function [parent=#pump] step
 @param #source source Source to pump.
 @param #sink sink Sink to fill.
 @return If successful, the function returns a value that evaluates to **true**.
 @return #nil, #string In case of error, the function returns a **false** value,
  followed by an error message.
]]

--[[----------------------------------------------------------------------------
 Provides sinks.

 @type sink
]]

--[[----------------------------------------------------------------------------
 Creates and returns a new sink that passes data through a filter before sending it to a given sink. 
 
 @function [parent=#sink] chain
 @param #filter filter
 @param #sink sink
 @retrun A new @{#sink}.
]]

--[[----------------------------------------------------------------------------
 Creates and returns a sink that aborts transmission with the error _message_.
 
 @function [parent=#sink] error
 @param #string message
 @return #sink A @{#sink} that aborts transmission with the error _message_.
]]

--[[----------------------------------------------------------------------------
 Creates a sink that sends data to a file.
 
 @function [parent=#sink] file
 @param handle A file handle. If _handle_ is _#nil_, _message_ should give the
  reason for failure. 
 @param #string message The reason for failure, when _handle_ is _#nil_.
 @return A @{#sink} that sends all data to the given _handle_ and closes the
  file when done, or a sink that aborts the transmission with the error
  _message_.
 @usage
-- load the ltn12 module
local ltn12 = require("ltn12")

-- copy a file
ltn12.pump.all(
  ltn12.source.file(io.open("original.png")),
  ltn12.sink.file(io.open("copy.png"))
)
]]

--[[----------------------------------------------------------------------------
 Returns a sink that ignores all data it receives. 

 @function [parent=#sink] null
 @return A @{#sink} that ignores all data it receives.
]]

--[[----------------------------------------------------------------------------
 Creates and returns a simple sink given a fancy @{#sink}.

 @function [parent=#sink] simplify
 @param #sink sink Fancy _sink_.
 @return #sink A simple @{#sink}.
]]

--[[----------------------------------------------------------------------------
 Creates a sink that stores all chunks in a table.
 
 The chunks can later be efficiently concatenated into a single string. 
 @function [parent=#sink] table
 @param #table table (optional) Used to hold the chunks. If _#nil_, the function
  creates its own table.
 @return #sink, #table The sink and the table used to store the chunks.
 @usage
-- load needed modules
local http = require("socket.http")
local ltn12 = require("ltn12")

-- a simplified http.get function
function http.get(u)
  local t = {}
  local respt = request{
    url = u,
    sink = ltn12.sink.table(t)
  }
  return table.concat(t), respt.headers, respt.code
end
]]

--[[----------------------------------------------------------------------------
 Provides sources.

 @type source
]]

--[[----------------------------------------------------------------------------
 Creates a new source that produces the concatenation of the data produced by a number of sources.

 @function [parent=#source] cat
 @param ... The original sources.
 @return #source The new @{#source}.
]]

--[[----------------------------------------------------------------------------
 Creates a new _source_ that passes data through a _filter_ before returning it. 

 @function [parent=#source] chain
 @param #source source Passes data through a _filter_ before returning it.
 @param #filter filter 
 @return #source The new @{#source}.
]]

--[[----------------------------------------------------------------------------
 Creates and returns an empty source. 

 @function [parent=#source] empty
 @return #source An empty @{#source}.
]]

--[[----------------------------------------------------------------------------
 Creates and returns a source that aborts transmission with the error _message_.

 @function [parent=#source] error
 @param #string message
 @return #source A @{#source} that aborts transmission with the error message.
]]

--[[----------------------------------------------------------------------------
 Creates a source that produces the contents of a file. 

 In the following example, notice how the prototype is designed to fit nicely
 with the _io.open_ function.
	-- load the ltn12 module
	local ltn12 = require("ltn12")
	
	-- copy a file
	ltn12.pump.all(
	  ltn12.source.file(io.open("original.png")),
	  ltn12.sink.file(io.open("copy.png"))
	)
 
 @function [parent=#source] file
 @param handle A file handle. If _handle_ is _#nil_, _message_ should give the
  reason for failure.
 @param #string message Gives the reason for failure, when _handle_ is _#nil_.
 @return #source A @{#source} that reads chunks of data from given _handle_ and
  returns it to the user, closing the file when done, or a source that aborts
  the transmission with the error _message_.
]]

--[[----------------------------------------------------------------------------
 Creates and returns a simple source given a fancy source.

 @function [parent=#source] simplify
 @param #source source A fancy @{#source}.
 @return #source A simple @{#source}.
]]

--[[----------------------------------------------------------------------------
 Creates and returns a source that produces the contents of a _string_, chunk by
 chunk.
 
 @function [parent=#source] string
 @param #string string Content of given @{#source}.
 @return #source A @{#source} that produces the contents of given _string_,
  chunk by chunk.
]]
return nil
