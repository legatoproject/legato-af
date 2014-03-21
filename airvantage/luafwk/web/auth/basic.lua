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
-- Base64 decoder
--------------------------------------------------------------------------------
web.b64decode = require 'mime' .decode 'base64'

--------------------------------------------------------------------------------
-- Generate a HTTP header function which forces an authentication,
-- before letting any page content being delivered.
--------------------------------------------------------------------------------
function web.authenticate_header (user_table, realm)
   realm = realm or 'Secure Area'
   return function (env)
      env.response_headers['WWW-Authenticate'] = 'Basic realm="'..realm..'"'
      local a = env.request_headers['authorization']
      if not a then -- no auth info given, request it
         log ("WEB-AUTH", "INFO", "No credential, asking for them")
         env.response = "HTTP/1.1 401 AUTHORIZATION REQUIRED"
         env.error_msg = "Login+password required"
      else -- some auth info given, check it
         a = a :match "^Basic (.*)"
         a = web.b64decode(a)
         local login, password = a :match "^(.-):(.*)$"
         log ("WEB-AUTH", "INFO", "Login attempt by %q", login)
         local expected_password = user_table[login]
         if DONT_GIVE_CLUES then
            env.response = "HTTP/1.1 401 AUTHORIZATION REQUIRED"
            env.error_msg = "Authentication failure"
         elseif not expected_password then
            env.response = "HTTP/1.1 401 AUTHORIZATION REQUIRED"
            env.error_msg = "Unknown user"
         elseif password ~= expected_password then
            env.response = "HTTP/1.1 401 AUTHORIZATION REQUIRED"
            env.error_msg = "Invalid password"
         else
            log ("WEB-AUTH", "INFO", "%q logged in successfully", login)
         end
      end
   end
end

--------------------------------------------------------------------------------
-- Minimal usage sample --
--------------------------------------------------------------------------------
web.site['auth_sample.html'] = {
   header = web.authenticate_header { fabien="toto" },
   content = web.template 'default' {
      title = "Authentication test",
      body  = [[You are authenticated.]]
   }
}
