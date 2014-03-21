-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local u = require 'unittest'
local config = require 'agent.config'
local tableutils = require 'utils.table'
local tm = require 'agent.treemgr'
local niltoken = require 'niltoken'
local loader = require'utils.loader'
local t=u.newtestsuite("config")
local tm_tree = require 'agent.treemgr.table'

local defaultconfig

function t:setup()
    config.default()
end

function t:test_meta1()
    config.tests = {meta1 = 42}
    u.assert_equal(config.tests.meta1, 42)
end

function t:test_meta2()
    config.tests = {meta2 = {t1 = 'toto', t2={t3={"a", "b"}}}}
    u.assert_equal(config.tests.meta2.t1, 'toto')
    u.assert_equal(config.tests.meta2.t2.t3[1], "a")
    u.assert_equal(config.tests.meta2.t2.t3[2], "b")
end

function t:test_default1()
    u.assert_no_error(config.default)
    defaultconfig = require 'agent.defaultconfig'
    loader.unload 'agent.defaultconfig'
    u.assert(#tableutils.diff(defaultconfig, config.get("")) == 0)
end

function t:test_set1()
    local s, r

    s, r = config.set("", 42)
    u.assert_nil(s)
    u.assert_string(r)

    s, r = config.set("tests.set1", 42)
    u.assert(s)
    u.assert_nil(r)
end

function t:test_getset1()
    local v = "someval"
    config.set("tests.getset1", v)
    u.assert_equal(v, config.get"tests.getset1")
end

function t:test_getset2()
    local v = {t1 = 'toto', t2={t3={"a", "b"}}}
    config.set("tests.getset2", v)
    u.assert_clone_tables(v, config.get"tests.getset2")
end


function t:test_diff1()
    config.default()
    config.tests={diff1=42}
    u.assert_clone_tables({"tests.diff1"}, config.diff())
    u.assert_clone_tables({"diff1"}, config.diff("tests"))
    u.assert_clone_tables({""}, config.diff("tests.diff1"))
end

function t:test_diff2()
    config.default()
    u.assert_clone_tables({}, config.diff())
end


function t:test_treemgr_proxy1()
    tm_tree.config.tests = {meta1 = 42}
    u.assert_equal(tm_tree.config.tests .meta1, 42)
end

function t:test_treemgr_proxy2()
    tm_tree.config.tests = {meta2 = {t1 = 'toto', t2={t3={"a", "b"}}}}
    u.assert_equal(tm_tree.config.tests.meta2.t1, 'toto')
    u.assert_equal(tm_tree.config.tests.meta2.t2.t3[1], "a")
    u.assert_equal(tm_tree.config.tests.meta2.t2.t3[2], "b")
end

function t:test_treemgr_set1()
    local s, r

    s, r = tm.set("config", 42)
    u.assert_nil(s)
    u.assert_string(r)

    s, r = tm.set("config.tests.set1", 42)
    u.assert(s)
    u.assert_nil(r)
end

function t:test_treemgr_getset1()
    local v = "someval"
    tm.set("config.tests.getset1", v)
    u.assert_equal(v, tm.get(nil, "config.tests.getset1"))
end

function t:test_treemgr_getset2()
    local v = {t1 = 'toto', t2={t3={"a", "b"}}}
    config.set("config.tests.getset2", v)
    u.assert_clone_tables(v, config.get"config.tests.getset2")
end

function t:test_treemgr_getset3()
    --test table overwrite with sub elements deleted.
    local v = {t1 = 'toto', t2={t3={"a", "b"}}}
    config.set("config.tests.getset3", v)
    local res = 33
    config.set("config.tests.getset3", res)
    local a, b =config.get"config.tests.getset3"
    u.assert_equal(res, a)
end

function t:test_treemgr_notify1()
    config.default()
    local vars
    local function notify(v) vars = v end
    local r = tm.register(nil, "config.tests", notify)

    tm.set("config.tests.notify1", 42)
    sched.wait()
    u.assert_clone_tables({["config.tests.notify1"]=42}, vars)

    tm.set("config.tests.notify1")
    sched.wait()
    u.assert_clone_tables({["config.tests.notify1"]=niltoken}, vars)

    tm.unregister(r)
end


function t:test_treemgr_notify2()
    config.default()
    local vars
    local function notify(v) vars = v end
    local r = tm.register(nil, "config.tests", notify)

    tm.set("config.tests.notify2", {t1=42, t2={"a", "b"}})
    sched.wait()
    u.assert_clone_tables({["config.tests.notify2.t1"]=42,
            ["config.tests.notify2.t2.1"]="a",
            ["config.tests.notify2.t2.2"]="b"}, vars)

    vars = nil
    tm.set("config.tests.notify2", {t1=42, t2={"a", "b"}})
    sched.wait()
    u.assert_nil(vars)

    vars = nil
    tm.set("config.tests.notify2", {t1=42, t2={[3]="c"}})
    sched.wait()
    u.assert_clone_tables({
            ["config.tests.notify2.t1"] = 42,
            ["config.tests.notify2.t2.3"]="c"}, vars)

    vars = nil
    tm.set("config.tests.notify2", nil)
    sched.wait()
    u.assert_clone_tables({["config.tests.notify2"]=niltoken}, vars)

    tm.unregister(r)
end


function t:test_treemgr_notify3()
    config.default()
    local vars
    local function notify(v) vars = v end
    local r = tm.register(nil, "config.tests", notify)

    tm.set("config.tests", {notify3=niltoken, ["notify3.t1"]=42})
    sched.wait()
    u.assert_clone_tables({["config.tests.notify3"]=niltoken, ["config.tests.notify3.t1"]=42}, vars)

    vars = nil
    tm.set("config.tests", {notify3=niltoken, ["notify3.t1"]=42})
    sched.wait()
    u.assert_nil(vars)

    vars = nil
    tm.set("config.tests", {notify3=niltoken})
    sched.wait()
    u.assert_clone_tables({["config.tests.notify3"]=niltoken}, vars)

    vars = nil
    tm.set("config.tests", {notify3=niltoken, ["notify3.t1"]=42})
    sched.wait()
    u.assert_clone_tables({["config.tests.notify3"] = niltoken ,["config.tests.notify3.t1"]=42}, vars)

    config.default()
    sched.wait()
    u.assert_clone_tables( { ["config.tests.notify3.t1"] = 42, ["config.tests.notify3"] = niltoken }, vars)

    tm.unregister(r)
end

function t:teardown()
    config.default()
end