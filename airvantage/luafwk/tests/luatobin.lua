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

local target = require 'luatobin'
local u = require 'unittest'
local _G = _G

local t = u.newtestsuite("luatobin")

function local_exec_simple(totable)
    local function funct() return "coucou voici le loup" end
    local s = target.serialize(nil, totable)
    local _, obj = target.deserialize(s)
    u.assert_nil(obj)

    s = target.serialize(true, totable)
    _, obj = target.deserialize(s)
    u.assert_true(obj)

    s = target.serialize(false, totable)
    _, obj = target.deserialize(s)
    u.assert_false(obj)

    s = target.serialize("string test is valid", totable)
    _, obj = target.deserialize(s)
    u.assert_equal("string test is valid", obj)

    for i=1, 25 do
        s = target.serialize(i, totable)
        _, obj = target.deserialize(s)
        u.assert_equal(i, obj)
    end

    s = target.serialize(funct, totable)
    _, obj = target.deserialize(s)
    u.assert_equal("coucou voici le loup", obj())

    local tab = {nil, true, false, "les chausettes de l'archiduchesse", 65535, funct}
    s = target.serialize(tab, totable)
    local _, obj = target.deserialize(s)
    u.assert_table(obj)
    u.assert_nil(obj[1])
    u.assert_true(obj[2])
    u.assert_false(obj[3])
    u.assert_equal("les chausettes de l'archiduchesse", obj[4])
    u.assert_equal(65535, obj[5])
    u.assert_equal("coucou voici le loup", obj[6]())

    local tab = {reflexion=tab, autre=tab}
    s = target.serialize(tab, totable)
    _, obj = target.deserialize(s)
    u.assert_table(obj)
    u.assert_table(obj.reflexion)
    u.assert_table(obj.autre)
    u.assert_nil(obj.reflexion[1])
    u.assert_true(obj.reflexion[2])
    u.assert_false(obj.reflexion[3])
    u.assert_equal("les chausettes de l'archiduchesse", obj.reflexion[4])
    u.assert_equal(65535, obj.reflexion[5])
    u.assert_equal("coucou voici le loup", obj.reflexion[6]())
    u.assert_nil(obj.autre[1])
    u.assert_true(obj.autre[2])
    u.assert_false(obj.autre[3])
    u.assert_equal("les chausettes de l'archiduchesse", obj.autre[4])
    u.assert_equal(65535, obj.autre[5])
    u.assert_equal("coucou voici le loup", obj.autre[6]())

    tab = {1, 2, 3, 4, 5, 6}
    tab = {'A', 'B', tab, 'C', 'D'}
    tab = {'X', tab, 'Y'}
    s = target.serialize(tab, totable)
    _, obj = target.deserialize(s)
    u.assert_table(obj)
    u.assert_equal('X', obj[1])
    u.assert_table(obj[2])
    u.assert_equal('A', obj[2][1])
    u.assert_equal('B', obj[2][2])
    u.assert_table(obj[2][3])
    u.assert_equal(1, obj[2][3][1])
    u.assert_equal(2, obj[2][3][2])
    u.assert_equal(3, obj[2][3][3])
    u.assert_equal(4, obj[2][3][4])
    u.assert_equal(5, obj[2][3][5])
    u.assert_equal(6, obj[2][3][6])
    u.assert_equal('C', obj[2][4])
    u.assert_equal('D', obj[2][5])
    u.assert_equal('Y', obj[3])

    tab = {"ok", nil, "zut", tab, nil, nil, 15, nil, 1549753, nil}
    s = target.serialize(tab, totable)
    _, obj = target.deserialize(s)
    u.assert_table(obj)
    u.assert_equal("ok", obj[1])
    u.assert_nil(obj[2])
    u.assert_equal("zut", obj[3])
    u.assert_table(obj[4])
    u.assert_equal('X', obj[4][1])
    u.assert_table(obj[4][2])
    u.assert_equal('A', obj[4][2][1])
    u.assert_equal('B', obj[4][2][2])
    u.assert_table(obj[4][2][3])
    u.assert_equal(1, obj[4][2][3][1])
    u.assert_equal(2, obj[4][2][3][2])
    u.assert_equal(3, obj[4][2][3][3])
    u.assert_equal(4, obj[4][2][3][4])
    u.assert_equal(5, obj[4][2][3][5])
    u.assert_equal(6, obj[4][2][3][6])
    u.assert_equal('C', obj[4][2][4])
    u.assert_equal('D', obj[4][2][5])
    u.assert_equal('Y', obj[4][3])
    u.assert_nil(obj[5])
    u.assert_nil(obj[6])
    u.assert_equal(15, obj[7])
    u.assert_nil(obj[8])
    u.assert_equal(1549753, obj[9])
    u.assert_nil(obj[10])
end

function t:test_simple_cases_1()
    --serialize(val, nil)
    local_exec_simple()
end

function t:test_simple_cases_2()
    --serialize(val, false)
    local_exec_simple(false)
    --serialize(val, true)
    local_exec_simple(true)
end

function t:test_simple_cases_3()
    --serialize(val, true)
    local_exec_simple(true)
end

function t:test_partial_deserialize()
    local buff1 = target.serialize("abcdefghijklmnopqrstuvwxyz")

    local tab = {string.sub(buff1,1,1),string.sub(buff1,2,5),string.sub(buff1,6,7),string.sub(buff1,8,#buff1)}
    local _, obj = target.deserialize(tab)
    u.assert_equal("abcdefghijklmnopqrstuvwxyz", obj)

    tab = {string.sub(buff1,1,1),string.sub(buff1,2,2),string.sub(buff1,3,7),string.sub(buff1,8,21),string.sub(buff1,22,#buff1)}
    _, obj = target.deserialize(tab)
    u.assert_equal("abcdefghijklmnopqrstuvwxyz", obj)

    tab = {string.sub(buff1,1,1),string.sub(buff1,2,2),"",string.sub(buff1,3,7),"","","",string.sub(buff1,8,21),string.sub(buff1,22,#buff1)}
    _, obj = target.deserialize(tab)
    u.assert_equal("abcdefghijklmnopqrstuvwxyz", obj)

    local buff2 = target.serialize("123456789")
    local buff3 = target.serialize(1200500300)
    local buff4 = target.serialize(function() return true end)
    local buff5 = target.serialize(nil)
    local buff6 = target.serialize({buff1, buff2, buff3, buff4, buff5})

    local i1, i2 ,i3, i4, i5, i6
    i1, obj = target.deserialize(buff1)
    u.assert_equal("abcdefghijklmnopqrstuvwxyz", obj)
    i2, obj = target.deserialize(buff2)
    u.assert_equal("123456789", obj)
    i3, obj = target.deserialize(buff3)
    u.assert_equal(1200500300, obj)
    i4, obj = target.deserialize(buff4)
    u.assert_equal(true, obj())
    i5, obj = target.deserialize(buff5)
    u.assert_nil(obj)
    i6, obj = target.deserialize(buff6)
    u.assert_table(obj)
    u.assert_equal(buff1, obj[1])
    u.assert_equal(buff2, obj[2])
    u.assert_equal(buff3, obj[3])
    u.assert_equal(buff4, obj[4])
    u.assert_equal(buff5, obj[5])

    local megabuff = buff1..buff2..buff3..buff4..buff5..buff6
    local res = {target.deserialize(megabuff)}
    u.assert_equal(7, #res)
    u.assert_equal("abcdefghijklmnopqrstuvwxyz", res[2])
    u.assert_equal("123456789", res[3])
    u.assert_equal(1200500300, res[4])
    u.assert_equal(true, res[5]())
    u.assert_nil(res[6])
    u.assert_table(res[7])
    u.assert_equal(buff1, res[7][1])
    u.assert_equal(buff2, res[7][2])
    u.assert_equal(buff3, res[7][3])
    u.assert_equal(buff4, res[7][4])
    u.assert_equal(buff5, res[7][5])

    res = {target.deserialize(megabuff,1)}
    u.assert_equal(2, #res)

    res = {target.deserialize(megabuff,2)}
    u.assert_equal(3, #res)

    res = {target.deserialize(megabuff,3)}
    u.assert_equal(4, #res)

    res = {target.deserialize(megabuff,4)}
    u.assert_equal(5, #res)

    res = {target.deserialize(megabuff,5)}
    u.assert_equal(5, #res)

    res = {target.deserialize(megabuff,6)}
    u.assert_equal(7, #res)

    res = {target.deserialize(megabuff,6)}
    u.assert_equal(7, #res)

    res = {target.deserialize(megabuff,20)}
    u.assert_equal(7, #res)

    res = {target.deserialize(megabuff,1,i1)}
    u.assert_equal(2, #res)
    u.assert_equal((i1-1)+(i2-1)+1, res[1])
    u.assert_equal("123456789", res[2])

    res = {target.deserialize(megabuff,1,(i1-1)+(i2-1)+1)}
    u.assert_equal(2, #res)
    u.assert_equal((i1-1)+(i2-1)+(i3-1)+1, res[1])
    u.assert_equal(1200500300, res[2])

    res = {target.deserialize(megabuff,1,(i1-1)+(i2-1)+(i3-1)+(i4-1)+(i5-1)+1)}
    u.assert_equal(2, #res)
    u.assert_table(res[2])
    u.assert_equal(buff1, res[2][1])
    u.assert_equal(buff2, res[2][2])
    u.assert_equal(buff3, res[2][3])
    u.assert_equal(buff4, res[2][4])
    u.assert_equal(buff5, res[2][5])

    res = {target.deserialize(megabuff,2,(i1-1)+(i2-1)+(i3-1)+1)}
    u.assert_equal(2, #res)
    u.assert_equal((i1-1)+(i2-1)+(i3-1)+(i4-1)+(i5-1)+1, res[1])
    u.assert_equal(true, res[2]())
end

function t:test_number()
    local s, _, obj

    for i=1, 10 do
        s = target.serialize(i/65536, true)
        _, obj = target.deserialize(s)
        u.assert_equal(i/65536, obj)
    end

    for i=11, 20 do
        s = target.serialize(i*3.14, true)
        _, obj = target.deserialize(s)
        u.assert_equal(i*3.14, obj)
    end

    for i=21, 30 do
        s = target.serialize(i/65536)
        _, obj = target.deserialize(s)
        u.assert_equal(i/65536, obj)
    end

    for i=31, 40 do
        s = target.serialize(i*3.14)
        _, obj = target.deserialize(s)
        u.assert_equal(i*3.14, obj)
    end
end

--[[ micro-bench (~4.2 seconds on my laptop)
local tab = { a='a', b='b', c='c', d='d', hop='jump', skip='foo', answer=42 }
local s = target.serialize(tab)
for i=1, 1000000 do
s = target.serialize(tab)
tab = target.deserialize(s)
end
--]]
