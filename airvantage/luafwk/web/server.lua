--------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- and Eclipse Distribution License v1.0 which accompany this distribution.
--
-- The Eclipse Public License is available at
--   http://www.eclipse.org/legal/epl-v10.html
-- The Eclipse Distribution License is available at
--   http://www.eclipse.org/org/documents/edl-v10.php
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

require 'socket'

if global then global "web" end
web = web or { }
web.site = web.site or {[""]="Nothing in httproot, try <a href='/map.html'>map</a>"}
web.pattern = {}

-- For backward compatibility
WEBSITE = web.site

-------------------------------------------------------------------------------
-- web.start()
-------------------------------------------------------------------------------
-- Start the web server. If an instance of it was previously running,
-- close it first.
-- All the server does is call [handle_connection] whenever a client connects
-- to it, and close the client socket when the peer closes or crashes.
-------------------------------------------------------------------------------
function web.start(port)
    if web.server_socket then
        log ('WEB', 'INFO', "Shutting down previous web server")
        web.server_socket :close()
    end
    log ('WEB', 'INFO', "Starting web server on port %d", port or 80)
    -- WARNING: oatlua-specific. Adapt it to run with Copas under regular OSes
    web.server_socket = socket.bind ('*', port or 80, web.handle_connection)
end

-------------------------------------------------------------------------------
-- web.handle_connection ()
-------------------------------------------------------------------------------
-- Loop to read a request and handle it, until the connection is closed from
-- somewhere else. Computing and sending the actual response is delegated to
-- the appropriate function in the table [handle_method].
-------------------------------------------------------------------------------
-- [url]:              complete URL, with parameters
-- [url_params]:       part of the URL after "?", if any
-- [handler]:          function in charge of serving the resource
-- [env]:              sandbox in which the page serving functions will run
-- [env.method]:       PUT/GET/POST/DEL/HEAD
-- [env.headers]:      name->value dictionary of headers set in the request
-- [env.params]:       name->value dictionary of URL-encoded parameters
-- [env.url]:          URL, without parameters
-- [env.channel]:      channel which received request and should send the response.
-- [env.mime_type]:    mime type, determined from the URL base's extension
-- [env.http_version]: protocol version, presumably one of '0.9', '1.0' or '1.1'
-------------------------------------------------------------------------------
function web.handle_connection (cx)
    log('WEB', 'DETAIL', "Connection opened")
    while true do
        log('WEB', 'INFO', "Connection waiting for a request on %s", tostring(cx))

        -- Set environment variables from the request
        local url, url_params, version, ext
        local line, msg = cx :receive '*l'
        if not line then
            log('WEB', 'INFO', "Ending connection: socket %s", msg)
            cx :close()
            break
        end
        log('WEB', 'DETAIL', "Got request %q", tostring(line))
        local env = { channel = cx; request_headers = { } }
        env.method, url, env.http_version = line :match  "^(%S+) (%S+) (%S+)"
        if not env.method or not url or not env.http_version then
            log('WEB', 'ERROR', "Invalid HTTP request %s", line)
            cx: close()
            break
        end
        env.url, url_params = url :match "/([^%?]*)%??(.*)"
        ext = env.url :match "%.(%w+)$" -- extension (to guess mime type)
        env.mime_type = ext and web.mime_types[ext] or web.mime_types["<default>"]
        env.params = web.url.decode (url_params)

        -- Parse headers
        repeat
            local key, value
            local line, msg = cx :receive '*l'
            if line then key, value = line :match "^([^:]+): (.*)\r?\n?$" end
            if key  then env.request_headers[key:lower()] = value end
        until not key

        web.handle_request(cx, env)
    end
end

-------------------------------------------------------------------------------
-- web.handle_request()
-------------------------------------------------------------------------------
-- Respond to a request from the client.
-------------------------------------------------------------------------------
function web.handle_request (cx, env)
    log('WEB', 'DETAIL', "Handling %s request for %q", env.method, env.url)

    local h = { } -- response headers
    env.response_headers = h

    local page = web.site[env.url]
    if not page then
        for pattern, map in pairs(web.pattern) do
            if pattern ~= "" and env.url:match(pattern) then
                page = map
                break
            end
        end
    end

    if not page then
        web.handle_request_body (env) -- Still flush request body from socket
        web.send_error (env, 404, "No handler found for " .. env.url); return
    end
    if type(page) == 'table' and page[env.method] then page = page[env.method] end
    if type(page) ~= 'table' then page={content=page} end
    local content = page.content or page[1]
    if not content and not page.header then
        web.handle_request_body (env) -- Still flush request body from socket
        web.send_error (env, 404, "HTTP/" .. env.method .. " is not supported for " .. env.url); return
    end

    env.page = page

    local res, err = web.handle_request_body (env)

    if not res and type(err) == "string" then
        web.send_error (env, 500, err)
        -- Connection closing or survival
        if h['Connection']=='close' then cx :close() end
        return
    end

    content = content or ''
    local static_page = type(content)=='string'

    -- Setup default response and headers
    h["Content-Type"] = page.mime_type or env.mime_type
    h["Connection"]   = env.request_headers.Connection or "keep-alive"
    if static_page then h["Content-Length"] = #content
    else h["Transfer-Encoding"] = "chunked" end

    -- Setting the default response status
    env.response = "HTTP/1.1 200 OK"
    -- execute the header function, if any
    local hf = page.header
    if hf then
        assert(type(hf)=='function')
        hf(env)
        if env.response ~= "HTTP/1.1 200 OK" then
            cx :send (env.response.."\r\n")
            for k,v in pairs (env.response_headers) do cx :send (k..': '..v..'\r\n') end
            cx :send '\r\n' -- end of headers
            return
        end
    end

    if static_page then
        -- Send the response and headers
        cx :send (env.response.."\r\n")
        for k,v in pairs (env.response_headers) do cx :send (k..': '..v..'\r\n') end
        cx :send '\r\n' -- end of headers
        cx :send (content) -- send the static page
    else
        local err = env.error_msg
        if err then
            cx :send (string.format ("%X\r\n%s\r\n0\r\n\r\n", #err, err))
            return
        end
        local headers_sent = false
        local function echo(...)
            if not headers_sent then
                cx :send (env.response.."\r\n")
                for k,v in pairs (env.response_headers) do cx :send (k..': '..v..'\r\n') end
                cx :send '\r\n' -- end of headers
                headers_sent = true
            end
            local chunk = table.concat{...}
            if #chunk>0 then
                cx :send (string.format ("%X\r\n", #chunk)..chunk.."\r\n")
            end
        end

        res = nil
        err = nil
        res, err = content(echo, env)
        if not res and type(err) == "string" then
            web.send_error (env, 500, err)
        else
            cx :send "0\r\n\r\n" -- Send the terminating empty chunk
        end
    end

    -- Connection closing or survival
    if h['Connection']=='close' then cx :close() end
end

web.url = { }

-------------------------------------------------------------------------------
-- web.url.decode()
-------------------------------------------------------------------------------
-- Decode URL-encoded data: fields are separated with "&", names are separated
-- from their value with "=", all characters requiring an escaping are encoded
-- as "%xx", with "xx" being two hexadecimal digits representing the char's
-- ASCII code.
-- Returns a name->value dictionary.
-------------------------------------------------------------------------------
function web.url.decode(txt)
    checks('string')
    local r = { }
    local function h2k(h)   return string.char (tonumber (h, 16)) end
    local function unesc(x) return x:gsub("+"," "):gsub("%%(%x%x?)", h2k) end
    for k, v in txt :gmatch "([^&=]+)=([^&=]+)" do r[unesc(k)] = unesc(v) end
    return r
end

-------------------------------------------------------------------------------
-- web.url.encode()
-------------------------------------------------------------------------------
-- Translates all non-word characters into their "%xx" escaped form, with
-- x hexadecimal numbers.
-------------------------------------------------------------------------------
function web.url.encode(x)
    checks('string|number|table')
    local function esc(x) return string.format("%%%x", string.byte(x)) end
    if type(x) == 'number' then return tostring(x) end
    if type(x) == 'string' then return x:gsub("%W", esc)
    else
        local r = { }
        for k, v in ipairs(x) do
            table.insert (r, k:gsub("%W", esc) .. "=" .. v:gsub("%W", esc))
        end
        return table.concat(r,'&')
    end
end

web.html = { }

-------------------------------------------------------------------------------
-- web.html.encode()
-------------------------------------------------------------------------------
-- Escape special characters ' " < > & into the corresponding HTML entities
-------------------------------------------------------------------------------
function web.html.encode(txt)
    checks('string')
    local entities = {
        ['<'] = '&lt;',   ['>'] = '&gt;', ['"'] = '&quot;',
        ["'"] = '&apos;', ["&"] = '&amp;'}
    return (txt :gsub ("[\"'<>&]", entities))
end

-------------------------------------------------------------------------------
-- web.send_error()
-------------------------------------------------------------------------------
-- Send an error message to the client.
-------------------------------------------------------------------------------
function web.send_error (env, status, txt)
    env.channel :send ("HTTP/1.1 " .. status .. " " .. txt .. "\r\n" ..
        "Content-Type: text/html; charset=iso-8859-1\r\n" ..
        "Connection: close\r\n" ..
        "Content-Length: " .. #txt .. "\r\n\r\n" ..
        txt)
    env.channel :close()
end

-------------------------------------------------------------------------------
-- Read a chunked body from a socket, return the recomposed content or nil+msg.
-- TODO: woudln't it be more consistent to pass a complete env?
-------------------------------------------------------------------------------
function web.read_chunks(skt)
    local chunks = { }
    repeat
        local length, chunk, msg
        length, msg = skt :receive '*l'
        if not length then return nil, msg end
        length = tonumber(length, 16)
        if not length then return nil, "bad chunk lenght" end
        if length > 0 then
            chunk, msg = skt :receive(length)
            if not chunk then return nil, msg end
            table.insert (chunks, chunk)
        end
        chunk, msg = skt :receive(2)
        if chunk ~= "\r\n" then return nil, "bad chunk ending" end
    until length == 0
    return table.concat (chunks)
end

-------------------------------------------------------------------------------
-- Try to read POST parameters, put them in env.params;
-- keep the raw body in env.body.
-------------------------------------------------------------------------------
function web.handle_request_body(env)
    -- TODO: support chunked encoding
    local rh = env.request_headers
    local h_cl, h_te =
        rh['content-length'],
        rh['transfer-encoding']
    local len = h_cl and tonumber(h_cl)
    local data, msg, res

    if len and len>0 then
        log( 'WEB', 'INFO', "Getting %d bytes of request body", len)
        if env.page and env.page.sink then
            local src = socket.source("by-length", env.channel, len)
            res, msg = ltn12.pump.all(src, env.page.sink)
            if not res then log("WEB", "ERROR", "Body decoding error while sending data to page sink (by-length)") end
        else
            data, msg = env.channel :receive (len)
        end
    elseif h_te == "chunked" or rh.te and rh.te :match "chunked" then
        log( 'WEB', 'DETAIL', "Decoding chunk-encoded request body because H=%s", sprint(rh))
        if env.page and env.page.sink then
            log('WEB', 'DETAIL', "Fed to an LTN12 sink handler")
            local src = socket.source("http-chunked", env.channel, env.request_headers)
            res, msg = ltn12.pump.all(src, env.page.sink)
            if not res then log("WEB", "ERROR", "Body decoding error while sending data to page sink (http-chunked)") end
        else
            data, msg = web.read_chunks (env.channel)
        end
    elseif env.method == 'GET' then
        log('WEB','DETAIL', "No body in PUT request")
    else
        log('WEB','WARNING', "No body found in %s request; headers = %s", env.method, siprint(2, rh))
    end

    if not env.page or not env.page.sink then
        env.body = data
        local ct = rh['content-type']
        if data and ct and ct :match 'urlencoded' then env.params = web.url.decode(data) end
    end
    return "ok"
end

-------------------------------------------------------------------------------
-- Generate a page which executes `f(env, ...)' if `f' is not nil,
-- then redirects to `url'.
-- Usage: web.site['/dostuff'] = web.redirect('/thengothere.html', dostuff)
-------------------------------------------------------------------------------
function web.redirect(url, f, ...)
    local args = {...}
    return { header = function (env)
        if f then f(env, unpack(args)) end
        env.response = "HTTP/1.1 302 REDIRECT"
        env.response_headers.Location = url
    end }
end

-------------------------------------------------------------------------------
-- URL extension ==> Mime type associations
-------------------------------------------------------------------------------
web.mime_types = {
    html = "text/html",
    rss  = "application/rss+xml",
    png  = "image/png",
    gif  = "image/gif",
    jpg  = "image/jpg",
    jpeg = "image/jpg",
    ico  = "image/ico",
    js   = "text/javascript",
    lua  = "text/lua",
    ["<default>"] = "text/html" }

-------------------------------------------------------------------------------
--
-- AJAX PLUGIN
--
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Browser-side RPC code. This will be linked from AJAX-enabled pages.
-------------------------------------------------------------------------------
web.site["ajax-rmi.js"] = [[
   function new_request() {
      var A;
      try { A = new ActiveXObject("Msxml2.XMLHTTP"); } catch (e) {
        try { A = new ActiveXObject("Microsoft.XMLHTTP"); } catch (oc) {
          A = null; } }
      if (!A && typeof XMLHttpRequest != "undefined") A = new XMLHttpRequest();
      if (!A) alert("Could not create AJAX connection object.");
      return A;
    }
    function ajax_rmi(func_name, args) {
      var i, x, n, url;
      url = "ajax?f="+func_name
      for (i = 1; i < args.length; i++) {
        var arg = escape(args[i-1]).replace(/\+/g, "%2b")
        url = url+"&"+i+"="+arg;
      }
      x = new_request();
      x.open( "GET", url, true);
      x.onreadystatechange = function() {
        if (x.readyState != 4) return;
        //alert( "Received " + x.responseText);
        var f = args [args.length-1]
        if(f) { f(x.responseText); }
      }
      x.send(null);
      delete x;
    }
]]

web.ajax = { }
web.ajax.exported_functions = { }

-------------------------------------------------------------------------------
-- Serve exported functions in response to XMLHTTPREQUESTS
-------------------------------------------------------------------------------
web.site['ajax'] = function (echo, env)
    local func_name = env.params.f
    local f = web.ajax.exported_functions[func_name]
    if not f then
        return nil, "Remote invocation of non-exported function : " .. (func_name or "nil")
    end

    local args = { }
    for i = 1, 1024 do
        local a = env.params[tostring(i)]
        if not a then break end
        args[i] = a
    end

    --print("call with args") p(args)

    -- TODO: f was setfenv'd; determine if/how it should
    -- access env (probably as a 1st param).
    local result = f(unpack(args))
    if result ~= nil then env.echo(result) end
end

-------------------------------------------------------------------------------
-- Mark (a) Lua function(s) as AJAX-importable.
-- The function must be stored in (a) global variable(s),
-- pass its (their) name(s) as string parameter(s).
-------------------------------------------------------------------------------
function web.ajax.export(...)
    local args, i = {...}, 1
    while i<=#args do
        local name = args[i]
        local f = i<#args and args[i+1]
        if type(f) == 'function' then i=i+2 else f,i = getfenv()[name],i+1 end
        assert (type(f)=='function', "Can only AJAX-export functions")
        web.ajax.exported_functions[name] = f
    end
end

-------------------------------------------------------------------------------
-- Import a list of functions in a dynamic page
-- For security reasons, these functions must have been exported with
-- web.ajax.export() before being imported in a web page.
-------------------------------------------------------------------------------
function web.ajax.import (env, ...)
    env.echo "<script src='/ajax-rmi.js'></script>\n"
    env.echo "<script type='text/javascript'> /* Imported Lua functions: */\n"
    for _, name in ipairs{...} do
        local f = web.ajax.exported_functions[name]
        if not f then error ("Can't import this function: "..name) end
        local txt = string.format('function %s(){ajax_rmi("%s", %s.arguments);}\n',
            name, name, name)
        env.echo (txt)
    end
    env.echo "</script>"
end

-------------------------------------------------------------------------------
-- List all pages available on the web site. You might want to
-- disable this for security reasons, with web.site['map.html']=nil
-------------------------------------------------------------------------------
web.site['map.html'] = function (echo, env)
    echo [[
<html>
  <head><title>Site map</title></head>
  <body><h1>Site map</h1>
    <ul>
]] local names = { }
    for name in pairs(web.site) do table.insert (names, name) end
    table.sort (names)
    for _, name in ipairs (names) do
        echo("<li><a href='/", name, "'>", #name>0 and name or "[home]", "</a></li>\n")
    end
    echo [[
    </ul>
  </body>
</html>]]
end

-- relaunch the server if it was allready running:
if web.server_socket then
    local _, port = web.server_socket :getsockname()
    web.start(port)
end

return web