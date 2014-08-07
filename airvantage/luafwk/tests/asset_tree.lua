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
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched  = require "sched"
local u = require "unittest"
local ltn12tostring = require "utils.ltn12.source".tostring
local racon  = require "racon"
local config = require "agent.config"
local asscon = require "agent.asscon"
local putils = require "utils.path"

local t = u.newtestsuite("asset_tree")

function t :setup()
    u.assert (racon.init())
    return "ok"
end

function t:test_createasset()
    local assets = {}

    --register
    for i=1, 10 do
        local newasset = racon.newasset("asset"..tostring(i))
        u.assert_not_nil(newasset)
        local status = newasset:start()
        u.assert_string(status)
        table.insert(assets, newasset)
    end

    --unregister
    for i,v in ipairs(assets) do
        u.assert_equal("ok", v:close())
    end
    assets = nil
end

-- Helper to build mock server messages
local function make_msg(Type, Path, Ticketid, Body)
    return {
        path = Path,
        body = Body,
        ticketid = Ticketid,
        type = Type,
    }
end

function t:test_asset_default_ReadNode()
    local assetid="test_asset_READNODE"
    --create/register
    local newasset = u.assert(racon.newasset(assetid))
    local status   = u.assert(newasset:start())

    --check initial status
    u.assert_not_nil(newasset.tree)
    u.assert_not_nil(newasset.tree.commands)
    u.assert_function(newasset.tree.commands.__default)
    u.assert_function(newasset.tree.commands.ReadNode)

    --Notes:
    -- - following ReadNode cases use the ReadNode command format as used by the AirAvantage server
    --  as the date of 2014-04-23
    -- - `make_msg` first argument is `Type`, which is not valid anymore since M3DA (AWTDA2 remains ...) and EMP
    -- modifications. It should be replaced by `__class` field, however as racon.asset.tree defaults to data writing behavior
    -- that's no big deal.

    --simple success case: read one leaf that exists
    newasset.tree.foo = "bar"
    local cmdpath = putils.concat(assetid, "commands.ReadNode")
    local payload = make_msg(2, cmdpath , 4242, { "foo" })
    local status, err = asscon.sendcmd(assetid, "SendData", payload)
    u.assert_equal(0, status, err)

    --2nd success case: read two leafs that exists, more complicated path
    newasset.tree.foo2 = { }
    newasset.tree.foo2.bar1 = "foofoo"
    newasset.tree.foo2.bar2 =  { bar3 = "foofoo"}
    local payload = make_msg(2, cmdpath, 4243, {"foo2.bar1", "foo2.bar2.bar3" })
    local status, err = asscon.sendcmd(assetid, "SendData", payload)
    u.assert_equal(0, status, err)

    --3nd success case: read a (interior) node that exists
    newasset.tree.foo3 = { }
    newasset.tree.foo3.bar1 = "foofoo"
    newasset.tree.foo3.bar2 =  { bar3 = "foofoo"}
    local payload = make_msg(2, cmdpath, 4243, {"foo3" })
    local status, err = asscon.sendcmd(assetid, "SendData", payload)
    u.assert_equal(0, status, err)

    --error case: read one node that doesn't exists (part of the part does exist)
    newasset.tree.foo3 = { node = "hello" }
    local payload = make_msg(2, cmdpath, 4244, { "foo3.absentvar"} )
    local status, err = asscon.sendcmd(assetid, "SendData", payload)
    u.assert_equal(-1, status, err)
    u.assert_match("path not found", err, "unexpected error, expecting not found error")

    u.assert(newasset:close())
end


function t:test_asset_tree_command()
    --create/register
    local newasset = u.assert(racon.newasset("command"))
    local status   = u.assert(newasset:start())

    --configure
    u.assert_not_nil(newasset.tree)
    u.assert_not_nil(newasset.tree.commands)

    --commands
    u.assert_function(newasset.tree.commands.__default)
    u.assert_function(newasset.tree.commands.ReadNode)

    --readnode without argument, first format
    local payload = make_msg(2, 'command.commands.ReadNode', 41, {""})

    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_equal(0, status, err)

     --readnode without argument, second format
    local readn =  nil
    newasset.tree.commands.ReadNode = function(...) readn = "ok"; return "ok" end
    local payload = make_msg(2, 'command', 41, { [ "commands.ReadNode" ] = "" })

    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_equal(0, status, err)
    u.assert_equal(readn, "ok")

    --dummy: non existing cmd, first format
    local payload = make_msg(2, 'command.commands.Dummy', 42, {""})

    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_not_equal(0, status, err) -- must be an error

    --dummy: non existing cmd, second format
    local payload = make_msg(2, 'command', 42, { [ "commands.Dummy" ] = "" })
    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_not_equal(0, status, err) -- must be an error

    --register commands
    config.asset_tree = {default = false, verifyexec = false}
    newasset.tree.mycommands = {}
    newasset.tree.mycommands.__default = function (asset, values, path)
        config.asset_tree.default = values
        return "ok"
    end
    newasset.tree.mycommands.VerifyExec = function (asset, values, path)
        config.asset_tree.verifyexec = values
        return "ok"
    end
    local payload = make_msg(5, "command.mycommands.VerifyExec", 43, {
        d = "2",
        r = 2 })

    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_equal(0, status, err)

    u.assert_equal(2, config.asset_tree.verifyexec.r)
    u.assert_equal("2", config.asset_tree.verifyexec.d)

    local payload = make_msg(5, "command.mycommands.Dummy", 44, { viva='lasvegas', d=2 })
    local status, err = asscon.sendcmd("command", "SendData", payload)
    u.assert_equal(0, status, err)
    u.assert_equal("lasvegas", config.asset_tree.default.Dummy.viva)
    u.assert_equal(2, config.asset_tree.default.Dummy.d)

    --unregister
    u.assert(newasset:close())
    newasset = nil
end

function t:test_asset_tree_variable()
    --create/register
    local newasset = racon.newasset("variable")
    u.assert_not_nil(newasset)
    local status = newasset:start()
    u.assert_equal("ok", status)
    --configure
    u.assert_not_nil(newasset.tree)
    u.assert_not_nil(newasset.tree.commands)

    --datawriting
    local payload = make_msg(5, 'variable', 3, {
        ['test.insert.value'] = -1.5,
        ['test.another.value'] = 'ok' })

    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --readnode
    local payload = make_msg(2, 'variable.ReadNode', 13, { "test" })
    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --check tree.test.insert.value = -1.5, tree.test.another.value = "ok"
    u.assert_equal(-1.5, newasset.tree.test.insert.value)
    u.assert_equal("ok", newasset.tree.test.another.value)

    local payload = make_msg(5, "variable.deeper.insert", 4, {
        ['test.insert.value'] = 1.5e-1,
        ['test.another.value'] = 'la fete' })
    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --readnode
    local payload = make_msg(2, 'variable.ReadNode', 14, { 'deeper' })
    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --check tree.test.insert.value = -1.5, tree.test.another.value = "ok"
    u.assert_equal(1.5e-1, newasset.tree.deeper.insert.test.insert.value)
    u.assert_equal("la fete", newasset.tree.deeper.insert.test.another.value)

    local payload = make_msg(5, "variable", 5, {
        ['test.insert.value'] = 4,
        ['test.another.field'] = 'ok' })
    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --readnode
    local payload = make_msg(2, "variable.ReadNode", 15, { "test" })
    local status, err = asscon.sendcmd("variable", "SendData", payload)
    u.assert_equal(0, status)

    --check tree.test.insert.value = -1.5, tree.test.another.value = "ok"
    u.assert_equal(4, newasset.tree.test.insert.value)
    u.assert_equal("ok", newasset.tree.test.another.value)
    u.assert_equal("ok", newasset.tree.test.another.field)

    --unregister
    u.assert_equal("ok", newasset:close())
    newasset = nil
end

function t:test_asset_connectreboot()
    --create/register
    local newasset = u.assert(racon.newasset("connectreboot"))
    u.assert(newasset :start())
    local res, err = racon.connecttoserver()

    -- We assume that getting a message "connection already in progress" is not a real error.
    -- All other messages are considered as critical
    if not res and type(err) == "string" and  err:match(": (.*)]") ~= "connection already in progress" then
       u.assert(nil)
    end

    local system = require 'racon.system'
    system.init()
    u.assert(system.reboot())

    --unregister
    u.assert(newasset:close())
    newasset = nil
end
