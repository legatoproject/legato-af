-------------------------------------------------------------------------------
-- Copyright (c) 2013 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Demonstration of basic authentication.
--
-- This module helps the implementation of the auth-basic HTTP authentication
-- method.
--------------------------------------------------------------------------------

require 'web.server'
require 'web.template'

--------------------------------------------------------------------------------
-- If this is true, error messaages will indicate whether an authentication
-- failed because of a non-existent user name or because of a bad password
-- for a valid user name.
--
-- Detailing the cause of an error is more convinient for legitimate users,
-- but facilitates cracking for mailicious users. Enable at your own risks.
--------------------------------------------------------------------------------
local DONT_GIVE_CLUES = false

--------------------------------------------------------------------------------
-- Contains informations about clients authentications
--------------------------------------------------------------------------------
local NONCES = {}

--------------------------------------------------------------------------------
-- Compute and returns the md5sum of input
--------------------------------------------------------------------------------
function web.md5(input)
   return require "crypto.md5"():update(input):digest()
end

--------------------------------------------------------------------------------
-- Generate a random nonce
--------------------------------------------------------------------------------
local function new_nonce()
   local f = io.open("/dev/urandom", "r")
   local value = f:read(64)
   local output = ""
   for c in value:gmatch(".") do 
      output = output .. string.format("%x", string.byte(c))
   end
   local nonce = web.md5(output)
   f:close()
   return nonce
end

local function nonce_authenticate_header(env)
   local nonce, opaque = new_nonce(), new_nonce()
   local timer = require 'timer'
   NONCES[nonce] = opaque
   timer.once(60, function() NONCES[nonce] = nil end)
   env.response_headers['WWW-Authenticate'] = env.response_headers['WWW-Authenticate'] .. 'nonce="'..nonce..'", opaque="'..opaque..'"'
   env.response_headers['Transfer-Encoding'] = nil
end

--------------------------------------------------------------------------------
-- Generate a HTTP header function which forces an authentication,
-- before letting any page content being delivered.
--------------------------------------------------------------------------------
function web.authenticate_header (realm, ha1)
   realm = realm or 'Secure Area'
   return function (env)
      env.response_headers['WWW-Authenticate'] = 'Digest realm="'..realm..'", qop="auth,auth-int",'
      local a = env.request_headers['authorization']
      if not a then -- no auth info given, request it
         log ("WEB-AUTH", "INFO", "No credential, asking for them")
         nonce_authenticate_header(env)
         env.response = "HTTP/1.1 401 Unauthorized"
         env.error_msg = "Login+password required"
         env.response_headers['Content-Length'] = 0
      else -- some auth info given, check it
         log ("WEB-AUTH", "INFO", "Login attempt on %s", env.url)
         local auth = {}
         for i in a:gmatch("%w+=[%w@%-\"/%.=]*") do
             auth[i:gsub("=[%w@%-\"/%.=]*", "")] = i:gsub('^%w+=', ''):gsub('"', '')
         end
         auth["method"] = env.method
         if not NONCES[auth["nonce"]] or NONCES[auth["nonce"]] ~= auth["opaque"] then
             env.response = "HTTP/1.1 401 Unauthorized"
             nonce_authenticate_header(env)
             env.error_msg = "Wrong input"
             env.response_headers['Content-Length'] = 0
             return
         end

         local ha2, response
         if auth["algorithm"] == "MD5-sess" then
             ha1 = web.md5(ha1 .. ":" .. auth["nonce"] .. ":" .. auth["cnonce"])
         end

         if auth["qop"] == "auth" or not auth["qop"] then
             ha2 = web.md5(auth["method"] .. ":" .. auth["uri"])
         end

         if auth["qop"] == "auth" or auth["qop"] == "auth-int" then
             response = web.md5(ha1 .. ":" .. auth["nonce"] .. ":" .. auth["nc"] .. ":" .. auth["cnonce"] .. ":" .. auth["qop"] .. ":" .. ha2)
         elseif not auth["qop"] then
             response = web.md5(ha1 .. ":" .. auth["nonce"] .. ":" .. ha2)
         end
         if response ~= auth["response"] then
             env.response = "HTTP/1.1 401 Unauthorized"
             env.error_msg = "Authentication failure"
             env.response_headers['Transfer-Encoding'] = nil
             env.response_headers['Content-Length'] = 0
             return
         end
         log ("WEB-AUTH", "INFO", "logged in successfully")
      end
   end
end
