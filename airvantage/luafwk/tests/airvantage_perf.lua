-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Gilles Cannenterre for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

function visitor(item)
    print(string.format("evaluating '%s'...", tostring(item)))
    local t = {store = tostring(item), max = {val = 0}, item = {}}
    t.item[t] = true
    local function evaluate(t, item, parent)
        t[t.store] = t[t.store] or 0
        --print(string.format("[%d] -> '%s' found.", t.store, type(item)))
        if not t.item[item] and type(item) == "nil" then
            t[t.store] = t[t.store] + 1
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "boolean" then
            t[t.store] = t[t.store] + 1 + 1
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "number" then
            t[t.store] = t[t.store] + 1 + 2
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "string" then
            t[t.store] = t[t.store] + 1 + #item
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "function" then
            t[t.store] = t[t.store] + 1 + 4
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "userdata" then
            t[t.store] = t[t.store] + 1 + 4
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "thread" then
            t[t.store] = t[t.store] + 1 + 4
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
        elseif not t.item[item] and type(item) == "table" then
            t[t.store] = t[t.store] + 1 + 4
            t.item[item] = true
            if t[t.store] > t.max["val"] then t.max["val"] = t[t.store] t.max["obj"] = {item = item, parent = parent} t.max["store"] = t.store end
            for k, v in pairs(item) do
                evaluate(t, k, item)
                evaluate(t, v, item)
            end
            t.store = tostring(item)
        end
    end
    evaluate(t, item)
    t.item = nil
    return t
end

local sched = require "sched"
local u = require "unittest"
local racon = require "racon"

local t = u.newtestsuite("racon_perf")

function t :setup()
    u.assert(racon.init())
end

local function senddata(asset)
    print(string.format("sendata for '%s'",asset.id))
    local m1 = collectgarbage("count")
    local status
    for i=1, 1000 do
        local path = {}
        local data = {}
        for j=1,math.random(50) do
            table.insert(path, string.format("segment-%d", j))
            table.insert(data, string.format("%dbrfdla%dgdrzdfzf", j, i))
        end
        status = asset:pushdata(table.concat(path, "."), {asset = asset.id, data = table.concat(data)})
        u.assert_equal("ok", status)
    end

    local m2 = collectgarbage("count")
    status = asset:triggerpolicy()
    sched.wait(1)
    u.assert_equal("ok", status)
    local m3 = collectgarbage("count")
    return m1, m2, m3
end

function t:test_multipleassets()
    local f = io.open("persist/racon_perf.txt", "w+")
    u.assert_not_nil(f)
    collectgarbage("collect")
    local mi = collectgarbage("count")
    f:write(string.format("before:\t'%f'\r\n", mi))

    --assets list
    local assets = {}
    --create and register
    for i=1,100 do
        local newasset = racon.newasset(string.format("asset-test-perf-%d", i))
        u.assert_not_nil(newasset)
        local status = newasset:start()
        u.assert_equal("ok", status)
        table.insert(assets, newasset)
    end

    f:write("<asset id>\t<before push>\t<after push>\t<after send>\r\n")
    for i, v in ipairs(assets) do
        local m1, m2, m3 = senddata(v)
        f:write(string.format("'%s'\t'%f'\t'%f'\t'%f'\r\n", v.id, m1-mi, m2-mi, m3-mi))
    end

    for i, v in ipairs(assets) do
        u.assert_equal("ok", v:close())
    end

    local mt = collectgarbage("count")
    f:write(string.format("after:\t'%f'\r\n", mt-mi))
    f:close()
end
