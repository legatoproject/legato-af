------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Julien Desgats     for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------


--TODO: we currently use low level functions, modify it when the higher level API will be
--implemented (accumulator)
require 'm3da.bysant.core'
local niltoken = require 'niltoken'

require 'pack' -- used only to get a specific value of NaN for floating point values

local core = m3da.bysant.core

-- Low level API helpers
-- create a wrapper around bysant object and buffer to add some methods
local bysant_wrapper = { }
function bysant_wrapper:serialize()  return table.concat(self.buffer) end
function bysant_wrapper:value(value)
    local tvalue = type(value)
    if self.ctx[tvalue] then return self[tvalue](self, value)
    elseif value == nil or value == niltoken then return self:null()
    end
end

function bysant_wrapper:__index(idx)
    if self.ctx[idx] then
        return function(self, ...)
            local ok, err = self.ctx[idx](self.ctx, ...)
            if ok then return self
            elseif not self.allow_errors then error(err)
            else return nil, err end
        end
    else return getmetatable(self)[idx] end
end

local function bysants(allow_errors)
    local ctx, buffer = core.init({})
    return setmetatable({ctx=ctx, buffer=buffer, allow_errors=allow_errors}, bysant_wrapper)
end


-- Test helpers
local u = require 'unittest'
-- Encode a string representation of a binary chunk into a real binary chunk
-- Bytes are given in hex with optional ascii sequences in double quotes.
-- Example '7f"hello"00'
local function encode(str)
    local bin = { }
    while #str > 0 do
        local chunk
        if str:sub(1,1) == '"' then
            chunk, str = str:match('^"(.-)"(.*)$')
            bin[#bin+1] = chunk
        else
            chunk, str = str:match('^(..)(.*)$')
            bin[#bin+1] = string.char(tonumber(chunk, 16))
        end
    end
    return table.concat(bin)
end

local function hexdump(hex)
    return (hex:gsub(".", function(byte)
        return string.format("%02x", byte:byte())
    end))
end

local function bysants_assert(value, expected, msg)
    if type(value) ~= "table" then value = bysants():value(value) end
    value = value:serialize()
    expected = encode(expected)
    u.assert_equal (expected, value,
                    string.format("Expected [%s], got [%s]", hexdump(expected), hexdump(value)))
end

local function bysantd(str, o) return core.deserializer():deserialize(encode(str), o) end
local function bysantd_assert(value, expected, msg)
    local f = type(expected) == "table" and u.assert_clone_tables or u.assert_equal
    local payload, length = bysantd(value)
    f(expected, payload, msg or  string.format("input value: [%s], expected output: [%s], failed on equal test", sprint(value), sprint(expected)))
    -- check that everything has been deserialized:
    -- using #encode(value) +1: +1 because deserializer has been converted to return 1-based string length
    u.assert_equal(#encode(value) +1, length,  string.format("input value: [%s], expected output: [%s], expected deserialized size %d, got %d", sprint(value), sprint(expected), #encode(value), length))
end

local function make_list_body(size, baseval, prefix, ctxid, ctx)
    local ctx = (ctx or bysants()):list(size, ctxid)
    local bysant, result = { prefix }, { }
    for i=1,size do
        bysant[i+1] = hexdump(string.char(baseval + i))
        result[i] = i
        ctx:number(i)
    end
    return ctx:close(), table.concat(bysant), result
end

local function make_map_body(size, baseval, prefix, ctxid, ctx)
    local ctx = (ctx or bysants()):map(size, ctxid)
    local bysant, result = { prefix }, { }
    for i=1,size do
        bysant[i+1] = hexdump(string.char(0x3b+i, baseval + i))
        result[i] = i
        ctx:number(i):number(i)
    end
    return ctx:close(), table.concat(bysant), result
end

local function maker_wrap(towrap, wctx)
    return function(size, baseval, prefix, ctxid)
        local ctx, bysant, result = towrap(size, baseval, prefix, ctxid, bysants():list(1, wctx))
        return ctx:close(), bysant, result
    end
end

--------------------------------------------------------------------------------
--- Global Context
--------------------------------------------------------------------------------

local globals = u.newtestsuite 'Bysant Serializer - Global context'

function globals :test_simpletypes()
    bysants_assert(nil,             '00')
    bysants_assert(true,            '01')
    bysants_assert(false,           '02')
    --bysants_assert({ 1, 2, 3 },     '2da0a1a2')
    --bysants_assert({ x = 1 },       '4202"x"81')
    bysants_assert(niltoken, '00')

    --u.assert_equal (bysants(), encode'00')
    --u.assert_nil   (bysants(function() end))

    -- try to serialize multiple values
    bysants_assert(bysants():number(0):number(0), '9f9f')
end

local GLOBAL_INTEGERS = {
    { -2147483649, 'fdffffffff7fffffff' },
    { -2147483648, 'fc80000000' }, -- global ctx: int32 min value
    { -  33818656, 'fcfdfbf7e0' },
    { -  33818655, 'fbffffff' }, --global ctx: large integer min value
    { -    264224, 'fa000000' },
    { -    264223, 'f7ffff' }, --global ctx: medium integer min value
    { -      2080, 'f40000' },
    { -      2079, 'efff' }, --global ctx: small integer min value
    { -        32, 'e800' },
    { -        31, '80' }, --global ctx: tiny integer min value
    {           0, '9f' },
    {          64, 'df' }, --global ctx: tiny integer max value
    {          65, 'e000' },
    {        2112, 'e7ff' }, --global ctx: small integer max value
    {        2113, 'f00000' },
    {      264256, 'f3ffff' }, --global ctx: medium integer max value
    {      264257, 'f8000000' },
    {    33818688, 'f9ffffff' }, --global ctx: large integer max value
    {    33818689, 'fc02040841' },
    {  2147483647, 'fc7fffffff' }, -- global ctx: int32 max value
    {  2147483648, 'fd0000000080000000' },
}

local GLOBAL_FLOATS = {
    { 0.25,    'fe3e800000' },
    { 1e30/13, 'ff45ef11a8e476d4d1' },
}

local GLOBAL_STRINGS = {
    { '', '03' },
    { string.rep('a',    32), '23"'    ..string.rep('a',    32)..'"' },
    { string.rep('a',    33), '2400"'  ..string.rep('a',    33)..'"' },
    { string.rep('a',  1056), '27ff"'  ..string.rep('a',  1056)..'"' },
    { string.rep('a',  1057), '280000"'..string.rep('a',  1057)..'"' },
    { string.rep('a', 66592), '28ffff"'..string.rep('a', 66592)..'"' },
    -- longer strings make chunked strings
    { string.rep('a', 66593), '29ffff"'..string.rep('a', 65535)..'"0422"'..string.rep('a', 1058)..'"0000' },
}

function globals :test_int()
    for _, t in ipairs(GLOBAL_INTEGERS) do bysants_assert(t[1], t[2]) end
end

function globals :test_float()
    for _, t in ipairs(GLOBAL_FLOATS) do bysants_assert(t[1], t[2]) end
end

function globals :test_string()
    for _, t in ipairs(GLOBAL_STRINGS) do bysants_assert(t[1], t[2]) end
    bysants_assert(bysants():chunked():chunk("abc"):chunk("def"):close(), '290003"abc"0003"def"0000')

    -- test error
    u.assert_nil(bysants(true):chunked():number(1), "Should not be allowed to define data inside chunked string")
end

local GLOBAL_LIST = {
    -- fixed, untyped
    { make_list_body( 0, 0x9f, '2a',   nil) },
    { make_list_body( 1, 0x9f, '2b',   nil) },
    { make_list_body( 9, 0x9f, '33',   nil) },
    { make_list_body(10, 0x9f, '343b', nil) },
--    -- variable, untyped
    { bysants():list():close(),           '3500',   { } },
    { bysants():list():number(1):close(), '35a000', { 1 } },
--    -- fixed, typed
    { make_list_body( 0, 0x62, '2a',   'number') },
    { make_list_body( 1, 0x62, '3602',   'number') },
    { make_list_body( 9, 0x62, '3e02',   'number') },
    { make_list_body(10, 0x62, '3f3b02', 'number') },
--    -- variable, typed
    { bysants():list(nil, "number"):close(),           '400200',   { } },
    { bysants():list(nil, "number"):number(1):close(), '40026300', { 1 } },
}

local GLOBAL_MAP = {
    -- fixed, untyped
    { make_map_body( 0, 0x9f, '41',   nil) },
    { make_map_body( 1, 0x9f, '42',   nil) },
    { make_map_body( 9, 0x9f, '4a',   nil) },
    { make_map_body(10, 0x9f, '4b3b', nil) },
    -- variable, untyped
    { bysants():map():close(), '4c00',  { } },
    { bysants():map():string("foo"):string("bar"):close(), '4c04"foo"06"bar"00', { foo="bar" }},
    -- fixed, typed
    { make_map_body( 0, 0x62, '41',     'number') },
    { make_map_body( 1, 0x62, '4d02',   'number') },
    { make_map_body( 9, 0x62, '5502',   'number') },
    { make_map_body(10, 0x62, '563b02', 'number') },
    -- variable, typed
    { bysants():map(nil, "number"):close(), '570200',  { } },
    { bysants():map(nil, "number"):string("foo"):number(0):close(), '570204"foo"6200', { foo=0 }},
}

function globals :test_list()
    for _, case in ipairs(GLOBAL_LIST) do
        local ctx, bysant, _ = unpack(case)
        bysants_assert(ctx, bysant)
    end

    -- test errors
    u.assert_nil(bysants(true):list(1):close(), "Not enough elements at close.")
    u.assert_nil(bysants(true):list(1):number(0):number(0), "Too many elements inserted.")

    -- null values
    bysants_assert(bysants():list(1):null():close(), '2b00')
    u.assert_nil(bysants(true):list():null())
end

function globals :test_map()
    for _, case in ipairs(GLOBAL_MAP) do
        local ctx, bysant, _ = unpack(case)
        bysants_assert(ctx, bysant)
    end

    -- errors on fixed length
    u.assert_nil(bysants(true):map(1):close(), "Not enough elements at close.")
    u.assert_nil(bysants(true):map(0):number(0), "Too many elements inserted.")
    u.assert_nil(bysants(true):map(1):number(0):close(), "Closed with a odd number of elements")
    u.assert_nil(bysants(true):map():number(0):close(), "Closed with a odd number of elements")

    -- null values
    bysants_assert(bysants():map(1):number(0):null():close(), '423b00')
    bysants_assert(bysants():map() :number(0):null():close(), '4c3b0000')
    u.assert_nil(bysants(true):map(1):null(), "Set null as map key")
end

function globals :test_class()
    -- class definition only
    bysants_assert(bysants():class({id=0}), '723b3b')
    bysants_assert(bysants():class({id=0, {context="global"}, {context="unsignedstring"}, {context="number"} }),
                  '723b3e000102')
    -- use short form
    bysants_assert(bysants():class({id=0, "global", "unsignedstring", "number"}), '723b3e000102')
    bysants_assert(bysants():class({
                    id=1, name="myclass",
                    {name="time", context="unsignedstring"},
                    {name="value", context="number"} }),
                  '713c08"myclass"3d05"time"0106"value"02')

    -- define a class in the middle of a list: this should not be counted as object
    bysants_assert(bysants():list(2):number(0):class({id=0}):number(1):close(),
                  '2c9f723b3ba0')
    -- in a map, it should not mess with key/value order
    bysants_assert(bysants():map():string("k1"):class({id=0}):number(1):close(),
                  '4c03"k1"723b3ba000')

    -- internal class
    bysants_assert(bysants():class({id=0}, true), '')
    -- internal classes can be declared anywhere
    bysants_assert(bysants():map():class({id=0}, true):string("k1"):number(1):close(),
                  '4c03"k1"a000')
end

function globals :test_instance()
    bysants_assert(bysants()
        :class({ id=20, name="myclass",
            { name="time", context="unsignedstring"},
            {name="value", context="number"}
        })
        :object(20):number(2000):number(-100):close()
        :number(0),
        '714f08"myclass"3d05"time"0106"value"02703fce44d4029f')
        --71: Full class definition, 4F: class id: 20 (4F-3B), 08: class name: string length 08-01 = 7 -> "myclass"
        --3D: length of field list (unsigned)  2 (3D - 3B),
        --   - 05:  class field name: string length 05-01 = 4  -> "time"
        --     01: contextid: Context 1: Unsigned Integers and Strings (UIS)
        --   - 06:  class field name: string length 06-01 = 5  -> "value"
        --     02: contextid: Context 2: Numbers
        --70: object instance, 3f: class id, UIS value: 4 (3f-3b) -> class id = 4 +16 = 20
        --ce: in context="unsignedstring" UIS:  Small unsigned integer from 140 to 8331.
        --44: BYTE0 for Small unsigned integer
        -- ->Value is 140 + (OPCODE - 0xC7) * 256 + BYTE0 : 140+(0xCE-0xC7)*256 + 0x44 : 2000
        --d4: in context="number" small integer (13 bits) with MSB as sign bit.
        --02  BYTE0 for:
        -- ->value For opcodes from 0xD8 to 0xE7: -1*(((OPCODE - 0xD4) << 8) + BYTE0) - 98 : -1*( ( (d4-d4)<<8) + 0x02 ) - 98 : -100
        --9F:  ctx global : 0
   bysants_assert(bysants()
        :class({id=0})
        :object(0):close()
        :number(0),
        '723b3b609f')
   -- test with an internal class
   bysants_assert(bysants()
        :class({ id=20, name="myclass",
            { name="time", context="unsignedstring"},
            {name="value", context="number"}
        }, true)
        :object(20):number(2000):number(-100):close()
        :number(0),
        '703fce44d4029f')
end

local globald = u.newtestsuite 'Bysant Deserializer - Global context'

---[[
function globald :test_simpletypes()
    bysantd_assert('00', niltoken)
    bysantd_assert('01', true)
    bysantd_assert('02', false)
end

function globald :test_int()
    for _, t in ipairs(GLOBAL_INTEGERS) do bysantd_assert(t[2], t[1]) end
end

function globald :test_float()
    for _, t in ipairs(GLOBAL_FLOATS) do bysantd_assert(t[2], t[1]) end
end

function globald :test_string()
    for _, t in ipairs(GLOBAL_STRINGS) do bysantd_assert(t[2], t[1]) end
end

function globald :test_list()
    for _, case in ipairs(GLOBAL_LIST) do
        local _, bysant, result = unpack(case)
        bysantd_assert(bysant, result)
    end
end

function globald :test_map()
    for _, case in ipairs(GLOBAL_MAP) do
        local _, bysant, result = unpack(case)
        bysantd_assert(bysant, result)
    end

    -- null values
    bysantd_assert('423b00', {[0]=niltoken})   -- fixed
    bysantd_assert('4c3b0000', {[0]=niltoken}) -- variable

    -- value overwrite
    bysantd_assert('4304"foo"06"bar"04"foo"06"baz"', {foo="baz"})
    bysantd_assert('4304"foo"06"bar"04"foo"00', {foo=niltoken})
end
--]]

function globald :test_instance()
    -- just a class declaration
    bysantd_assert('714f08"myclass"3d05"time"0106"value"0201', true) -- class declaration followed by a boolean because class declaration returns nothing
    bysantd_assert('723b3e000102609fce44d402', { 0, 2000, -100, __class = 0 })
    bysantd_assert('714f08"myclass"3d05"time"0106"value"02703fce44d402', { time = 2000, value = -100, __class = "myclass" })
    -- class declaration in middle of other objects
    bysantd_assert('2c9f723b3ba0', { 0, 1 })
    bysantd_assert('4c03"k1"723b3ba000', { k1 = 1 })
    -- with internal class
    local d = core.deserializer()
    d:addClass{
        name = "MyInternalClass", id = 2,
        { name = "time",  context = "unsignedstring" },
        { name = "value", context = "number"         },
    }
    u.assert_clone_tables({ time = 2000, value = -100, __class = "MyInternalClass" },
                          d:deserialize(encode("62ce44d402")))
end

function globald :test_nested_containers()
    -- test various combinaisons of nested lists/maps
    bysantd_assert( hexdump(bysants():list():number(1):map(1):string("foo"):number(0):close():number(2):close():serialize()),
                    { 1, { foo=0 }, 2} )
    bysantd_assert( hexdump(bysants():list(3):number(1):map(1):string("foo"):list():close():close():number(2):close():serialize()),
                    { 1, { foo={} }, 2} )
    bysantd_assert( hexdump(bysants():list(3):number(1):map(1):string("foo"):number(0):close():number(2):close():serialize()),
                    { 1, { foo=0 }, 2} )
    bysantd_assert( hexdump(bysants():map(3)
                        :string("k"):map():string("foo"):number(0):close()
                        :number(2):string("v")
                        :string("k2"):list(1):number(0):close()
                    :close():serialize()),
                    { k = { foo=0 }, [2]="v", k2={ 0 } } )

    -- container inside instance
    bysantd_assert('723b3d00006035a0a1009f', { {1, 2}, 0, __class=0})
    bysantd_assert('723b3d0000602ca0a19f', { {1, 2}, 0, __class=0})
end

--------------------------------------------------------------------------------
--- Number Context
--------------------------------------------------------------------------------
local numbers = u.newtestsuite 'Bysant Serializer - Number context'

local NUMBER_INTEGER = {
    { -2147483649, 'fdffffffff7fffffff' },
    { -2147483648, 'fc80000000' }, --number ctx: int32 min value
    { -  67637346, 'fcfbf7ef9e' },
    { -  67637345, 'fbffffff' }, --number ctx: large integer min value
    { -    528482, 'f8000000' },
    { -    528481, 'f3ffff' }, --number ctx: medium integer min value
    { -      4194, 'ec0000' },
    { -      4193, 'e3ff' }, --number ctx: small integer min value
    { -        98, 'd400' },
    { -        97, '01' }, --number ctx: tiny integer min value
    {           0, '62' },
    {          97, 'c3' }, --number ctx: tiny integer max value
    {          98, 'c400' },
    {        4193, 'd3ff' }, --number ctx: small integer max value
    {        4194, 'e40000' },
    {      528481, 'ebffff' }, --number ctx: medium integer max value
    {      528482, 'f4000000' },
    {    67637345, 'f7ffffff' }, --number ctx: large integer max value
    {    67637346, 'fc04081062' },
    {  2147483647, 'fc7fffffff' },   --number ctx: int32 max value
    {  2147483648, 'fd0000000080000000' },
}

local NUMBER_FLOAT = {
    { 0.25,    'fe3e800000' },
    { 1e30/13, 'ff45ef11a8e476d4d1' },
}

-- test a value in number context
--3602: 36 declare a list with specific context, list of size = 36-36+1 = 1
--02 context number
local function bysants_assert_number(v, exp) bysants_assert(bysants():list(1, "number"):value(v):close(), '3602'..exp) end
local function bysantd_assert_number(v, exp) bysantd_assert('3602'..v, {exp}) end

function numbers :test_int()
    for _, t in ipairs(NUMBER_INTEGER) do bysants_assert_number(t[1], t[2]) end
end

function numbers :test_float()
    for _, t in ipairs(NUMBER_FLOAT) do bysants_assert_number(t[1], t[2]) end
end

function numbers :test_errors()
    u.assert_nil(bysants(true):list(1, "number"):boolean(true))
    u.assert_nil(bysants(true):list(1, "number"):string(""))
    u.assert_nil(bysants(true):list(1, "number"):list())
    u.assert_nil(bysants(true):list(1, "number"):map())
    u.assert_nil(bysants(true):list(1, "number"):object(0))
    u.assert_nil(bysants(true):list(1, "number"):class({id=0}))
end

---[[
local numberd = u.newtestsuite 'Bysant Deserializer - Number context'

function numberd :test_int()
    for _, t in ipairs(NUMBER_INTEGER) do bysantd_assert_number(t[2], t[1]) end
end

function numberd :test_float()
    for _, t in ipairs(NUMBER_FLOAT) do bysantd_assert_number(t[2], t[1]) end
end
--]]

--------------------------------------------------------------------------------
--- Unsigned or String Context
--------------------------------------------------------------------------------
local uiss = u.newtestsuite 'Bysant Serializer - Unsigned and Strings context'

local UIS_INTEGER = {
    {          0, '3b' },
    {        139, 'c6' },
    {        140, 'c700' },
    {       8331, 'e6ff' },
    {       8332, 'e70000' },
    {    1056907, 'f6ffff' },
    {    1056908, 'f7000000' },
    {  135274635, 'feffffff' },
    {  135274636, 'ff0810208c' },
    { 4294967295, 'ffffffffff' },
}

local UIS_STRING = {
    { '', '01' },
    { string.rep('a',    47), '30"'    ..string.rep('a',    47)..'"' },
    { string.rep('a',    48), '3100"'  ..string.rep('a',    48)..'"' },
    { string.rep('a',  2095), '38ff"'  ..string.rep('a',  2095)..'"' },
    { string.rep('a',  2096), '390000"'..string.rep('a',  2096)..'"' },
    { string.rep('a', 67631), '39ffff"'..string.rep('a', 67631)..'"' },
    -- longer strings make chunked strings
    { string.rep('a', 67632), '3affff"'..string.rep('a', 65535)..'"0831"'..string.rep('a', 2097)..'"0000' },
}

local function bysants_assert_uis(v, exp) bysants_assert(bysants():list(1, "unsignedstring"):value(v):close(), '3601'..exp) end
local function bysantd_assert_uis(v, exp) bysantd_assert('3601'..v, {exp}) end

function uiss :test_int()
    for _, t in ipairs(UIS_INTEGER) do bysants_assert_uis(t[1], t[2]) end
end

function uiss :test_string()
    for _, t in ipairs(UIS_STRING) do bysants_assert_uis(t[1], t[2]) end
    bysants_assert(bysants():list(1, "unsignedstring"):chunked():chunk("abc"):chunk("def"):close():close(),
                  '36013a0003"abc"0003"def"0000')
end

function uiss :test_errors()
    u.assert_nil(bysants(true):list(1, "unsignedstring"):boolean(true))
    u.assert_nil(bysants(true):list(1, "unsignedstring"):number(0.25))
    u.assert_nil(bysants(true):list(1, "unsignedstring"):number(-1))
    u.assert_nil(bysants(true):list(1, "unsignedstring"):list())
    u.assert_nil(bysants(true):list(1, "unsignedstring"):map())
    u.assert_nil(bysants(true):list(1, "unsignedstring"):object(0))
    u.assert_nil(bysants(true):list(1, "unsignedstring"):class({id=0}))
end

local uisd = u.newtestsuite 'Bysant Deserializer - Unsigned and Strings context'

---[[
function uisd :test_int()
    for _, t in ipairs(UIS_INTEGER) do bysantd_assert_uis(t[2], t[1]) end
end

function uisd :test_string()
    for _, t in ipairs(UIS_STRING) do bysantd_assert_uis(t[2], t[1]) end
end
--]]

--------------------------------------------------------------------------------
--- List & map Context
--------------------------------------------------------------------------------

local make_list_body_lm = maker_wrap(make_list_body, 'listmap')
local make_map_body_lm = maker_wrap(make_map_body, 'listmap')
local LISTMAP_LIST = {
    -- fixed, untyped
    { make_list_body_lm( 0, 0x9f, '01',   nil) },
    { make_list_body_lm( 1, 0x9f, '02',   nil) },
    { make_list_body_lm(60, 0x9f, '3d',   nil) },
    { make_list_body_lm(61, 0x9f, '3e3b', nil) },
    -- variable, untyped
    { bysants():list(1, "listmap"):list():close():close(),           '3f00',   { } },
    { bysants():list(1, "listmap"):list():number(1):close():close(), '3fa000', { 1 } },
    -- fixed, typed
    { make_list_body_lm( 0, 0x62, '01',     "number") },
    { make_list_body_lm( 1, 0x62, '4002',   "number") },
    { make_list_body_lm(60, 0x62, '7b02',   "number") },
    { make_list_body_lm(61, 0x62, '7c3b02', "number") },
    -- variable, typed
    { bysants():list(1, "listmap"):list(nil, "number"):close():close(),           '7d0200',   { } },
    { bysants():list(1, "listmap"):list(nil, "number"):number(1):close():close(), '7d026300', { 1 } },
}

local LISTMAP_MAP = {
    -- fixed, untyped
    { make_map_body_lm( 0, 0x9f, '83') },
    { make_map_body_lm( 1, 0x9f, '84') },
    { make_map_body_lm(60, 0x9f, 'bf') },
    { make_map_body_lm(61, 0x9f, 'c03b') },
    -- variable, untyped
    { bysants():list(1, "listmap"):map():close():close(), 'c100', { } },
    { bysants():list(1, "listmap"):map():number(1):number(1):close():close(), 'c13ca000', { 1 } },
    -- fixed, typed
    { make_map_body_lm( 0, 0x62, '83', 'number') },
    { make_map_body_lm( 1, 0x62, 'c202', 'number') },
    { make_map_body_lm(60, 0x62, 'fd02', 'number') },
    { make_map_body_lm(61, 0x62, 'fe3b02', 'number') },
    -- variable, typed
    { bysants():list(1, "listmap"):map(nil, 'number'):close():close(), 'ff0200', { } },
    { bysants():list(1, "listmap"):map(nil, 'number'):number(1):number(1):close():close(), 'ff023c6300', { 1 } },
}

-- Lists and maps
local lms = u.newtestsuite 'Bysant Serializer - List & Map context'
function lms :test_list()
    for _, case in ipairs(LISTMAP_LIST) do
        local ctx, bysant, _ = unpack(case)
        bysants_assert(ctx, '3606'..bysant)
    end
end

function lms :test_map()
    for _, case in ipairs(LISTMAP_MAP) do
        local ctx, bysant, _ = unpack(case)
        bysants_assert(ctx, '3606'..bysant)
        --3606: 36 declare a list with specific context, list of size = 36-36+1 = 1
        --06 context number
    end
end

function lms :test_errors()
    u.assert_nil(bysants(true):list(1, "listmap"):boolean(true))
    u.assert_nil(bysants(true):list(1, "listmap"):number(0))
    u.assert_nil(bysants(true):list(1, "listmap"):string(""))
    u.assert_nil(bysants(true):list(1, "listmap"):object(0))
    u.assert_nil(bysants(true):list(1, "listmap"):class({id=0}))
end

local lmd = u.newtestsuite 'Bysant Deserializer - List & Map context'
local function bysantd_assert_lm(v, exp) bysantd_assert('3606'..v, {exp}) end

---[[
function lmd :test_list()
    for _, case in ipairs(LISTMAP_LIST) do
        local _, bysant, result = unpack(case)
        bysantd_assert_lm(bysant, result)
    end
end

function lmd :test_map()
    for _, case in ipairs(LISTMAP_MAP) do
        local _, bysant, result = unpack(case)
        bysantd_assert_lm(bysant, result)
    end
end
--]]

--------------------------------------------------------------------------------
--- Int32 Context
--------------------------------------------------------------------------------
local int32s = u.newtestsuite 'Bysant Serializer - int32 context'

function int32s :test_int()
    bysants_assert(bysants():list(1, "int32"):number(0):close(), '360300000000')
    bysants_assert(bysants():list(1, "int32"):number(-2147483648):close(), '36038000000001')
end

function int32s :test_null()
    bysants_assert(bysants():list(1, "int32"):null():close(), '36038000000000')
    bysants_assert(bysants():list(nil, "int32"):close(), '40038000000000')
end

function int32s :test_errors()
    u.assert_nil(bysants(true):list(1, "int32"):boolean(true))
    u.assert_nil(bysants(true):list(1, "int32"):number(0.25))
    u.assert_nil(bysants(true):list(1, "int32"):list())
    u.assert_nil(bysants(true):list(1, "int32"):map())
    u.assert_nil(bysants(true):list(1, "int32"):string(""))
    u.assert_nil(bysants(true):list(1, "int32"):object(0))
    u.assert_nil(bysants(true):list(1, "int32"):class({id=0}))
end

local int32d = u.newtestsuite 'Bysant Deserializer - int32 context'
local function bysantd_assert_int32(v, exp) bysantd_assert('3603'..v, {exp}) end

function int32d :test()
    bysantd_assert_int32('00000000', 0)
    bysantd_assert_int32('8000000001', -2147483648)
    bysantd_assert_int32('8000000000', niltoken)
end

--------------------------------------------------------------------------------
--- Float/Double Contexts
--------------------------------------------------------------------------------
local floats = u.newtestsuite 'Bysant Serializer - float context'

function floats :test_float()
    -- we want a specific value of NaN, there is no simple way to get it
    local function getNaN(kind) return select(2, string.unpack(string.rep('\255', 8), kind)) end
    bysants_assert(bysants():list(1, "float"):number(1):close(),  '36043f800000')
    bysants_assert(bysants():list(1, "float"):number(0.25):close(),  '36043e800000')
    bysants_assert(bysants():list(1, "float"):number(1e30/13):close(),  '36046f788d47')
    bysants_assert(bysants():list(1, "float"):number(getNaN('f')):close(), '3604ffffffff01')
    bysants_assert(bysants():list(1, "double"):number(0.25):close(), '36053fd0000000000000')
    bysants_assert(bysants():list(1, "double"):number(1e30/13):close(),  '360545ef11a8e476d4d1')
    bysants_assert(bysants():list(1, "double"):number(getNaN('d')):close(), '3605ffffffffffffffff01')
end

function floats :test_null()
    bysants_assert(bysants():list(1, "float"):null():close(), '3604ffffffff00')
    bysants_assert(bysants():list(nil, "float"):close(), '4004ffffffff00')
    bysants_assert(bysants():list(1, "double"):null():close(), '3605ffffffffffffffff00')
    bysants_assert(bysants():list(nil, "double"):close(), '4005ffffffffffffffff00')
end

local floatd = u.newtestsuite 'Bysant Deserializer - float context'
local function bysantd_assert_float(v, exp) bysantd_assert('3604'..v, {exp}) end
local function bysantd_assert_double(v, exp) bysantd_assert('3605'..v, {exp}) end

function floatd :test()
    local function isnan(x) return x ~= x end
    bysantd_assert_float('3f800000', 1)
    bysantd_assert_float('3e800000', 0.25)
    bysantd_assert_float('6f788d47', select(2, string.unpack(encode('478d786f'), 'f'))) -- 32bit representation of 1e30/13
    bysantd_assert_float('ffffffff00', niltoken)
    u.assert_true(isnan(bysantd('3604ffffffff01')[1]))

    bysantd_assert_double('3fd0000000000000', 0.25)
    bysantd_assert_double('45ef11a8e476d4d1', 1e30/13)
    bysantd_assert_double('ffffffffffffffff00', niltoken)
    u.assert_true(isnan(bysantd('3605ffffffffffffffff01')[1]))
end
