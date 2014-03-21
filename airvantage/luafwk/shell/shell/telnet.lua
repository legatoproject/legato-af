-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require 'sched'
require "print"
require "coxpcall"
local _G = _G
local assert = assert
local copcall = copcall
local coxpcall = coxpcall
local error = error
local getmetatable = getmetatable
local ipairs = ipairs
local loader = require"utils.loader"
local log = require 'log'
local next = next
local pairs = pairs
local print = print
local proc = proc
local select = select
local setmetatable = setmetatable
local siprint = siprint
local socket = require 'socket'
local string = string
local table = table
local telnet = require 'telnet'
local tonumber = tonumber
local tostring = tostring
local type = type
local unpack = unpack
local pathutils = require 'utils.path'

module(...)

SHELL = {
   auth     = false,  -- do not do authentification by default
   prompt   = "> ",
   prompt2  = "+ ",
   greeting = "Lua interactive shell\r\n",
   printindent = 3,
   runningtasks = {}, -- used to record what is the task calling print coming from
}


local function autocomplete(path, env)
    env = env or _G
    path = path or ""

    path = path:match("([%w_][%w_%.%:]*)$") or "" -- get the significant end part of the path (non alphanum are spliters...)
    local p, s, l = path:match("(.-)([%.%:]?)([^%.%:]*)$") -- separate into sub path and leaf, getting the separator
    local t = pathutils.get(env, p)
    local funconly =  s == ":"
    local tr = {}

    local function copykeys(src, dst)
        if type(src) ~= 'table' then return end
        for k, v in pairs(src) do
            local tv = type(v)
            if type(k)=='string' and k:match("^"..l) and (not funconly or tv=='function' or (tv=='table' and getmetatable(v) and getmetatable(v).__call)) then dst[k] = true end
        end
    end

    copykeys(t, tr)

    if s == ":" or s == "." then
        local m = getmetatable(t)
        if m then
            local i, n = m.__index, m.__newindex
            copykeys(i, tr)
            if n ~= i then copykeys(n, tr) end
            if m~= i and m ~= n then copykeys(m, tr) end -- far from being perfect, but it happens that __index or __newindex are functions that uses the metatable as a object method holder
        end
    end

    if not next(tr) then return 0, nil end -- no proposals

    local r = {}
    for k, v in pairs(tr) do table.insert(r, k) end

    -- sort the entries by lexical order
    table.sort(r)

    -- when we have a complete match we can add a ".", ":" or "(" if the object is a table or a function
    if l == r[1] then
        local tl = t[l]
        local ttl = type(tl)
        if ttl == 'function' then r[1]=r[1].."("
        elseif getmetatable(tl) and getmetatable(tl).__index then r[1]=r[1]..":" -- pure guess, object that have __index metamethod may be an 'object', so add ':'
        elseif ttl == 'table' then r[1]=r[1].."."
        end
    end

    return #r, table.concat(r, "\0")
end


local function interpret(self, input)
    local output
    local cmd, args, again
    repeat
        again, output, cmd, args = self.ts:interpret(input)
        if output then
            assert(self.sock:send(output))
        end
        if cmd ~= "nop" then
            sched.signal(self, cmd, args)
        end
        input = nil
    until not again
end


local function getcurrentshell()
    return SHELL.runningtasks[proc.tasks.running]
end



-- Redefine the print function so that it get printed on the shell
local origprint = _G.print
local function print(...)
    local self = getcurrentshell()
    if not self then return origprint(...) end -- fallback on standard printer if the shell is closed
    local t = {}
    local args = table.pack(...)
    for k = 1, args.n do table.insert(t, tostring(args[k])) end
    self.ts:output(table.concat(t, '\t')..'\r\n')
    interpret(self)
end
_G.print = print



local function readloop(self)
    local input
    local cmd, args, again

    while true do
        input = assert(self.sock:receive('*')) -- read some data as it arrives
        interpret(self, input)
    end
end
local function readloop_prot(self)
    local s, err = copcall(readloop, self)
    if not s and not err:match('closed') then error(err) end
    return
end

local function put_in_background(self)
    local i = #self.runningjobs + 1 -- get an entry that was set to nil
    self.runningjobs[i] = self.foreground
    self.runningjobs[self.foreground] = i
    self.ts:output("Put task "..tostring(self.foreground).." in background as job #"..i.."\r\n")
    self.foreground = nil
end

-- this is needed for coxpcall so to print the traceback
-- coxpcall call the err handler with the traceback as first argument (xpcall actually only gives the error message)
local function id(...)
    return ...
end
local function try_to_compile_and_run(self)

    local background, pretty
    -- Parse first special character --
    local special_char, special_line = self.lines[1] :match "^%s*([&=:])(.*)"
    local original1st = self.lines[1]
    if special_char == '=' then -- print expression value
        self.lines[1] = "return " .. special_line
    elseif special_char == '&' then -- Execute in //
        background = true
        self.lines[1] = special_line
    elseif special_char == ':' then -- Use pretty printer to output the results
        pretty = true
        self.lines[1] = "return " .. special_line
    end

    -- Attempt to compile and run --
    local hsrc = table.concat { original1st, select (2, unpack (self.lines))}
    local code, msg = loader.loadbuffer(self.lines, "@shell")

    -- can't compile --
    if not code then
        if msg:match "<eof>" then -- incomplete
            self.ts:showprompt(self.prompt2)
        else -- compile error
            self.ts:addhistory(hsrc)
            self.lines = { }
            self.ts:output("Compilation error: "..msg.."\r\n")
            self.ts:showprompt(self.prompt)
        end

    -- compiled successfully --
    else
        self.ts:addhistory(hsrc)
        local task, results
        task = sched.run(function() results = table.pack(coxpcall(code, id)) end)
        self.foreground = task
        SHELL.runningtasks[task] = self
        if background then
            put_in_background(self)
            self.ts:showprompt(self.prompt)
        end
        self.lines = { }
        sched.sigonce (task, 'die', function()
            local out
            SHELL.runningtasks[task] = nil
            if task == self.foreground then
                self.foreground = nil
                if results[1] then
                    out = {}
                    local printer = pretty and function(...) return siprint(self.printindent, ...) end  or tostring
                    for i = 2, results.n do
                        table.insert(out, "= ")
                        table.insert(out, printer(results[i]))
                        table.insert(out, "\r\n")
                    end
                else
                    out = tostring(results[2]) :gsub ("\n", "\r\n") .. "\r\n"
                end
            else
                out = "Job #"..tostring(self.runningjobs[task]).." finished\r\n"
                if not results[1] then log("SHELL", "ERROR", "Background task(%s) failed with err=%s", tostring(task), results[2]) end
                self.runningjobs[self.runningjobs[task]] = nil
                self.runningjobs[task] = nil
            end
            sched.signal(self, 'done', out)
        end)
    end
end

local function actionloop(self)
    local shell_events_fifo = { }
    self.acthook = sched.sighook (self, '*',
            function (...)
                table.insert (shell_events_fifo, { ... })
            end)

    local linebuf = {} -- holds lines that are added while already running a task
    while true do
        local ev, args
        while true do -- pop an event from fifo, wait for it if necessary
            local item = table.remove (shell_events_fifo, 1)
            if item then ev, args = unpack (item); break
            else sched.wait (self, '*') end
        end

        if ev == 'line' then
            if not self.foreground then
                table.insert(self.lines, args)
                try_to_compile_and_run(self)
            else
                table.insert(linebuf, args)
            end

        elseif ev == 'done' then
            args = type(args) == 'table' and table.concat(args) or args
            self.ts:output(args)
            self.ts:showprompt(self.prompt)

            if #linebuf>0 then
                for _, l in ipairs(linebuf) do sched.signal(self, 'line', l) end  -- resend line event with the buffered lines
                linebuf = {}
            end

        elseif ev == 'interrupt' then
            if self.foreground then
                self.ts:output("Interrupt task "..tostring(self.foreground).."!\r\n")
                sched.kill(self.foreground)
                self.foreground = nil
            end
            self.ts:showprompt(self.prompt)
            self.lines = {}
            linebuf = {}

        elseif ev == 'suspend' then
            if self.foreground then put_in_background(self) end
            self.ts:showprompt(self.prompt)
            self.lines = {}
            linebuf = {}


        elseif ev == 'close' then
            return

        end

        -- call the telnet interpreter with no input in case there some output to flush
        interpret(self)
    end
end


-- Instantiate and run a new shell
function run(sock, config)

    local self = setmetatable({}, {__index=SHELL})
    self.sock = sock

    -- Send the greeting!
    assert(sock:send(self.greeting))

    self.ts = telnet.new{mode=config.editmode, autocomplete=autocomplete, history=config.historysize}
    interpret(self) -- trigger some exchanges that may be necessary (like telnet initialization)

    self.lines = {}
    self.runningjobs = {}

    self.ts:showprompt(self.prompt)
    interpret(self)

    -- TODO do the authentication here?


    -- run the shell
    local readtask = sched.run(readloop_prot, self)
    local actiontask = sched.run(actionloop, self)


    -- Wait for end and release ressources on shell exit
    sched.multiwait({actiontask, readtask}, 'die')
    sched.signal(self, 'close') -- in case it was the read task that died first, ensure the other task is also killed...
    sock:close()
    sched.kill(self.acthook)
    log("SHELL", "INFO", "Shell connection closed")
end

-- Global functions added to enhance shell
function _G.jobs()
    local self = getcurrentshell()
    if not self then return end
    for k, v in pairs(self.runningjobs) do
        if tonumber(k) then
            self.ts:output("Job "..k..": "..tostring(v).."\r\n")
        end
    end
end

function _G.killjob(jobid)
    local self = getcurrentshell()
    if not self then return end
    if not self.runningjobs[jobid] then error("No job with id "..jobid) end
    sched.kill(self.runningjobs[jobid])
end

-- Start the server socket so that new shell can be instanciated on new connections
-- config is a table that hols shell params:
--   address: local address to bind to (default: "localhost")
--   port: local port to bind to (default : 2000)
--   editmode: "edit" or "line". "line" is the trivial default no-editable shell
--   historysize: the number of line that history must be able to handle. Only valid when editmode="edit" (default: 0 = no history)
function init(config)
   local function on_accept(client, server)
      log("SHELL", "INFO", "Shell connection from client (%s)", client:getpeername())
      run(client, config)
  end
   log("SHELL", "INFO", "Binding a shell server at address %s, port %s",  config.address or "?localhost", config.port or "?2000")
   assert(socket.bind(config.address or "localhost", config.port or 2000, on_accept))
   return "ok"
end