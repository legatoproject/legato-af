require 'web.server'
require 'web.template'
local backend = require 'platform.backend'
require 'platform.response_templates'

local m3da = require 'm3da.bysant'
local function m3da_serialize(x)
    local s, t = m3da.serializer{ }
    assert(s(x))
    return table.concat(t)
end


function platform.enqueue_response(envelope)
    checks('table')
    backend.to_device(envelope)
end

--------------------------------------------------------------------------------
-- Common templates for all UI pages.
--------------------------------------------------------------------------------
web.templates.default = [[
${prolog}
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <title>${title}</title>
    <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
    <style type="text/css">
      h1,h2,h3,h4,h5,h6,th { color: #E53B30; font-family: Arial, Helvetica, sans-serif; }
      .error { color: red; font-weight: bold; }
      .code  { font-family : "courier new",courier,monospace; }
      table, td { border: 1px solid black; }
      td.action { border: 0px; }
      ${style}
     </style>
  </head>
  <body>
    <h1>${title}</h1>
    ${body}
  </body>
</html>
]]


--------------------------------------------------------------------------------
-- Main page.
--------------------------------------------------------------------------------
web.site[''] = web.template 'default' {
    title = [[M2M platform server simulator]],
    body = [[
    <ul>
      <li><a href='history'>See history</a></li>
      <li>Create a new response:
        <ul>
          <li><a href='response/edit'>From scratch</a></li>
          <% for k, v in pairs(platform.response_templates) do %>
          <li><a href='response/edit?template=<%=k%>'>From template '<%=k%>'</a></li>
          <% end %>
        </ul>
      </li>
    </ul>
]] }

--------------------------------------------------------------------------------
--
--------------------------------------------------------------------------------
web.site['config'] = web.template 'default' {
    title = [[M2M platform server simulator]],
    body = [[
      <p class='error'>Not implemented</p>
      <form action='/config/submit' method='POST'>
        <table>
          <tr><th></th><td><input/></td></tr>
        </table>
      </form>
]] }

web.site['config/submit'] = web.redirect('/config', function(env) end)

--------------------------------------------------------------------------------
-- Display all messages exchanged between server and agent(s).
--------------------------------------------------------------------------------
web.site['history'] = web.template 'default' {
    title = [[History of data exchanges]],
    body = [[
    <h2>Past exchanges</h2>
    <table>
      <tr><th>Date</th><th>Type</th><th>Content</th><th></th></tr>
      <% for i, item in ipairs(platform.history) do
           local date, is_resp, content = unpack(item)
           content = sprint(content)
           if #content>32 then content = content :sub(1, 29) .. '...' end
      %>
      <tr class='<%=is_resp and "response" or "request"%>'>
        <td><%=date%></td>
        <td><%=is_resp and "To device" or "From device"%></td>
        <td class='code'><a href='history/item?id=<%=i%>'><%=web.html.encode(content)%></a></td>
        <%if is_resp then%>
        <td class='action'><a href='/response/again?id=<%=i%>&url=/<%=env.url%>'>Resend</a></td>
        <td class='action'><a href='/response/edit?id=<%=i%>&url=/<%=env.url%>'>Edit</a></td>
        <%else%>
        <td class='action'></td><td class='action'></td>
        <%end%>
      </tr>
      <% end %>
     </table>

     <p> New...
       <% for k, v in pairs(platform.response_templates) do %>
       <a href='response/edit?template=<%=k%>'><%=k%></a>;
       <% end %>
     </p>
]] }


--------------------------------------------------------------------------------
-- Display one request or response.
--------------------------------------------------------------------------------
web.site['history/item'] = web.template 'default' {
    prolog = [[<%
      local id = tonumber(env.params.id)
      local date, is_resp, content = unpack(platform.history[id] or { }) %>]],
    title = [[<%= is_resp and "Response sent by server" or "Request received from asset" %> on <%=date%>]],
    body = [[<pre><%=web.html.encode(siprint(2, content))%></pre>
    <% if is_resp then %>
    <p><a href='/response/again?id=<%=id%>&url=/<%=env.url%>'>Send again</a></p>
    <% end %>
    <p><a href='/history'>Back to history</a></p>
]] }

--------------------------------------------------------------------------------
-- Legacy for tests.
--------------------------------------------------------------------------------
web.site['lastvalue'] = function(echo, env)
    error "Can't serve last value: not implemented"
end

--------------------------------------------------------------------------------
-- Resend a response already sent.
--------------------------------------------------------------------------------
web.site['response/again'] = {
    header = function (env)
        platform.enqueue_response(platform.history[tonumber(env.params.id)][3])
        env.response = "HTTP/1.1 302 REDIRECT"
        env.response_headers.Location = env.params.url
    end }

--------------------------------------------------------------------------------
-- Modify then send a response.
--------------------------------------------------------------------------------
web.site['response/edit'] = web.template 'default' {
    title = [[Create a response</title>]],
    prolog = [[<%
        local content
        if tonumber(env.params.id) then
            local item = platform.history[tonumber(env.params.id)]
            content = siprint(2, item[3])
        elseif env.params.template then
            content = platform.response_templates[env.params.template]
        elseif env.params.content then
            content = env.params.content
        else
            content = ''
        end
    %>]],
    body = [[
         <% if env.params.error then
             echo ("<p class='error'>"..env.params.error.."</p>")
         end %>
         <form action='/response/submit' method='POST'>
           <textarea cols='80' rows='25' value='source' id='source' name='source'>
<%=web.html.encode(content)%>
           </textarea>
           <p>
             <input type="submit" value="Enqueue for sending"/>
             <a href='<%=env.params.url or "/history"%>'>Cancel</a>
           </p>
         </form>
]] }

web.site['response/submit'] = {
    header = function(env)
        p(env.params)
        env.response = "HTTP/1.1 302 REDIRECT"
        local success, result
        local f, result = loadstring('return '..env.params.source)
        if f then success, result = pcall(f) end
        if success then -- success, redirect to history
            platform.enqueue_response(result)
            env.response_headers.Location = '/history'
        else -- error, redirect to editor with error msg
            result = result :gsub ("^%[string.*%]:(%d+):", "Error line %1:")
            local args = string.format("content=%s&error=%s",
                web.url.encode(env.params.source),
                web.url.encode(result))
            env.response_headers.Location = '/response/edit?'..args
        end
    end }

