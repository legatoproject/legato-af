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
-- Session handling back-end
--
-- This module demonstrates how to attach persistant data to a user session:
-- if several users, from several browsers, dialog with the server, each user
-- will have its own session table, which he will retrieve every time a page
-- is served to him, and which is distinct from all other users' sessions.
--
-- The mechanism is not secure, i.e. a malicious user could rather easily access
-- the session data of other users. Consider implementing random session id
-- and/or encryption with a shared secret key if you need to handle
-- confidentiality. In this case, using HTTPS might also be a must.
--
--------------------------------------------------------------------------------

require 'web.server'
require 'web.template'

SESSIONS = { }

--------------------------------------------------------------------------------
-- Session keys are consecutive integers. This is absolutely not secure.
-- For any usage where anyone might wish to hijack someone else's session,
-- you must use hard-to-guess keys.
--------------------------------------------------------------------------------
local sidmax = 0

--------------------------------------------------------------------------------
-- Must be called during headers handling.
-- session argument is optional, defaults to { }
--------------------------------------------------------------------------------
function new_session (env, session)
   session = session or { }
   sidmax = sidmax+1
   local key = "@"..sidmax
   SESSIONS [key] = session
   env.response_headers['Set-Cookie'] = key
   return session
end

function get_session (env)
   local key = env.request_headers.cookie
   if key then return SESSIONS [key] end
end

function reset_session (env)
   local key = env.request_headers.cookie
   SESSIONS [key] = nil
end

--------------------------------------------------------------------------------
-- Session handling front-end
--------------------------------------------------------------------------------

-- Uncomment this to make home.html the default page --
-- web.site[''] = web.redirect '/home.html'

-- Describe a new session to create --
web.site['new-session.html']= {
   header = new_session,
   content = web.template 'default' {
      title = "Creating a new session",
      body  = [[
         <p>Create a new session. Enter your name in the field.</p>
         <form action='post-new-session' method='POST'>
           <label>Your name:</label>
           <input type='text' id='name' name='name'/>
           <input type="submit"/>
         </form>
      ]] } }

-- Fill the newly created session with browser-provided data --
web.site['post-new-session'] = web.redirect ('/home.html', function (env)
   local session = get_session(env)
   session.date = os.date()
   session.name=env.params.name
end)

-- Destroy a session --
web.site['logout'] = web.redirect('/home.html', reset_session)

-- Add arbitrary data to the session --
web.site['add-session-data'] = web.redirect('/home.html', function(env)
   local s, k, v = get_session(env), env.params.key, env.params.value
   if s and k then s[k] = v end
end)


-- Main demo page --
web.site['home.html'] = {
   content = web.template 'default' {
      title = 'Session demonstration',
      body = [[
        <% local session = get_session(env) %>
        <% if session then %>
          <p>Hello <%=session.name%>, your session has been created on <%=session.date%></p>
          <ul>
            <li> <a href='/logout'>Log out</a></li>
            <li>
               <form action='add-session-data' method='POST'>
                  Add data
                  <label>key:</label>
                  <input type='text' id='key' name='key'/>
                  <label>value:</label>
                  <input type='text' id='value' name='value'/>
                  <input type="submit" value='add'/>
               </form>
             </li>
          </ul>

          <h2>Session data</h2>
          <table border=2><tr><th>key</th><th>value</th></tr>
            <% for k, v in pairs(session) do %>
              <tr><td><%= k %></td><td><%= v %></td></tr>
            <% end %>
          </table>
        <% else %>
          <p>You are not loged in.</p>
          <p><a href='/new-session.html'>Create new session</a></p>.
        <% end %>
        ]] } }
