require 'sched'
require 'web.server'
require 'print'

local u = require "unittest"

local T = u.newtestsuite 'agent_datawrite'

local asset

local function stack_response(msg)
    local msg_str = m3da_serialize(msg)
    assert(socket.http.request{
               url    = cfg.server.."/stack_response",
               source = ltn12.source.string(msg_str) })
end

local function remote_test(src)
    local msg_str = m3da_serialize(msg)
    local snk, result = ltn12.sink.table{ }
    assert(socket.http.request{
               url    = cfg.server.."/test",
               source = ltn12.source.string(msg_str),
               sink   = snk
           })
    return dostring(table.concat(result))
end

local function connect()
    asset:connect()
end

function T :setup()
    asset = racon.newasset 'a'
    function asset.tree.commands.set(...)
        p{...}
    end

    u.assert(asset :start())
end

function T :teardown()
    asset :close()
end

function T :test_cmd1()
    store.foo=nil
    stack_response {
        __class  = 'Message',
        path     = 'a.commands',
        ticketid = 0,
        body = {
            set = { 'foo', 123 } } }
    connect()
    u.assert_equal(store.foo, 123)
end