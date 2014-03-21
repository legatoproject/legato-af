require 'web.server'
platform = { }

backend = require 'platform.backend'
last_message=nil

-- Insert a response, to be served as a response to the next connection
function web.site.stack_response(echo, env)
    p(env)
end

-- Execute arbitrary remote Lua code. Don't put in prod... :)
function web.site.test(echo, env)
    local src = env.body
    local x = assert(loadstring(src))()
    echo(sprint(x))
end

local function from_device(msg)
    -- message received from device
    table.insert(messages, msg)
    last_message = msg
end

local function init(cfg)
    web.start(8070)
    backend.init(cfg, from_device)
end

return init