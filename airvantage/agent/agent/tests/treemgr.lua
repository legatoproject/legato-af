-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot        for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local u          = require 'unittest'
local pathutils  = require 'utils.path'
--local tableutils = require 'utils.table'
local treemgr    = require 'agent.treemgr'
local niltoken   = require 'niltoken'
local treemgr_table = require 'agent.treemgr.table'

local t = u.newtestsuite("treemgr")

local function assert_table_eq_no_order(t1, t2)
    checks('table', 'table')
    local tk1, tk2 = { }, { }
    for _, x in ipairs(t1) do tk1[x]=true end
    for _, x in ipairs(t2) do tk2[x]=true end
    u.assert_clone_tables(tk1, tk2)
end

-- setup:
-- two handlers, ramstore1 and ramstore2, mounted as tests.ramstore1 and
-- tests.ramstore2.
function t :setup()
    require 'tests.treemgr_table1'
    require 'tests.treemgr_table2'
    table1.numbers = {
        fortytwo = 42,
        thirties = {
            thirtyone = 31,
            thirtythree = 33 } }
    table2.key='value'
end

-- get: single value, success
function t :test_get1()
    local a, b = treemgr.get(nil, 'tests.x.table1.numbers.fortytwo')
    u.assert_equal(a, 42)
    u.assert_nil(b)
end

-- get: inexistent value in a table handler
function t :test_get_inexistent_in_handler()
    local a, b = treemgr.get(nil, 'tests.x.table1.whatever')
    u.assert_nil(a)
    u.assert_nil(b)
end

-- get: inexistent value out of hanlder
function t :test_get_inexistent_out_of_handler()
    local a, b = treemgr.get(nil, 'whatever.whatever.whatever')
    u.assert_nil(a)
    u.assert_string(b)
end

-- get: non-leaf node, travel recursively towards leaves, through proxy
function t :test_get_recursive_proxy()
    local n = treemgr_table.tests.x.table1.numbers
    u.assert_clone_tables(n, table1.numbers)
end

-- get: multiple values, checked directly rather then through a table proxy
function t :test_get_recursive_raw()
    local n_nil, n_children = treemgr.get(nil, 'tests.x.table1.numbers')
    u.assert_nil(n_nil)
    assert_table_eq_no_order(n_children, {
        'tests.x.table1.numbers.thirties',
        'tests.x.table1.numbers.fortytwo',
    })
    n_nil, n_children = treemgr.get(nil, 'tests.x.table1.numbers.thirties')
    u.assert_nil(n_nil)
    assert_table_eq_no_order(n_children, {
        'tests.x.table1.numbers.thirties.thirtyone',
        'tests.x.table1.numbers.thirties.thirtythree',
    })
    local n
    n, n_nil = treemgr.get(nil, 'tests.x.table1.numbers.fortytwo')
    u.assert_equal(n, 42)
    u.assert_nil(n_nil)
    n, n_nil = treemgr.get(nil, 'tests.x.table1.numbers.thirties.thirtyone')
    u.assert_nil(n_nil)
    u.assert_equal(n, 31)
    n, n_nil = treemgr.get(nil, 'tests.x.table1.numbers.thirties.thirtythree')
    u.assert_nil(n_nil)
    u.assert_equal(n, 33)
end

-- get: multiple values across several handlers
function t :test_twohandlers()
    local a, b = treemgr.get(nil, {'tests.x.table1.numbers.fortytwo', 'tests.y.table2.key'})
    u.assert_clone_tables(a, {
        ['tests.x.table1.numbers.fortytwo']=42,
        ['tests.y.table2.key']='value' })
    u.assert_clone_tables(b, { })
end

-- get: find path toward mountpoints below
function t :test_mount_paths()
    local a, b = treemgr.get(nil, 'tests')
    u.assert_nil(a)
    assert_table_eq_no_order(b, {
        'tests.x',
        'tests.y',
        'tests.z',
        'tests.w' })
end

-- set: single value, with a variety of prefix/reacord key combinations
function t :test_set1()
    u.assert(treemgr.set('tests.x.table1.junk.key', 1))
    u.assert_equal(table1.junk.key, 1)
    u.assert(treemgr.set('tests.x.table1.junk', {key=2}))
    u.assert_equal(table1.junk.key, 2)
    u.assert(treemgr.set('tests.x.table1', {['junk.key']=1}))
    u.assert_equal(table1.junk.key, 1)
    u.assert(treemgr.set('', {['tests.x.table1.junk.key']=2}))
    u.assert_equal(table1.junk.key, 2)
    u.assert(treemgr.set('tests.x.table1.junk.key', {['']=1}))
    u.assert_equal(table1.junk.key, 1)
end

-- set: multiple values, in a record, in the same handler
function t :test_multi_1_handler()
    u.assert(treemgr.set('tests.x.table1.junk', {key=3, board=4}))
    u.assert_equal(table1.junk.key, 3)
    u.assert_equal(table1.junk.board, 4)
end

-- set: multiple values, in a record, in several handlers
function t :test_multi_2_handlers()
    u.assert(treemgr.set('tests', {
        ['x.table1.junk.key']=5,
        ['y.table2.junk.key']=6 }))
    u.assert_equal(table1.junk.key, 5)
    u.assert_equal(table2.junk.key, 6)
end


-- set: invalid target path
function t :test_path_without_handler()
    local err, msg = treemgr.set('whatever.whatever', 123)
    u.assert_nil(err)
    u.assert_string(msg)
end


local function save_into(store)
    store = store or { }
    return function(r)
        for k, v in pairs(r)do store[k] = v end
    end, store
end

-- notify: single value, triggered by a set
function t :test_notify1()
    local hook_z, last_values_z = save_into{ }
    local hook_deep, last_values_deep = save_into{ }
    local r1 = treemgr.register(nil, {'tests.z'}, hook_z)
    u.assert(r1)
    local r2 = treemgr.register(nil, {'tests.z.ramstore.d.e.e.p'}, hook_deep,
        {'tests.x.table1.junk.step'})

    -- Nothing reported yet
    u.assert(not next(last_values_z))
    u.assert(not next(last_values_deep))

    -- setting an associated value doesn't trigger any notification
    u.assert(treemgr.set('tests.x.table1.junk.step', 'a'))
    u.assert_equal(last_values_deep['tests.x.table1.junk.step'], nil)

    -- must trigger both notifications, with associated value
    u.assert(treemgr.set('tests.z.ramstore.d.e.e.p.x.x.x', 123))
    u.assert(treemgr.set('tests.x.table1.junk.step', 'b'))
    u.assert_equal(last_values_deep['tests.x.table1.junk.step'], 'a')

    -- must trigger both notifications, with associated value
    u.assert(treemgr.set({
        ['tests.z.ramstore.d.e.e.p.x.x.y']=234,
        ['tests.z.ramstore.d.e.e.p.x.x.z']=345,
        ['tests.x.table1.junk.unrelated']='shouldnt be reported' }))
    u.assert(treemgr.set('tests.x.table1.junk.step', 'c'))
    u.assert_equal(last_values_deep['tests.x.table1.junk.step'], 'b')

    -- must only trigger one notificatios, without associated value
    u.assert(treemgr.set('tests.z.ramstore.d.e.tour', 456))
    u.assert(treemgr.set('tests.x.table1.junk.step', 'd'))
    u.assert_equal(last_values_deep['tests.x.table1.junk.step'], 'b')

    -- Check consistency
    u.assert_clone_tables(last_values_z, {
        ['tests.z.ramstore.d.e.e.p.x.x.x'] = 123,
        ['tests.z.ramstore.d.e.e.p.x.x.y'] = 234,
        ['tests.z.ramstore.d.e.e.p.x.x.z'] = 345,
        ['tests.z.ramstore.d.e.tour'] = 456 })
    u.assert_clone_tables(last_values_deep, {
        ['tests.z.ramstore.d.e.e.p.x.x.x'] = 123,
        ['tests.z.ramstore.d.e.e.p.x.x.y'] = 234,
        ['tests.z.ramstore.d.e.e.p.x.x.z'] = 345,
        ['tests.x.table1.junk.step'] = 'b' })

    -- unregister
    u.assert(treemgr.unregister(r1))
    u.assert(treemgr.unregister(r2))

    -- There must be no more notification after unregister
    u.assert(treemgr.set('tests.z.ramstore.d.e.e.p.x.x.x', 456))
    u.assert_equal(last_values_deep['tests.z.ramstore.d.e.e.p.x.x.x'], 123)
    u.assert_equal(last_values_z['tests.z.ramstore.d.e.e.p.x.x.x'], 123)

end

function t :test_niltoken()
    u.assert(treemgr.set('tests.x.table1.junk.n1', niltoken))
    u.assert(treemgr.set('tests.x.table1.junk', {n2=niltoken, n3=niltoken}))
    local g = treemgr.get(nil, {
        'tests.x.table1.junk.n1',
        'tests.x.table1.junk.n2',
        'tests.x.table1.junk.n3'})
    u.assert_clone_tables(g, {
        ['tests.x.table1.junk.n1'] = niltoken,
        ['tests.x.table1.junk.n2'] = niltoken,
        ['tests.x.table1.junk.n3'] = niltoken })
end

-- notify: one hpath mapped more than once
function t :test_doublenotif()
    local seen_1, seen_2 = false, false
    local function seen_on_ramstore_w(x)
        u.assert_equal(x["tests.w.ramstore2.some.random.hpath.into.ramstore.foobar"], 1234)
        seen_1=true
    end
    local function seen_on_ramstore_z(x)
        u.assert_equal(x["tests.z.ramstore.foobar"], 1234)
        seen_2=true
    end
    local h1=u.assert(treemgr.register(nil, {'tests.w.ramstore2'}, seen_on_ramstore_w))
    local h2=u.assert(treemgr.register(nil, {'tests.z.ramstore'}, seen_on_ramstore_z))
    u.assert(treemgr.set('tests.z.ramstore.foobar', 1234))
    u.assert(treemgr.unregister(h1))
    u.assert(treemgr.unregister(h2))
    u.assert(treemgr.set('tests.z.ramstore.foobar', nil))
    u.assert(seen_1)
    u.assert(seen_2)
end
