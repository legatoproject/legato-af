-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- This service allows to compile strings, with <% ... %> markers, into
-- web pages, on the fly. It is more convinient than compiling pages
-- offline, but also more expensive in terms of time and memory.
--
-- Consider precompiling your pages offline before releasing your code,
-- or if you become short on resources.
--------------------------------------------------------------------------------

require 'web.server'

--------------------------------------------------------------------------------
-- Surround txt with '[=...=[' and ']=...=]' markers to make it a string,
-- making sure that it won't interfere with any ']=...=]' marker already
-- occuring inside txt.
--------------------------------------------------------------------------------
local function quote (txt)
   local eqstr = ''
   while txt :match (']'..eqstr..']') do eqstr = eqstr..'=' end
   return string.format("[%s[%s]%s]", eqstr, txt, eqstr)
end

--------------------------------------------------------------------------------
-- Take a string with some HTML and some <% ... %> markers, return the
-- source of a Lua function serving that page.
-- As a 2nd result, return a Boolean indicating whether some <% ... %> markers
-- were actually found and parsed.
--------------------------------------------------------------------------------
local function html2lua (x)

   -- no code fragments inside: --
   if not x :match "<%%.-%%>" then return quote (x), false end

   -------------------------------------------------------------------
   -- It's cleaner to extract bits of string from a chunk than
   -- chunk fragments from a string, so we add phony markers
   -- around, to search for "%>...<%" patterns rather "<%...%>"
   -------------------------------------------------------------------
   x = "%>"  ..  x  ..  "<%"

   -- we'll accumulate the function's sources here with acc(): --
   local _acc = { }
   local function acc(...)
      for _, x in ipairs{...} do table.insert (_acc, x) end
   end

   -------------------------------------------------------------------
   -- the resulting function takes as parameters:
   -- * 'echo' the source producing function
   -- * 'env'  the environment table, with fields:
   --   + method: 'PUT'/'GET'/'POST'/'DEL'/'HEAD'
   --   + request_headers:  key->value hashmap (to be read)
   --   + response_headers: key->value hashmap (to be written)
   --   + params: key->value hashmap, URL or POST body parameters
   --   + url: string without parameters
   --   + channel: WIP communication channel
   --   + mime_type: string
   -------------------------------------------------------------------
   acc "function (echo, env)\n"

   for code, munch, text in x :gmatch "(.-)(%-?)%%>(.-)<%%" do
      -- code starting with an '=' --> expression to print: --
      local expr = code :match "^%s*=(.+)$"
      if expr then acc ("echo (", expr, ") ")
      elseif #code>0 then acc (code, ' ') end
      munch = #munch>0

      -- '-' before closing '%>' --> remove spaces ahead of text: --
      if munch then text = text :match "^%s*(.*)$" end

      -- Surround the string with appropriate [=...=[ ... ]=...=] --
      if #text>0 then
         if not munch then text = '\n'..text end
         acc ("echo ", quote (text), " ")
      end
   end
   acc "end"
   return table.concat(_acc), true
end

function web.compile (html, name)
    local src = "return " .. html2lua (html)
    --print("***", name, '***\n\n', src)
    local f = loadstring (src, name or "html-template") ()
    return f
end

return web.compile
