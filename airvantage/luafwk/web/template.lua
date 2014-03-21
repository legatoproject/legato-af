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
-- Simple web page Template System.
--
-- This module defines the `template' function, which lets you define web page
-- templates with variables in it. Then, a page can be built from templates,
-- by providing a value for each variable.
--
-- * Templates are described as strings, indexed by name
--   in global table TEMPLATES; users are expected to provide additional samples
--   in this table;
--
-- * In a template, variables are represented by an identifier, surrounded with
--   braces and prefixed with a dollar. For instance, variable "foo" is
--   represented as "${foo}";
--
-- * A variable can occur zero, one or several tiimes in a template;
--
-- * Template application is performed by function template();
--
-- * the template() function forces the precompilation of the web page:
--    - it is intended to be called from inside table braces;
--    - it performs variable substitutions immediately, not at 1st request;
--    - it extracts and precompile code extracts within "<%...%>" markers;
--   cf. function template() for a usage sample.
--------------------------------------------------------------------------------

require 'web.server'
require 'web.compile'

web.templates = { }

web.templates.default = [[
${prolog}
<html>
  <head>
    <title>${title}</title>
  </head>
  <body>
    <h1>${title}</h1>
    ${body}
  </body>
</html>
]]

--------------------------------------------------------------------------------
-- With this change enabled in string metatables, the "%" modulo operator
-- performs replacements in strings, taken from a table. For instance,
-- ``"foo ${bar} bar" % { bar = "gnu" } '' evaluates to "foo gnu bar".
--
-- This mechanism is available globally, but is primarily used to perform
-- template replacements.
--------------------------------------------------------------------------------
getmetatable '' .__mod = function (self, replacements)
   return string.gsub (self, "%${([%w_]+)}", replacements)
end

--------------------------------------------------------------------------------
-- Usage sample:
-- web.site['foo.html'] =
--   { template 'default' { title="foo", body="<p>bar</p>" } }
--------------------------------------------------------------------------------
function web.template (name)
    local t = web.templates [name] or error "No such template"
    return function (args)
        local parsed_args = { }
        for k, v in pairs(args) do
            local t = type(v)
            if t=='nil' then v=''
            elseif t=='table' then v=web.html.encode(siprint(2, v))
            else v=tostring(v) end
            parsed_args[k] = v
        end
        setmetatable(parsed_args,{__index=function() return '' end})
        return web.compile (t % parsed_args, parsed_args.title)
    end
end

return web.template
