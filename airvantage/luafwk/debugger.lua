-------------------------------------------------------------------------------
-- Copyright (c) 2011 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Julien Desgats for Sierra Wireless - initial API and implementation
--     Simon Bernard  for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------
-- Debugger using DBGp protocol.
-------------------------------------------------------------------------------
-- The module returns a single init function which takes 3 parameters (HOST, PORT, IDEKEY).

-- HOST: the host name or the ip address of the DBGP server (so your ide)
-- if HOST is nil, the DBGP_IDEHOST env var is used.
-- if the env var is nil, the default value '127.0.0.1' is used.

-- PORT: the port of the DBGP server (must be configure in the IDE)
-- if PORT is nil, the DBGP_IDEPORT env var is used.
-- if the env var is nil, the default value '10000' is used.

-- IDEKEY: a string which is used as session key
-- if IDEKEY is nil, the DBGP_IDEKEY env var is used.
-- if the env var is nil, the default value 'luaidekey' is used.

-------------------------------------------------------------------------------
-- Known Issues:
--   * Functions cannot be created using the debugger and then called in program because their environment is mapped directly to
--     a debugger internal structure which cannot be persisted (i.e. used outside of the debug_hook).
--   * The DLTK client implementation does not handle context for properties. As a workaround, the context is encoded into the
--     fullname attribute of each property and is used likewise in property_get commands. The syntax is "<context ID>|<full name>"
--   * Dynamic code (compiled with load or loadstring) is not handled (the debugger will step over it, like C code)
-- Design notes:
--   * The whole debugger state is kept in a (currently) unique session table in order to ease eventual adaptation to a multi-threaded
--     model, as DBGp needs one connection per thread.
--   * Full names of properties are base64 encoded because they can contain arbitrary data (spaces, escape characters, ...), this makes
--     command parsing munch easier and faster
--   * This debugger supports asynchronous commands: any command can be done at any time, but some of them (continuations) can lead to
--     inconsistent states. In addition, this have a quite big overhead (~66%), if performance is an issue, a custom command to disable
--     async mode could be done.
--   * All commands are implemented in table commands, see this comments on this table to additional details about commands implementation
--   * The environments in which are evaluated user code (property_* and eval commands, conditional breakpoints, ...) is a read/write
--     mapping of the local environment of a given stack level (can be accessed with variable names). See Context for additional details.
--     Context instantiation is pooled inside a debugging loop with ContextManager (each stack level is instantiated only once).
--   * Output redirection is done by redefining print and some values inside the io table. See "Output redirection handling" for details.

-- The duebugger needs access to the real socket functions, not those overwritten
-- by sched. Indeed, the debugger wants to freeze the whole VM, not only one thread.
-- TODO: a cleaner access to original functions should be kept available by sched.
local blockingtcp = {}
do
    local socketcore = require"socket.core"
    local reg = debug.getregistry()
    blockingtcp.sleep      = socketcore.sleep
    blockingtcp.connect    = reg["tcp{master}"].__index.connect
    blockingtcp.receive    = reg["tcp{client}"].__index.receive
    blockingtcp.send       = reg["tcp{client}"].__index.send
    blockingtcp.settimeout = reg["tcp{master}"].__index.settimeout

    -- verify that the socket function are the real ones rather than Mihini sched's non-blocking versions
    assert(debug.getinfo(blockingtcp.connect, "S").what == "C", "The debugger needs the real socket functions !")
    -- cleanup the package.loaded table (socket.core adds socket field into it)
    package.loaded.socket = nil
end
-- TODO as of 07/20/2011, the socket module packaged with ready agent requires sched to be loaded before
pcall(require, "sched")
local socket = require"socket"
local url = require"socket.url"

local mime = require"mime"
local ltn12 = require"ltn12"

local log
do
    -- make a bare minimal log system
    local LEVELS = { ERROR = 0, WARNING = 1, INFO = 2, DETAIL = 3, DEBUG = 4 }
    local LOG_LEVEL = 1 -- log only errors and warnings
    log = function(mod, level, msg, ...)
        if (LEVELS[level] or -1) > LOG_LEVEL then return end
        if select("#", ...) > 0 then msg = msg:format(...) end
        io.base.stderr:write(string.format("%s\t%s\t%s\n", mod, level, msg))
    end
end

-- TODO complete the stdlib access
local debug_getinfo, corunning, cocreate, cowrap, coyield, coresume, costatus = debug.getinfo, coroutine.running, coroutine.create, coroutine.wrap, coroutine.yield, coroutine.resume, coroutine.status

-- correct function debug.getinfo behavior regarding first argument (sometimes nil in our case)
local function getinfo(coro, level, what)
    if coro then return debug_getinfo(coro, level, what)
    else return debug_getinfo(level + 1, what) end
end

-- "BEGIN PLATFORM DEPENDENT CODE"
-- Get the execution plaform os (could be win or unix)
-- Used to manage file path difference between the 2 platform
local platform = "unix"
do
    local function iswindows()
        local p = io.popen("echo %os%")
        if p then
            local result =p:read("*l")
            p:close()
            return result == "Windows_NT"
        end
        return false
    end
    local function setplatform()
        if iswindows() then
            platform = "win"
        end
    end
    pcall(setplatform)
end

-- The Path separator character
local path_sep = (platform == "unix" and "/" or "\\")


-- return true is the path is absolute
-- the path must be normalized
-- test if it begin by / for unix and X:/ for windows
local is_path_absolute
if platform == "unix" then
   is_path_absolute = function (path) return path:sub(1,1) == "/" end
else
   is_path_absolute = function (path) return path:match("^%a:/") end
end

-- return a clean path with / as separator (without double path separator)
local normalize
if platform == "unix" then
   normalize = function (path) return path:gsub("//","/") end
else
   normalize = function (path)
        return path:gsub("\\","/"):gsub("//","/"):lower()
   end
end

-- TODO the way to get the absolute path can be wrong if the program loads new source files by relative path after a cd.
-- currently, the directory is registered on start, this allows program to load any source file and then change working dir,
-- which is the most common use case.
local base_dir
if platform == "unix" then
   base_dir = os.getenv("PWD")
else
   local p = io.popen("echo %cd%")
   if p then
       base_dir = p:read("*l")
       base_dir = normalize(base_dir)
       p:close()
   end
end
if not base_dir then error("Unable to determine the working directory.")end


-- parse a normalized path and return a table of each segment
-- you could precise the path seperator.
local function split(path,sep)
  local t = {}
  for w in string.gfind(path, "[^"..sep.."]+")do
    table.insert(t, w)
  end
  return t
end

-- merge an asbolute and a relative path in an absolute path
-- you could precise the path seperator, '/' is the default one.
local function merge_paths(absolutepath, relativepath,separator)
  local sep = separator or "/"
  local absolutetable = split(absolutepath,sep)
  local relativetable = split(relativepath,sep)
  for i, path in ipairs(relativetable) do
    if path == ".." then
      table.remove(absolutetable, table.getn(absolutetable))
    elseif path ~= "." then
      table.insert(absolutetable, path)
    end
  end
  return sep .. table.concat(absolutetable, sep)
end


-- convert absolute normalized path file to uri
local to_file_uri
if platform == "unix" then
   to_file_uri = function (path) return url.build{scheme="file",authority="", path=path} end
else
   to_file_uri = function (path) return url.build{scheme="file",authority="", path="/"..path} end
end

-- convert parsed URL table to file path  for the current OS (see url.parse from luasocket)
local to_path
if platform == "unix" then
   to_path = function (url) return url.path end
else
   to_path = function (url) return url.path:gsub("^/", "") end
end
-- "END PLATFORM DEPENDENT CODE"


-- register the URI of the debugger, to not jump into with redefined function or coroutine bootstrap stuff
local debugger_uri = nil -- set in init function

-- will contain the session object, and eventually a list of all sessions if a multi-threaded model is adopted
-- this is only used for async commands.
local active_session = nil

-- tracks all active coroutines and associate an id to them, the table from_id is the id=>coro mapping, the table from_coro is the reverse
local active_coroutines = { n = 0, from_id = setmetatable({ }, { __mode = "v" }), from_coro = setmetatable({ }, { __mode = "k" }) }

-------------------------------------------------------------------------------
--  Debugger features & configuration
-------------------------------------------------------------------------------

local constant_features = {
    language_supports_threads = 0,
    language_name = "Lua",
    language_version = _VERSION,
    protocol_version = 1,
    supports_async = 1,
    data_encoding = "base64",
    breakpoint_languages = "Lua",
    breakpoint_types = "line conditional",
}

local variable_features = setmetatable({ }, {
    -- functions that format/validate data (defaults to tostring)
    validators = {
        multiple_sessions = tonumber,
        max_children = tonumber,
        max_data = tonumber,
        max_depth = tonumber,
        show_hidden = tonumber,
    },
    __index = {
        multiple_sessions = 0,
        encoding ="UTF-8",
        max_children = 32,
        max_data = 0xFFFF,
        max_depth = 1,
        show_hidden = 1,
        uri = "file"
    },
    __newindex = function(self, k, v)
        local mt = getmetatable(self)
        local values, validators = mt.__index, mt.validators
        if values[k] == nil then error("No such feature " .. tostring(k)) end
        v = (validators[k] or tostring)(v)
        values[k] = v
    end,
})

-------------------------------------------------------------------------------
--  Utility functions
-------------------------------------------------------------------------------

--- Encodes a string into Base64 with line wrapping
-- @param data (string) data to encode
-- @return base64 encoded string
local function b64(data)
    local filter = ltn12.filter.chain(mime.encode("base64"), mime.wrap("base64"))
    local sink, output = ltn12.sink.table()
    ltn12.pump.all(ltn12.source.string(data), ltn12.sink.chain(filter, sink))
    return table.concat(output)
end

--- Encodes a string into Base64, without any extra parsing (wrapping, ...)
-- @param data (string) data to encode
-- @return decoded string
local function rawb64(data)
    return (mime.b64(data)) -- first result of the low-level function is fine here
end

--- Decodes base64 data
-- @param data (string) base64 encoded data
-- @return decoded string
local function unb64(data)
    return (mime.unb64(data)) -- first result of the low-level function is fine here
end

--- Parses the DBGp command arguments and returns it as a Lua table with key/value pairs.
-- For example, the sequence <code>-i 5 -j foo</code> will result in <code>{i=5, j=foo}</code>
-- @param cmd_args (string) sequence of arguments
-- @return table described above
local function arg_parse(cmd_args)
    local args = {}
    for arg, val in cmd_args:gmatch("%-(%w) (%S+)") do
        args[arg] = val
    end
    return args
end

--- Parses a command line
-- @return commande name (string)
-- @retrun arguments (table)
-- @return data (string, optional)
local function cmd_parse(cmd)
    local cmd_name, args, data
    if cmd:find("--", 1, true) then -- there is a data part
        cmd_name, args, data = cmd:match("^(%S+)%s+(.*)%s+%-%-%s*(.*)$")
    else
        cmd_name, args, data = cmd:match("^(%S+)%s+(.*)$")
    end
    return cmd_name, arg_parse(args), unb64(data)
end

--- Returns the packet read from socket, or nil followed by an error message on errors.
local function read_packet(skt)
    local size = {}
    while true do
        local byte, err = blockingtcp.receive(skt, 1)
        if not byte then return nil, err end
        if byte == "\000" then break end
        size[#size+1] = byte
    end
    return table.concat(size)
end

-- get uri from source debug info
local get_uri
local get_path
do
    local cache = { }
    --- Returns a RFC2396 compliant URI for given source, or false if the mapping failed
    local function get_abs_file_uri (source)
      local uri
      if source:sub(1,1) == "@" then -- real source file
        local sourcepath = source:sub(2)
        local normalizedpath = normalize(sourcepath)
        if not is_path_absolute(normalizedpath) then
          normalizedpath = merge_paths(base_dir, normalizedpath)
        end
        return to_file_uri(normalizedpath)
      else -- dynamic code, stripped bytecode, tail return, ...
        return false
      end
    end

    local function get_module_uri (source)
      local uri
      if source:sub(1,1) == "@" then -- real source file
        local sourcepath = source:sub(2)
        local normalizedpath = normalize(sourcepath)
        local normalizedluapath = normalize(package.path)
        local luapathtable = split (normalizedluapath,";")
        -- workarround : Add always the ?.lua entry to support
        -- the case where file was loaded by : "lua myfile.lua"
        table.insert(luapathtable,"?.lua")
        for i,var in ipairs(luapathtable) do
          local escaped = string.gsub(var,"[%^%$%(%)%%%.%[%]%*%+%-%?]",function(c) return "%"..c end)
          local pattern = string.gsub(escaped,"%%%?","(.+)")
          local modulename = string.match(normalizedpath,pattern);
          if modulename then
            modulename = string.gsub(modulename,"/",".");
            -- if we find more than 1 possible modulename return the shorter
            if not uri or string.len(uri)>string.len(modulename) then
              uri = modulename
            end
          end
        end
        if uri then return "module:///"..uri end
      end
      return false
    end

    get_uri = function (source)
      -- search in cache
      local uri = cache[source]
      if uri ~= nil then return uri end

      -- not found, create uri
      if variable_features.uri == "module" then
         uri = get_module_uri(source)
         if not uri then uri = get_abs_file_uri (source) end
      else
         uri =  get_abs_file_uri (source)
      end

      cache[source] = uri
      return uri
    end

  -- get path file from uri
  get_path=  function (uri)
      local parsed_path = assert(url.parse(uri))
      if parsed_path.scheme == "file" then
         return to_path(parsed_path)
      else
         -- search in cache
         -- we should surely calculate it instead of find in cache
         for k,v in pairs(cache)do
           if v == uri then
           assert(k:sub(1,1) == "@")
           return k:sub(2)
           end
         end
      end
   end
end

-- Used to store complex keys (other than string and number) as they cannot be passed in text
local key_cache = setmetatable({ n=0 }, { __mode = "v" })

local function generate_key(name)
    if type(name) == "string" then return string.format("%q", name)
    elseif type(name) == "number" or type(name) == "boolean" then return tostring(name)
    else -- complex key, use key_cache for lookup
        local i = key_cache.n
        key_cache[i] = name
        key_cache.n = i+1
        return "key_cache["..tostring(i).."]"
    end
end

local function generate_printable_key(name)
    return "[" .. (type(name) == "string" and string.format("%q", name) or tostring(name)) .. "]"
end

local DBGP_ERR_METATABLE = {} -- unique object used to identify DBGp errors
--- Throws a correct DBGp error which result in a fine tuned error message to the server.
-- It is intended to be called into a command to make a useful error message, a standard Lua error
-- result in a code 998 error (internal debugger error).
-- @param code numerical error code
-- @param message message string (optional)
-- @param attrs extra attributes to add to the response tag (optional)
local function dbgp_error(code, message, attrs)
    error(setmetatable({ code = code, message = message, attrs = attrs or {} }, DBGP_ERR_METATABLE), 2)
end

local function dbgp_assert(code, success, ...)
    if not success then dbgp_error(code, (...)) end
    return success, ...
end

--- Gets a script stack level with additional debugger logic added
-- @param l (number) stack level to get for debugged script (0 based)
-- @return real Lua stack level suitable to be passed through deubg functions
local function get_script_level(l)
    local hook = debug.gethook()
    for i=2, math.huge do
        if assert(debug.getinfo(i, "f")).func == hook then
            return i + l -- the script to level is just below, but because of the extra call to this function, the level is ok for callee
        end
    end
end

local Multival = { __tostring = function() return "" end } -- used as metatable to identify tables that are actually multival
--- Packs a pcall results into a table with a correct nil handling
-- @param ... what pcall returned
-- @return (boolean) success state
-- @return (Multival or any) In case of success, table containing results and the "n" field for size (trailing nil handling)
--                           and having Multival as metatable. In case of failure, a the error object
local function packpcall(...)
    local success = (...)
    local results = success and setmetatable({ n = select("#", ...) - 1, select(2, ...) }, Multival) or (select(2, ...))
    return success, results
end

-------------------------------------------------------------------------------
--  XML Elements generators
-------------------------------------------------------------------------------

local xmlattr_specialchars = { ['"'] = "&quot;", ["<"] = "&lt;", ["&"] = "&amp;" }
--- Very basic XML generator
-- Takes a table describing the document as argument. This table has the following structure:
--   * name: (string) tag name
--   * attrs: (table, optional) tag attribute (key/value table)
--   * children: (table, optional) child nodes, either sub-elements or string (text data) (sequence)
local function generateXML(xml)
    local pieces = { } -- string buffer

    local function generate(node)
        pieces[#pieces + 1] = "<"..node.name
        pieces[#pieces + 1] = " "
        for attr, val in pairs(node.attrs or {}) do
            pieces[#pieces + 1] = attr .. '="' .. tostring(val):gsub('["&<]', xmlattr_specialchars) .. '"'
            pieces[#pieces + 1] = " "
        end
        pieces[#pieces] = nil -- remove the last separator (useless)

        if node.children then
            pieces[#pieces + 1] = ">"
            for _, child in ipairs(node.children) do
                if type(child) == "table" then generate(child)
                else pieces[#pieces + 1] = "<![CDATA[" .. tostring(child) .. "]]>" end
            end
            pieces[#pieces + 1] = "</" .. node.name .. ">"
        else
            pieces[#pieces + 1] = "/>"
        end
    end

    generate(xml)
    return table.concat(pieces)
end

local function send_xml(skt, resp)
    if not resp.attrs then resp.attrs = {} end
    resp.attrs.xmlns = "urn:debugger_protocol_v1"

    local data = '<?xml version="1.0" encoding="UTF-8" ?>\n'..generateXML(resp)
    log("DEBUGGER", "DEBUG", "Send " .. data)
    blockingtcp.send(skt, tostring(#data).."\000"..data.."\000")
end

--- Return an XML tag describing a debugger error, with an optional message
-- @param code (number) error code (see DBGp specification)
-- @param msg  (string, optional) textual description of error
-- @return table, suitable to be converted into XML
local function make_error(code, msg)
    local elem = { name = "error", attrs = { code = code } }
    if msg then
        elem.children = { { name = "message", children = { tostring(msg) } } }
    end
    return elem
end

local debugintrospection = require"debugintrospection"
-- Provides some more informations about Lua functions
local function new_function_introspection(...)
    local result = debugintrospection["function"](...)
    if result and result.file then
        result.repr = result.repr .. "\n" .. get_uri("@" .. result.file) .. "\n" .. tostring(result.line_from)
        result.type = "function (Lua)"
    end
    return result
end


--- Makes a property form a name/value pair (and fullname), see DBGp spec 7.11 for details
-- It has a pretty basic handling of complex types (function, table, userdata), relying to Lua Inspector for advanced stuff.
-- @param context (number) context ID in which this value resides (workaround bug 352316)
-- @param value (any) the value to debug
-- @param name (any) the name associated with value, passed through tostring
-- @param fullname (string) a Lua expression to eval to get that property again (if nil, computed automatically)
-- @param depth (number) the maximum property depth (recursive calls)
-- @param pagesize (number) maximum children to include
-- @param page (number) the page to generate (0 based)
-- @param size_limit (number, optional) if set, the maximum size of the string representation (in bytes)
-- @param safe_name (boolean) if true, does not encode the name as table key
--TODO BUG ECLIPSE TOOLSLINUX-99 352316 : as a workaround, context is encoded into the fullname property
local function make_property(context, value, name, fullname, depth, pagesize, page, size_limit, safe_name)
    local dump = debugintrospection:new(false, false, true, false, true, true)
    dump["function"] = new_function_introspection -- override standard definition

    -- build XML
    local function build_xml(node, name, fullname, page, depth)
        local data = tostring(node.repr)

        local specials = { }
        if node.metatable then specials[#specials + 1] = "metatable" end
        if node.environment then specials[#specials + 1] = "environment" end

        local numchildren = #node + #specials
        local attrs = { type = node.array and "sequence" or node.type, name=name, fullname=rawb64(tostring(context).."|"..fullname),
                        encoding="base64", children = 0, size=#data }
        if numchildren > 0 then
            attrs.children = 1
            attrs.numchildren = numchildren
            attrs.pagesize = pagesize
            attrs.page = page
        end
        local children = { b64(size_limit and data:sub(1, size_limit) or data) }

        if depth > 0 then
            local from, to = page * pagesize + 1, (page + 1) * (pagesize)
            for i = from, math.min(#node, to) do
                local key, value = unpack(node[i])
                key = type(key) == "number" and dump.dump[key] or key
                value = type(value) == "number" and dump.dump[value] or value
                children[#children + 1] = build_xml(value, "[" .. key.repr .. "]", fullname .. "[" .. generate_key(key.ref) .. "]", 0, depth - 1)
            end
            for i = #node + 1, math.min(to, numchildren) do
                local special = specials[i - #node]
                local prop = build_xml(dump.dump[node[special]], special, special .. "[" .. fullname .. "]", 0, depth - 1)
                prop.attrs.type = "special"
                children[#children + 1] = prop
            end
        end

        return { name = "property", attrs = attrs, children = children }
    end

    fullname = fullname or ("(...)[" .. generate_key(name) .. "]")
    if not safe_name then name = generate_printable_key(name) end

    if getmetatable(value) == Multival then
        local children = { }
        for i=1, value.n do
            local val = dump[type(value[i])](dump, value[i], depth)
            val = type(val) == "number" and dump.dump[val] or val
            -- Since fullname is impossible to build for multivals and they are read only,
            -- generate_key is used to retireve reference to the object
            children[#children + 1] = build_xml(val, "["..i.."]", generate_key(val.ref), 0, depth - 1)
        end

        -- return just the value in case of single result
        if #children == 1 then
            return children[1]
        end

        -- when there are multiple results, they a wrapped into a multival
        return { attrs = { type="multival", name=name, fullname=tostring(context).."|"..fullname, encoding="base64",
                           numchildren=value.n, children=value.n > 0 and 1 or 0, size=0, pagesize=pagesize },
                 children = children,
                 name = "property" }
    else
        local root = dump[type(value)](dump, value, depth + 1)
        return build_xml(type(root) == "number" and dump.dump[root] or root, name, fullname, page, depth)
    end
end


-------------------------------------------------------------------------------
--  Output redirection handling
-------------------------------------------------------------------------------
-- Override standard output functions & constants to redirect data written to these files to IDE too.
-- This works only for output done in Lua, output written by C extensions is still go to system output file.

-- references to native values
io.base = { output = io.output, stdin = io.stdin, stdout = io.stdout, stderr = io.stderr }

function print(...)
    local buf = {...}
    for i=1, select("#", ...) do
        buf[i] = tostring(buf[i])
    end
    io.stdout:write(table.concat(buf, "\t") .. "\n")
end

-- Actually change standard output file but still return the "fake" stdout
function io.output(output)
    output = io.base.output(output)
    return io.stdout
end

local dummy = function() end

-- metatable for redirecting output (not printed at all in actual output)
local redirect_output = {
    write = function(self, ...)
        local buf = {...}
        for i=1, select("#", ...) do buf[i] = tostring(buf[i]) end
        buf = table.concat(buf):gsub("\n", "\r\n")
        send_xml(self.skt, { name="stream", attrs = { type=self.mode }, children = { b64(buf) } } )
    end,
    flush = dummy,
    close = dummy,
    setvbuf = dummy,
    seek = dummy
}
redirect_output.__index = redirect_output

-- metatable for cloning output (outputs to actual system and send to IDE)
local copy_output = {
    write = function(self, ...)
        redirect_output.write(self, ...)
        io.base[self.mode]:write(...)
    end,
    flush   = function(self, ...) return self.out:flush(...) end,
    close   = function(self, ...) return self.out:close(...) end,
    setvbuf = function(self, ...) return self.out:setvbuf(...) end,
    seek    = function(self, ...) return self.out:seek(...) end,
}
copy_output.__index = copy_output

-- Factory for both stdout and stderr commands, change file descriptor in io
local function output_command_handler_factory(mode)
    return function(self, args)
        if args.c == "0" then -- disable
            io[mode] = io.base[mode]
        else
            io[mode] = setmetatable({ skt = self.skt, mode = mode }, args.c == "1" and copy_output or redirect_output)
        end
        send_xml(self.skt, { name = "response", attrs = { command = mode, transaction_id = args.i, success = "1" } } )
    end
end


-------------------------------------------------------------------------------
--  Context handling
-------------------------------------------------------------------------------
local Context
do
    -- make unique object to access contexts
    local LOCAL, UPVAL, GLOBAL, STORE, HANDLE = {}, {}, {}, {}, {}

    -- create debug functions depending on coroutine context
    local foreign_coro_debug = { -- bare Lua functions are fine here because all parameters will be provided
        getlocal = debug.getlocal,
        setlocal = debug.setlocal,
        getinfo  = debug.getinfo,
    }

    local current_coro_debug = {
        getlocal = function(_, level, index)        return debug.getlocal(get_script_level(level), index) end,
        setlocal = function(_, level, index, value) return debug.setlocal(get_script_level(level), index, value) end,
        getinfo  = function(_, level, what)         return debug.getinfo(get_script_level(level), what) end,
    }

    --- Captures variables for given stack level. The capture contains local, upvalues and global variables.
    -- The capture can be seen as a proxy table to the stack level: any value can be queried or set no matter
    -- it is a local or an upvalue.
    -- The individual local and upvalues context are also available and can be queried and modified with indexed notation too.
    -- These objects are NOT persistant and must not be used outside the debugger loop which instanciated them !
    Context = {
        -- Context identifiers can be accessed by their DBGp context ID
        [0] = LOCAL,
        [1] = GLOBAL, -- DLTK internal ID for globals is 1
        [2] = UPVAL,

        -- gets a variable by name with correct handling of Lua scope chain
        -- the or chain does not work here beacause __index metamethod would raise an error instead of returning nil
        __index = function(self, k)
            if     self[LOCAL][STORE][k] then return self[LOCAL][k]
            elseif self[UPVAL][STORE][k] then return self[UPVAL][k]
            else return self[GLOBAL][k] end
        end,
        __newindex = function(self, k, v)
            if     self[LOCAL][STORE][k] then self[LOCAL][k] = v
            elseif self[UPVAL][STORE][k] then self[UPVAL][k] = v
            else self[GLOBAL][k] = v end
        end,

        LocalContext = {
            __index = function(self, k)
                local index = self[STORE][k]
                if not index then error("The local "..tostring(k).." does not exitsts.") end
                local handle = self[HANDLE]
                return select(2, handle.callbacks.getlocal(handle.coro, handle.level, index))
            end,
            __newindex = function(self, k, v)
                local index = self[STORE][k]
                if index then
                    local handle = self[HANDLE]
                    handle.callbacks.setlocal(handle.coro, handle.level, index, v)
                else error("Cannot set local " .. k) end
            end,
            -- Lua 5.2 ready :)
            --__pairs = function(self) return getmetatable(self).iterator, self, nil end,
            iterator = function(self, prev)
                local key, index = next(self[STORE], prev)
                if key then return key, self[key] else return nil end
            end
        },

        UpvalContext = {
            __index = function(self, k)
                local index = self[STORE][k]
                if not index then error("The local "..tostring(k).." does not exitsts.") end
                return select(2, debug.getupvalue(self[HANDLE], index))
            end,
            __newindex = function(self, k, v)
                local index = self[STORE][k]
                if index then debug.setupvalue(self[HANDLE], index, v)
                else error("Cannot set upvalue " .. k) end
            end,
            -- Lua 5.2 ready :)
            -- __pairs = function(self) return getmetatable(self).iterator, self, nil end,
            iterator = function(self, prev)
                local key, index = next(self[STORE], prev)
                if key then return key, self[key] else return nil end
            end
        },

        --- Context constructor
        -- @param coro  (coroutine or nil) coroutine to map to (or nil for current coroutine)
        -- @param level (number) stack level do dump (script stack level)
        new = function(cls, coro, level)
            local locals, upvalues = {}, {}
            local debugcallbacks = coro and foreign_coro_debug or current_coro_debug
            local func = (debugcallbacks.getinfo(coro, level, "f") or dbgp_error(301, "No such stack level: "..tostring(level))).func

            -- local variables
            for i=1, math.huge do
                local name, val = debugcallbacks.getlocal(coro, level, i)
                if not name then break
                elseif name:sub(1,1) ~= "(" then -- skip internal values
                    locals[name] = i
                end
            end

            -- upvalues
            for i=1, math.huge do
                local name, val = debug.getupvalue(func, i)
                if not name then break end
                upvalues[name] = i
            end

            locals = setmetatable({ [STORE] = locals, [HANDLE] = { callbacks = debugcallbacks, level = level, coro = coro } }, cls.LocalContext)
            upvalues = setmetatable({ [STORE] = upvalues, [HANDLE] = func }, cls.UpvalContext)
            return setmetatable({ [LOCAL] = locals, [UPVAL] = upvalues, [GLOBAL] = debug.getfenv(func) }, cls)
        end,
    }
end

--- Handle caching of all instantiated context.
-- Returns a function which takes 2 parameters: thread and stack level and returns the corresponding context. If this
-- context has been already queried there is no new instantiation. A ContextManager is valid only during the debug loop
-- on which it has been instantiated. References to a ContextManager must be lost after the end of debug loop (so
-- threads can be collected).
-- If a context cannot be instantiated, an 301 DBGP error is thrown.
local function ContextManager()
    local cache = { }
    return function(thread, level)
        local thread_contexts = cache[thread or true] -- true is used to identify current thread (as nil is not a valid table key)
        if not thread_contexts then
            thread_contexts = { }
            cache[thread or true] = thread_contexts
        end

        local context = thread_contexts[level]
        if not context then
            context = Context:new(thread, level)
            thread_contexts[level] = context
        end

        return context
    end
end

-------------------------------------------------------------------------------
--  Property_* environment handling
-------------------------------------------------------------------------------
-- This in the environment in which properties are get or set.
-- It notably contain a collection of proxy table which handle transparentely get/set operations on special fields
-- and the cache of complex keys.
local property_evaluation_environment = {
    key_cache = key_cache,
    metatable = setmetatable({ }, {
        __index = function(self, tbl) return getmetatable(tbl) end,
        __newindex = function(self, tbl, mt) return setmetatable(tbl, mt) end,
    }),
    environment = setmetatable({ }, {
        __index = function(self, func) return getfenv(func) end,
        __newindex = function(self, func, env) return setfenv(func, env) end,
    }),
}
-- to allows to be set as metatable
property_evaluation_environment.__index = property_evaluation_environment

-------------------------------------------------------------------------------
--  Breakpoint registry
-------------------------------------------------------------------------------
-- Registry of current stack levels of all running threads
local stack_levels = setmetatable( { }, { __mode = "k" } )

-- File/line mapping for breakpoints (BP). For a given file/line, a list of BP is associated (DBGp specification section 7.6.1
-- require that multiple BP at same place must be handled)
-- A BP is a table with all additional properties (type, condition, ...) the id is the string representation of the table.
local breakpoints = {
    -- functions to call to match hit conditions
    hit_conditions = {
        [">="] = function(value, target) return value >= target end,
        ["=="] = function(value, target) return value == target end,
        ["%"]  = function(value, target) return (value % target) == 0 end,
    }
}

-- tracks events such as step_into or step_over
local events = { }

do
    local file_mapping = { }
    local id_mapping = { }
    local waiting_sessions = { } -- sessions that wait for an event (over, into, out)
    local step_into = nil        -- session that registered a step_into event, if any
    local sequence = 0 -- used to generate breakpoint IDs

    --- Inserts a new breakpoint into registry
    -- @param bp (table) breakpoint data
    -- @param uri (string, optional) Absolute file URI, for line breakpoints
    -- @param line (number, optional) Line where breakpoint stops, for line breakpoints
    -- @return breakpoint identifier
    function breakpoints.insert(bp)
        local bpid = sequence
        sequence = bpid + 1
        bp.id = bpid
        -- re-encode the URI to avoid any mismatch (with authority for example)
        local uri = url.parse(bp.filename)
        bp.filename = url.build{ scheme=uri.scheme, authority="", path=normalize(uri.path)}

        local filereg = file_mapping[bp.filename]
        if not filereg then
            filereg = { }
            file_mapping[bp.filename] = filereg
        end

        local linereg = filereg[bp.lineno]
        if not linereg then
            linereg = {}
            filereg[bp.lineno] = linereg
        end

        table.insert(linereg, bp)

        id_mapping[bpid] = bp
        return bpid
    end

    --- If breakpoint(s) exists for given file/line, uptates breakpoint counters
    -- and returns whether a breakpoint has matched (boolean)
    function breakpoints.at(file, line)
        local bps = file_mapping[file] and file_mapping[file][line]
        if not bps then return nil end

        local do_break = false
        for _, bp in pairs(bps) do
            if bp.state == "enabled" then
                local match = true
                if bp.condition then
                    -- TODO: this is not the optimal solution because Context can be instantiated twice if the breakpoint matches
                    local success, result = pcall(setfenv(bp.condition, Context:new(nil, 0)))
                    if not success then log("DEBUGGER", "ERROR", "Condition evaluation failed for breakpoint at %s:%d", file, line) end
                    -- debugger always stops if an error occurs
                    match = (not success) or result
                end
                if match then
                    bp.hit_count = bp.hit_count + 1
                    if breakpoints.hit_conditions[bp.hit_condition](bp.hit_count, bp.hit_value) then
                        if bp.temporary then
                            breakpoints.remove(bp.id)
                        end
                        do_break = true
                        -- there is no break to handle multiple breakpoints: all hit counts must be updated
                    end
                end
            end
        end
        return do_break
    end

    function breakpoints.get(id)
        if id then return id_mapping[id]
        else return id_mapping end
    end

    function breakpoints.remove(id)
        local bp = id_mapping[id]
        if bp then
            id_mapping[id] = nil
            local linereg = file_mapping[bp.filename][bp.lineno]
            for i=1, #linereg do
                if linereg[i] == bp then
                    table.remove(linereg, i)
                    break
                end
            end

            -- cleanup file_mapping
            if not next(linereg) then file_mapping[bp.filename][bp.lineno] = nil end
            if not next(file_mapping[bp.filename]) then file_mapping[bp.filename] = nil end
            return true
        end
        return false
    end

    --- Returns an XML data structure that describes given breakpoint
    -- @param id (number) breakpoint ID
    -- @return Table describing a <breakpooint> tag or nil followed by an error message
    function breakpoints.get_xml(id)
        local bp = id_mapping[id]
        if not bp then return nil, "No such breakpoint: "..tostring(id) end

        local response = { name = "breakpoint", attrs = { } }
        for k,v in pairs(bp) do response.attrs[k] = v end
        if bp.expression then
            response.children = { { name = "expression", children = { bp.expression } } }
        end

        -- internal use only
        response.attrs.expression = nil
        response.attrs.condition = nil
        response.attrs.temporary = nil -- TODO: the specification is not clear whether thsi should be provided, see other implementations
        return response
    end

    --- Register an event to be triggered.
    -- @param event event name to register (must be "over", "out" or "into")
    function events.register(event)
        local thread = coroutine.running() or "main"
        log("DEBUGGER", "DEBUG", "Registered %s event for %s (%d)", event, tostring(thread), stack_levels[thread])
        if event == "into" then
            step_into = true
        else
            waiting_sessions[thread] = { event, stack_levels[thread] }
        end
    end

    --- Returns if an event (step into, over, out) is triggered.
    -- Does *not* discard events (even if they match) as event must be discarded manually if a breakpoint match before anyway.
    -- @return true if an event has matched, false otherwise
    function events.does_match()
        if step_into then return true end

        local thread = coroutine.running() or "main"
        local event = waiting_sessions[thread]
        if event then
            local event_type, target_level = unpack(event)
            local current_level = stack_levels[thread]

            if (event_type == "over" and current_level <= target_level) or   -- step over
               (event_type == "out"  and current_level <  target_level) then -- step out
                log("DEBUGGER", "DEBUG", "Event %s matched!", event_type)
                return true
            end
        end
        return false
    end

    --- Discards event for current thread (if any)
    function events.discard()
        waiting_sessions[coroutine.running() or "main"] = nil
        step_into = nil
    end
end

-------------------------------------------------------------------------------
--  Debugger main loop
-------------------------------------------------------------------------------

--- Send the XML response to the previous continuation command and clear the previous context
local function previous_context_response(self, reason)
    self.previous_context.status = self.state
    self.previous_context.reason = reason or "ok"
    send_xml(self.skt, { name="response", attrs = self.previous_context } )
    self.previous_context = nil
end

--- Gets the coroutine behind an id
-- Throws errors on unknown identifiers
-- @param  coro_id  (string or nil) Coroutine identifier or nil
-- @return Coroutine instance or nil (if coro_id was nil or if coroutine is the current coroutine)
local function get_coroutine(coro_id)
    if coro_id then
        local coro = dbgp_assert(399, active_coroutines.from_id[tonumber(coro_id)], "No such coroutine")
        dbgp_assert(399, coroutine.status(coro) ~= "dead", "Coroutine is dead")
        return coro ~= corunning() and coro or nil
    end
    return nil
end

-- Debugger command functions. Each function handle a different command.
-- A command function is called with 3 arguments
--   1. the debug session instance
--   2. the command arguments as table
--   3. the command data, if any
-- The result is either :
--   * true (or any value evaluated to true) : the debugger will resume the execution of the application (continuation command)
--   * false : only in async mode, the debugger WILL wait for further commands instead of continuing (typically, break command)
--   * nil/no return : in sync mode, the debugger will wait for another command. In async mode the debugger will continue the execution
local commands
commands = {
    ["break"] = function(self, args)
        self.state = "break"
        -- send response to previous command
        previous_context_response(self)
        -- and then response to break command itself
        send_xml(self.skt, { name="response", attrs = { command = "break", transaction_id = args.i, success = 1 } } )
        return false
    end,

    status = function(self, args)
        send_xml(self.skt, { name="response", attrs = {
            command = "status",
            reason = "ok",
            status = self.state,
            transaction_id = args.i } } )
    end,

    stop = function(self, args)
        send_xml(self.skt, { name="response", attrs = {
            command = "stop",
            reason = "ok",
            status = "stopped",
            transaction_id = args.i } } )
        self.skt:close()
        os.exit(1)
    end,

    feature_get = function(self, args)
        local name = args.n
        local response = constant_features[name] or variable_features[name] or (not not commands[name])
        send_xml(self.skt, { name="response", attrs = {
              command = "feature_get",
              feature_name = name,
              supported = response and "1" or "0",
              transaction_id = args.i },
            children = { tostring(response) } } )
    end,

    feature_set = function(self, args)
        local name, value = args.n, args.v
        local success = 0
        if variable_features[name] then
            variable_features[name] = value
            success = 1
        end
        send_xml(self.skt, { name="response", attrs = {
            command = "feature_set",
            feature = name,
            success = success,
            transaction_id = args.i
        } } )
    end,

    typemap_get = function(self, args)
        local function gentype(name, type, xsdtype)
            return { name = "map", attrs = { name = name, type = type, ["xsi:type"] = xsdtype } }
        end

        send_xml(self.skt, { name="response", attrs = {
                command = "typemap_get",
                transaction_id = args.i,
                ["xmlns:xsi"] = "http://www.w3.org/2001/XMLSchema-instance",
                ["xmlns:xsd"] = "http://www.w3.org/2001/XMLSchema",
            }, children = {
                gentype("nil", "null"),
                gentype("boolean", "bool", "xsd:boolean"),
                gentype("number", "float", "xsd:float"),
                gentype("string", "string", "xsd:string"),
                gentype("function", "resource"),
                gentype("userdata", "resource"),
                gentype("thread", "resource"),
                gentype("table", "hash"),
                gentype("sequence", "array"), -- artificial type to represent sequences (1-n continuous indexes)
                gentype("multival", "array"), -- used to represent return values
            } } )
    end,

    run = function(self) return true end,

    step_over = function(self)
        events.register("over")
        return true
    end,

    step_out = function(self)
        events.register("out")
        return true
    end,

    step_into = function(self)
        events.register("into")
        return true
    end,

    eval = function(self, args, data)
        log("DEBUGGER", "DEBUG", "Going to eval "..data)
        local result, err, success
        -- first, try to load as expression
        local func, err = loadstring("return "..data)

        -- if it is not a, expression, try as statement (assignment, ...)
        if not func then
            func, err = loadstring(data)
        end

        if func then
            -- DBGp does not support stack level here, see http://bugs.activestate.com/show_bug.cgi?id=81178
            setfenv(func, self.stack(nil, 0))
            success, result = packpcall(pcall(func))
            if not success then err = result end
        end

        local response = { name = "response", attrs = { command = "eval", transaction_id = args.i } }
        if not err then
            response.attrs.success = 1
            -- As of Lua 5.1, the maximum stack size (and result count) is 8000, this limit is used to fit all results in one page
            response.children = { make_property(0, result, data, "", 1, 8000, 0, nil) }
        else
            response.attrs.success = 0
            response.children = { make_error(206, err) }
        end
        send_xml(self.skt, response)
    end,

    stdout = output_command_handler_factory("stdout"),
    stderr = output_command_handler_factory("stderr"),

    breakpoint_set = function(self, args, data)
        if args.o and not breakpoints.hit_conditions[args.o] then dbgp_error(200, "Invalid hit_condition operator: "..args.o) end

        local filename, lineno = args.f, tonumber(args.n)
        local bp = {
            type = args.t,
            state = args.s or "enabled",
            temporary = args.r == "1", -- "0" or nil makes this property false
            hit_count = 0,
            filename = filename,
            lineno = lineno,
            hit_value = tonumber(args.h or 0),
            hit_condition = args.o or ">=",
        }

        if args.t == "conditional" then
            bp.expression = data
            -- the expression is compiled only once
            bp.condition = dbgp_assert(207, loadstring("return (" .. data .. ")"))
        elseif args.t ~= "line" then dbgp_error(201, "BP type " .. args.t .. " not yet supported") end

        local bpid = breakpoints.insert(bp)
        send_xml(self.skt, { name = "response", attrs = { command = "breakpoint_set", transaction_id = args.i, state = bp.state, id = bpid } } )
    end,

    breakpoint_get = function(self, args)
        send_xml(self.skt, { name = "response",
                             attrs = { command = "breakpoint_get", transaction_id = args.i },
                             children = { dbgp_assert(205, breakpoints.get_xml(tonumber(args.d))) } })
    end,

    breakpoint_list = function(self, args)
        local bps = { }
        for id, bp in pairs(breakpoints.get()) do bps[#bps + 1] = breakpoints.get_xml(id) end
        send_xml(self.skt, { name = "response",
                             attrs = { command = "breakpoint_list", transaction_id = args.i },
                             children = bps })
    end,

    breakpoint_update = function(self, args)
        local bp = breakpoints.get(tonumber(args.d))
        if not bp then dbgp_error(205, "No such breakpint "..args.d) end
        if args.o and not breakpoints.hit_conditions[args.o] then dbgp_error(200, "Invalid hit_condition operator: "..args.o) end

        local response = { name = "response", attrs = { command = "breakpoint_update", transaction_id = args.i } }
        bp.state = args.s or bp.state
        bp.lineno = tonumber(args.n or bp.lineno)
        bp.hit_value = tonumber(args.h or bp.hit_value)
        bp.hit_condition = args.o or bp.hit_condition
        send_xml(self.skt, response)
    end,

    breakpoint_remove = function(self, args)
        local response = { name = "response", attrs = { command = "breakpoint_remove", transaction_id = args.i } }
        if not breakpoints.remove(tonumber(args.d)) then dbgp_error(205, "No such breakpint "..args.d) end
        send_xml(self.skt, response)
    end,

    stack_depth = function(self, args)
        local depth = 0
        local coro = get_coroutine(args.o)
        for level = coro and 0 or get_script_level(0), math.huge do
            local info = getinfo(coro, level, "S")
            if not info then break end -- end of stack
            depth = depth + 1
            if info.what == "main" then break end -- levels below main chunk are not interesting
        end
        send_xml(self.skt, { name = "response", attrs = { command = "stack_depth", transaction_id = args.i, depth = depth} } )
    end,

    stack_get = function(self, args) -- TODO: dynamic code
        -- special URIs to identify unreachable stack levels
        local what2uri = {
            tail = "tailreturn:/",
            C    = "ccode:/",
        }

        local function make_level(info, level)
            local attrs = { level = level, where = info.name, type="file" }
            local uri = get_uri(info.source)
            if uri and info.currentline then -- reachable level
                attrs.filename = uri
                attrs.lineno = info.currentline
            else
                attrs.filename = what2uri[info.what] or "unknown:/"
                attrs.lineno = -1
            end
            return { name = "stack", attrs = attrs }
        end

        local children = { }
        local coro = get_coroutine(args.o)
        local level = coro and 0 or get_script_level(0)

        if args.d then
            local stack_level = tonumber(args.d)
            children[#children+1] = make_level(getinfo(coro, stack_level + level, "nSl"), stack_level)
        else
            for i=level, math.huge do
                local info = getinfo(coro, i, "nSl")
                if not info then break end
                children[#children+1] = make_level(info, i-level)
                if info.what == "main" then break end -- levels below main chunk are not interesting
            end
        end

        send_xml(self.skt, { name = "response", attrs = { command = "stack_get", transaction_id = args.i}, children = children } )
    end,

    --- Lists all active coroutines.
    -- Returns a list of active coroutines with their id (an arbitrary string) to query stack and properties. The id is
    -- guaranteed to be unique and stable for all coroutine life (they can be reused as long as coroutine exists).
    -- Others commands such as stack_get or property_* commands takes an additional -o switch to query a particular cOroutine.
    -- If the switch is not given, running coroutine will be used.
    -- In case of error on coroutines (most likely coroutine not found or dead), an error 399 is thrown.
    -- Note there is an important limitation due to Lua 5.1 coroutine implementation: you cannot query main "coroutine" from
    -- another one, so main coroutine is not in returned list (this will change with Lua 5.2).
    --
    -- This is a non-standard command. The returned XML has the following strucuture:
    --     <response command="coroutine_list" transaction_id="0">
    --       <coroutine name="<some printtable name>" id="<coroutine id>" running="0|1" />
    --       ...
    --     </response>
    coroutine_list = function(self, args)
        local running = coroutine.running()
        local coroutines = {  }
        -- as any operation on main coroutine will fail, it is not yet listed
        -- coroutines[1] = { name = "coroutine", attrs = { id = 0, name = "main", running = (running == nil) and "1" or "0" } }
        for id, coro in pairs(active_coroutines.from_id) do
            if id ~= "n" then
                coroutines[#coroutines + 1] = { name = "coroutine", attrs = { id = id, name = tostring(coro), running = (coro == running) and "1" or "0" } }
            end
        end
        send_xml(self.skt, { name = "response", attrs = { command = "coroutine_list", transaction_id = args.i}, children = coroutines } )
    end,

    context_names = function(self, args)
        local coro = get_coroutine(args.o)
        local level = tonumber(args.d or 0)
        local info = getinfo(coro, coro and level or get_script_level(level), "f") or dbgp_error(301, "No such stack level "..tostring(level))

        -- All contexts are always passed, even if empty. This is how DLTK expect context, what about others ?
        local contexts = {
            { name = "context", attrs = { name = "Local",   id = 0 } },
            { name = "context", attrs = { name = "Upvalue", id = 2 } },
            { name = "context", attrs = { name = "Global",  id = 1 } },
        }

        send_xml(self.skt, { name = "response", attrs = { command = "context_names", transaction_id = args.i}, children = contexts } )
    end,

    context_get = function(self, args)
        local context = tonumber(args.c or 0)
        local cxt_id = Context[context] or dbgp_error(302, "No such context: "..tostring(args.c))
        local level = tonumber(args.d or 0)
        local coro = get_coroutine(args.o)
        local cxt = self.stack(coro, level)

        local properties = { }
        -- iteration over global is different (this could be unified in Lua 5.2 thanks to __pairs metamethod)
        for name, val in (context == 1 and next or getmetatable(cxt[cxt_id]).iterator), cxt[cxt_id], nil do
            -- the DBGp specification is not clear about the depth of a context_get, but a recursive get could be *really* slow in Lua
            properties[#properties + 1] = make_property(context, val, name, nil, 0, variable_features.max_children, 0,
                                                        variable_features.max_data, context ~= 1)
        end

        send_xml(self.skt, { name = "response",
                             attrs = { command = "context_get", transaction_id = args.i, context = context},
                             children = properties } )
    end,

    property_get = function(self, args)
        --TODO BUG ECLIPSE TOOLSLINUX-99 352316
        local context, name = assert(unb64(args.n):match("^(%d+)|(.*)$"))
        context = tonumber(args.c or context)
        local cxt_id = Context[context] or dbgp_error(302, "No such context: "..tostring(args.c))
        local level = tonumber(args.d or 0)
        local coro = get_coroutine(args.o)
        local size = tonumber(args.m or variable_features.max_data)
        if size < 0 then size = nil end -- call from property_value
        local page = tonumber(args.p or 0)
        local cxt = self.stack(coro, level)
        local chunk = dbgp_assert(206, loadstring("return "..name))
        setfenv(chunk, property_evaluation_environment)
        local prop = select(2, dbgp_assert(300, pcall(chunk, cxt[cxt_id])))
        local response = make_property(context, prop, name, name, variable_features.max_depth, variable_features.max_children, page, size)
        -- make_property is not able to flag special variables as such when they are at root of property
        -- special variables queries are in the form "<proxy name>[(...)[a][b]<...>]"
        -- TODO: such parsing is far from perfect
        if name:match("^[%w_]+%[.-%b[]%]$") == name then response.attrs.type = "special" end
        send_xml(self.skt, { name = "response",
                             attrs = { command = "property_get", transaction_id = args.i, context = context},
                             children = { response } } )
    end,

    property_value = function(self, args)
        args.m = -1
        commands.property_get(self, args)
    end,

    property_set = function(self, args, data)
        local context, name = assert(unb64(args.n):match("^(%d+)|(.*)$"))
        context = tonumber(args.c or context)
        local cxt_id = Context[context] or dbgp_error(302, "No such context: "..tostring(args.c))
        local level = tonumber(args.d or 0)
        local coro = get_coroutine(args.o)
        local cxt = self.stack(coro, level)

        -- evaluate the new value in the local context
        local value = select(2, dbgp_assert(206, pcall(setfenv(dbgp_assert(206, loadstring("return "..data)), cxt))))

        local chunk = dbgp_assert(206, loadstring(name .. " = value"))
        setfenv(chunk, setmetatable({ value = value }, property_evaluation_environment))
        dbgp_assert(206, pcall(chunk, cxt[cxt_id]))
        send_xml(self.skt, { name = "response", attrs = { success = 1, transaction_id = args.i } } )
    end,

    --TODO dynamic code handling
    -- The DBGp specification is not clear about the line number meaning, this implementation is 1-based and numbers are inclusive
    source = function(self, args)
        local path
        if args.f then
            path = get_path(args.f)
        else
            path = debug.getinfo(get_script_level(0), "S").source
            assert(path:sub(1,1) == "@")
            path = path:sub(2)
        end
        local file, err = io.open(path)
        if not file then dbgp_error(100, err, { success = 0 }) end
        -- Try to identify compiled files
        if file:read(1) == "\033" then dbgp_error(100, args.f.." is bytecode", { success = 0 }) end
        file:seek("set", 0)

        local source = cowrap(function()
            local beginline, endline, currentline = tonumber(args.b or 0), tonumber(args.e or math.huge), 0
            for line in file:lines() do
                currentline = currentline + 1
                if currentline >= beginline and currentline <= endline then
                    coyield(line.."\n")
                elseif currentline >= endline then break end
            end
            file:close()
        end)
        -- the DBGp specification does not require any specific EOL marker, but use normalize can be used here
        local filter = ltn12.filter.chain(mime.encode("base64"), mime.wrap("base64"))
        local output = { }
        local sink = ltn12.sink.chain(filter, ltn12.sink.table(output))
        assert(ltn12.pump.all(source, sink))

        send_xml(self.skt, { name = "response",
                             attrs = { command = "source", transaction_id = args.i, success = 1},
                             children = { table.concat(output) } })
    end,
}

--- This function handles the debugger commands while the execution is paused. This does not use coroutines because there is no
-- way to get main coro in Lua 5.1 (only in 5.2)
local function debugger_loop(self, async_packet)
    blockingtcp.settimeout(self.skt, nil) -- set socket blocking

    -- in async mode, the debugger does not wait for another command before continuing and does not modify previous_context
    local async_mode = async_packet ~= nil

    if self.previous_context and not async_mode then
        self.state = "break"
        previous_context_response(self)
    end
    self.stack = ContextManager() -- will be used to mutualize context allocation for each loop

    while true do
        -- reads packet
        local packet = async_packet or assert(read_packet(self.skt))
        async_packet = nil
        log("DEBUGGER", "DEBUG", packet)
        local cmd, args, data = cmd_parse(packet)

        -- FIXME: command such as continuations sent in async mode could lead both engine and IDE in inconsistent state :
        --        make a blacklist/whitelist of forbidden or allowed commands in async ?
        -- invoke function
        local func = commands[cmd]
        if func then
            local ok, cont = pcall(func, self, args, data)
            if not ok then -- internal exception
                local code, msg, attrs
                if type(cont) == "table" and getmetatable(cont) == DBGP_ERR_METATABLE then
                    code, msg, attrs = cont.code, cont.message, cont.attrs
                else
                    code, msg, attrs = 998, tostring(cont), { }
                end
                log("DEBUGGER", "ERROR", "Command %s caused: (%d) %s", cmd, code, msg)
                attrs.command, attrs.transaction_id = cmd, args.i
                send_xml(self.skt, { name="response", attrs = attrs, children = { make_error(code, msg) } } )
            elseif cont then
                self.previous_context = { command = cmd, transaction_id = args.i }
                break
            elseif cont == nil and async_mode then
                break
            elseif cont == false then -- In case of commands that fully resumes debugger loop, the mode is sync
                async_mode = false
            end
        else
            log("DEBUGGER", "WARNING", "Got unknown command: "..cmd)
            send_xml(self.skt, { name="response", attrs = { command = cmd, transaction_id = args.i, },
                children = { make_error(4) } } )
        end
    end

    self.stack = nil -- free allocated contexts
    self.state = "running"
    blockingtcp.settimeout(self.skt, 0) -- reset socket to async
end

local function debugger_hook(event, line)
    local thread = corunning() or "main"

    if event == "call" then
        stack_levels[thread] = stack_levels[thread] + 1
    elseif event == "return" or event == "tail return" then
        stack_levels[thread] = stack_levels[thread] - 1
    else -- line event: check for breakpoint
        local do_break, packet = nil, nil
        local info = debug_getinfo(2, "S")
        local uri = get_uri(info.source)
        if uri and uri ~= debugger_uri then -- the debugger does not break if the source is not known
            do_break = breakpoints.at(uri, line) or events.does_match()
            if do_break then
                events.discard()
            end

            -- check for async commands
            if not do_break then
                packet = read_packet(active_session.skt)
                if packet then do_break = true end
            end
        end

        if do_break then
            local success, err = pcall(debugger_loop, active_session, packet)
            if not success then log("DEBUGGER", "ERROR", "Error while debug loop: "..err) end
        end
    end
end

local function init(host, port,idekey)
    host = host or os.getenv "DBGP_IDEHOST" or "127.0.0.1"
    port = port or os.getenv "DBGP_IDEPORT" or "10000"
    idekey = idekey or os.getenv("DBGP_IDEKEY") or "luaidekey"

    local skt = assert(socket.tcp())
    blockingtcp.settimeout(skt, nil)

    -- try to connect several times: if IDE launches both process and server at same time, first connect attempts may fail
    local ok, err
    for i=1, 5 do
        ok, err = blockingtcp.connect(skt, host, port)
        if ok then break end
        blockingtcp.sleep(0.5)
    end
    if err then error(string.format("Cannot connect to %s:%d : %s", host, port, err)) end

    -- get the debugger URI
    debugger_uri = get_uri(debug.getinfo(1).source)

    -- get the root script path (the highest possible stack index)
    local source
    for i=2, math.huge do
        local info = debug.getinfo(i)
        if not info then break end
        source = get_uri(info.source) or source
    end
    if not source then source = "unknown:/" end -- when loaded before actual script (with a command line switch)

    -- generate some kind of thread identifier
    local thread = coroutine.running() or "main"
    stack_levels[thread] = 1 -- the return event will set the counter to 0
    local sessionid = tostring(os.time()) .. "_" .. tostring(thread)

    send_xml(skt, { name="init", attrs = {
        appid = "Lua DBGp",
        idekey = idekey,
        session = sessionid,
        thread = tostring(thread),
        parent = "",
        language = "Lua",
        protocol_version = "1.0",
        fileuri = source
        } })

    local sess = { skt = skt, state = "starting", id = sessionid }
    active_session = sess
    debugger_loop(sess)

    -- set debug hooks
    debug.sethook(debugger_hook, "rlc")

    -- install coroutine collecting functions.
    -- TODO: maintain a list of *all* coroutines can be overkill (for example, the ones created by copcall), make a extension point to
    -- customize debugged coroutines
    -- coroutines are referenced during their first resume (so we are sure that they always have a stack frame)
    local function resume_handler(coro, ...)
        if costatus(coro) == "dead" then
            local coro_id = active_coroutines.from_coro[coro]
            active_coroutines.from_id[coro_id] = nil
            active_coroutines.from_coro[coro] = nil
            stack_levels[coro] = nil
        end
        return ...
    end

    function coroutine.resume(coro, ...)
        if not stack_levels[coro] then
            -- first time referenced
            stack_levels[coro] = 0
            active_coroutines.n = active_coroutines.n + 1
            active_coroutines.from_id[active_coroutines.n] = coro
            active_coroutines.from_coro[coro] = active_coroutines.n
            debug.sethook(coro, debugger_hook, "rlc")
        end
        return resume_handler(coro, coresume(coro, ...))
    end

    -- coroutine.wrap uses directly C API for coroutines and does not trigger our overridden coroutine.resume
    -- so this is an implementation of wrap in pure Lua
    local function wrap_handler(status, ...)
        if not status then error((...)) end
        return ...
    end

    function coroutine.wrap(f)
        local coro = coroutine.create(f)
        return function(...)
            return wrap_handler(coroutine.resume(coro, ...))
        end
    end

    return sess
end

return init
