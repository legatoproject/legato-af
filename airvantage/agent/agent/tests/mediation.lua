-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
--     Silvere Chabal     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local sched = require "sched"
local target = require "agent.mediation"
local srv = require "tests.mediationtestserver"
local config = require "agent.config"
local srvcon = require "agent.srvcon"
local u = require "unittest"

local t = u.newtestsuite("mediation")
local status = {}
local preconnecthook = nil

function t:setup()
    target.stop()
    config.mediation.servers = {{addr = "localhost", port = 8888}}
    srv.start("localhost", 8888, status)
    preconnecthook = srvcon.preconnecthook
end

function t:teardown()
    srv.stop()
    srvcon.preconnecthook = preconnecthook
end

function t:test_start()
    status.countkeepalive = 0
    status.countack = 0
    target.start(4)
    sched.wait(40)
    target.stop()
    u.assert_gte(4, status.countkeepalive)
    u.assert_gte(4, status.countack)
    status.countkeepalive = 0
    status.countack = 0
    target.start(4)
    sched.wait(40)
    target.stop()
    u.assert_gte(4, status.countkeepalive)
    u.assert_gte(4, status.countack)
    sched.wait(5)
end

function t:test_period()
    status.countkeepalive = 0
    status.countack = 0
    srv.clients = {}
    target.start(5)
    sched.wait(2)
    u.assert_equal("ok", srv.clients[1]:setperiod(1))
    sched.wait(32)
    target.stop()
    u.assert_gte(25, status.countkeepalive)
    u.assert_gte(25, status.countack)
    sched.wait(5)
end

function t:test_connect()
    srv.clients = {}
    target.start(5)
    sched.wait(2)
    srvcon.preconnecthook = function () config.toto = "ok" sched.signal("TEST-MEDIATION", "A") end
    config.toto = false
    u.assert_equal(false, config.toto)
    u.assert_equal("ok", srv.clients[1]:connect())
    sched.wait("TEST-MEDIATION", "A", 10)
    u.assert_equal("ok", config.toto)
    config.toto = "mimi"
    u.assert_equal("mimi", config.toto)
    u.assert_equal("ok", srv.clients[1]:connect())
    sched.wait("TEST-MEDIATION", "A", 10)
    u.assert_equal("ok", config.toto)
    sched.wait(10)
    target.stop()
    sched.wait(5)
end

function t:test_setaddress()
    srv.clients = {}
    target.start(5)
    sched.wait(5)
    local hostname = "m2mop.net"
    local port = 80
    local ip, stats = socket.dns.toip(hostname)
    u.assert_equal("ok", srv.clients[1]:setaddress(hostname, port))
    sched.wait(30)
    target.stop()
    u.assert_equal(ip, config.mediation.servers[1].addr)
    u.assert_equal(port, config.mediation.servers[1].port)
    u.assert_equal("localhost", config.mediation.servers[2].addr)
    u.assert_equal(8888, config.mediation.servers[2].port)
    sched.wait(5)
end
