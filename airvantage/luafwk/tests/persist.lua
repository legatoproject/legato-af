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
local u = require 'unittest'
local _G = _G

--it used to exist 2 impl of persist, persist.qdbm and persist.file.
-- dynamic detection of available implementation is made at the end of this file.

--the point of this function is to generate 1 test suite per available persist implementation
local function  persist_testsuite(persist_impl, name)

    local target = persist_impl
    local t = u.newtestsuite("persist_"..name)

    local cb, val, nt, last

    function t:setup()
        nt = target.table.new("testNewtable")
    end

    function t:test_newtable()
        target.table.empty(nt)
        for i=1,256 do
            nt["key"..i] = "value"..i
        end

        for i=1,256 do
            local val = nt["key"..i]
            u.assert_equal("value"..i, val)
        end

        for i=1,256 do
            nt["key"..i] = "value"..(2*i)
        end

        for i=1,256 do
            local val = nt["key"..i]
            u.assert_equal("value"..(2*i), val)
        end

        for i=1,256 do
            nt["key"..i] = nil
        end

        for i=1,256 do
            local val = nt["key"..i]
            u.assert_nil(val)
        end
    end

    function t:test_persit_store()
        local obj = {function () return "titi" end, 1 , "string test", nil, true}
        target.save("test_persit_store", obj)

        local val = target.load("test_persit_store")
        u.assert_table(val)
        u.assert_function(val[1])
        u.assert_equal("titi", val[1]())
        u.assert_equal(1, val[2])
        u.assert_equal("string test", val[3])
        u.assert_nil(val[4])
        u.assert_equal(true, val[5])

        target.save("test_persit_store", nil)
        val = target.load("test_persit_store")
        u.assert_nil(val)
    end

    function t:teardown()
        target.table.empty(nt)
        nt = nil
    end


end
local status,persist_impl
local impls= {"qdbm", "file"}

for _,v in pairs(impls) do
    local impl = 'persist.'..v
    status, persist_impl = pcall(require, impl)
    if status then persist_testsuite(persist_impl, v)
    --if the error is "module not found" returned by require, then silently ignore it
    --to handle gracefully case with qdbm not available
    --other persist impl loading issues generate a "failure only" test suite
    elseif type(persist_impl)~="string" or not persist_impl:match("module '"..impl.."' not found") then
            local t = u.newtestsuite(impl)
            --when the "failure only" test suite will run, those up values may have change
            local mod = impl
            local error = persist_impl

            function t:setup()
                u.abort(string.format("can't load %s, error = %s", tostring(mod), tostring(error)))
            end
            --fake test to keep unittest "happy": needed to activate "failure only" testsuite so that the error shows up in test log
            function t:test_void()
            end
    end
end

