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
--     Julien Desgats     for Sierra Wireless - initial API and implementation
--     Fabien Fleutot     for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

require 'strict'
local m3da_deserialize = require "m3da.bysant".deserializer()
require 'stagedb'
require 'ltn12'
local tableutils = require 'utils.table'
require 'pack'
local u = require 'unittest'

--- @field #stagedb_generic
local t = u.newtestsuite("stagedb generic")

local x

function os_time() return 1311013410 end
--function os_time() return 100 end

--Empties the database and returns its flushed content, as a string, followed by its length
--Chain : sdb handler -> sdb:serialise -> m3da_deserialise -> flush data
--input : rawdata
--output: flush data
local function flush_data(db)
    local src = db :serialize()
    local snk, tbl = ltn12.sink.table()
    ltn12.pump.all(src, snk)
    local str = table.concat(tbl)
    local value = m3da_deserialize(str)
    return value, #str
end

--setup :  create a db ram to store data
function t :setup()
    x = stagedb("ram:foo.db", { 'temperature',  'pressure', 'timestamp' })
    x.BLOCKSIZE=300
end

-- fill data in db ram
local function feed_data(db, nrows)
    for i=1, (nrows+4)/5 do
        db :row{ temperature = 27, pressure = 1013, timestamp = os_time()}
        if 5*(i-1)+1==nrows then break end
        db :row{ temperature = 23, pressure = 1014, timestamp = os_time()+1 }
        if 5*(i-1)+2==nrows then break end
        db :row{ temperature = 22, pressure = 1015, timestamp = os_time()+2 }
        if 5*(i-1)+3==nrows then break end
        db :row{ temperature = 20, pressure = 1016, timestamp = os_time()+3 }
        if 5*(i-1)+4==nrows then break end
        db :row{ temperature = 21, pressure = 1098, timestamp = os_time()+4 }
        db :trim()
    end
end

--get new_data from flush data according to __class
local function get_data_according_to_class(data, nrows)
    local new_data={}
    for v, column in pairs(data) do
        local Y = {}
        --if data is deserialised like rawdata, so we just check number of line
        if column.__class==nil then
            u.assert_equal(#column, nrows)

        --if data is deserialised like 'QuasiPeriodicVector' class
        elseif column.__class=='QuasiPeriodicVector' then
            local v = column.start
            Y[1]=v
            for i=1, #column.shifts, 2 do
                local n, s = column.shifts[i], column.shifts[i+1]
                for count = 1, n do
                    v = v + column.period
                    table.insert(Y, v)
                end
                if s then -- not the last run
                    v = v + column.period + s
                    table.insert(Y, v)
                end
            end
        --if data is deserialised like 'DeltasVector'class
        elseif column.__class=='DeltasVector' then
            Y[1] = column.factor * column.start
            for i = 1, #column.deltas, 1 do
                Y[i+1] = Y[i] + column.factor * column.deltas[i]
            end
        else error("Column class "..column.__class.." not supporter or not implement") end

        if #Y>0 then new_data[v] = Y  -- if we passed by "PeriodicVector" or "DeltasVector"
        else new_data[v]=data[v] end
    end
    return new_data
end

local function check_data(data, nrows)
    local temps = { 27, 23, 22, 20, 21 }
    local pressures = { 1013, 1014, 1015, 1016, 1098 }
    local time = os_time()

    local new_data = get_data_according_to_class(data, nrows)

    for i = 1, nrows do
        local mod5 = (i-1) % 5 + 1
        u.assert_equal(temps[mod5], new_data.temperature[i])
        u.assert_equal(pressures[mod5], new_data.pressure[i])
        u.assert_equal(time-1+mod5, new_data.timestamp[i])
    end
end

local function test_nrows_blocksize(nrows, blocksize)
    return function()
        u.assert (x :reset())
        feed_data(x, nrows)
        x.BLOCKSIZE=blocksize
        local data = u.assert(flush_data(x))
        check_data(data, nrows)
    end
end

t.test_2_1 = test_nrows_blocksize(2, 1)
t.test_10_10 = test_nrows_blocksize(10, 10)
--t.test_100_100 = test_nrows_blocksize(100, 100)
--t.test_100_1 = test_nrows_blocksize(100, 1)
--t.test_10000_1000 = test_nrows_blocksize(10000, 1000)
--t.test_10000_64K = test_nrows_blocksize(10000, 0xffff)

--test the storage of stagedb in file
function t :test_file_storage()
    local path = os.tmpname()
    local fx = stagedb("file:"..path, { 'temperature',  'pressure', 'timestamp' })
    feed_data(fx, 10)
    check_data(flush_data(fx), 10)
    fx :close()

    local fx2 = stagedb("file:"..path, { 'temperature',  'pressure', 'timestamp' })
    check_data(flush_data(fx2), 10)
    fx2 :reset()
    feed_data(fx2, 5)
    check_data(flush_data(fx2), 5)
    u.assert(os.remove(path))
end

--test consolidation 1
function t :test_conso1()
    local y = x :newconsolidation("ram:test_conso1", { temperature='max', pressure='mean' })
    feed_data(x, 20)
    u.assert(x :consolidate())
    local y2 = flush_data(y)
    u.assert_equal(#y2.pressure, 1)
    u.assert_equal(#y2.temperature, 1)
    u.assert_equal(y2.temperature[1], 27)
    u.assert(y2.pressure[1] > 1013)
    u.assert(y2.pressure[1] < 1098)
    u.assert_nil(u.timestamp)

    u.assert_nil(x :newconsolidation("ram:test_conso1", { temperature='max', pressure='mean' }),
        "setting a 2nd conso table should not be possible")
end

--test consolidation 2
function t :test_conso2()
    local function test_consolidation_helper(method, expected)
        local raw = u.assert(stagedb("ram:raw.db", { { name="temp", serialization="smallest"} }))
        local y = u.assert(raw :newconsolidation("ram:test_conso2",
            { {name="temp", serialization="fastest", consolidation=method} }))
        u.assert(raw :row{ temp=15 } :row{ temp=18 } :row{ temp=16 })
        u.assert(raw :consolidate())
        local nrows = y :state().nrows
        u.assert_equal(nrows, 1, "Incorrect row count: "..nrows)
        local y2 = flush_data(y)
        u.assert_equal(y2.temp[1], expected, "Incorrect value: "..y2.temp[1])
    end

    test_consolidation_helper("sum", 49)
    test_consolidation_helper("mean", 16+1/3)
    test_consolidation_helper("min", 15)
    test_consolidation_helper("max", 18)
    test_consolidation_helper("median", 16)
    test_consolidation_helper("first", 15)
    test_consolidation_helper("last", 16)
    test_consolidation_helper("middle", 18)
end

-- test that factor parameter is taken in account for consolidation tables
function t :test_conso_precision()
    local raw = u.assert(stagedb("ram:raw", { { name="temp", serialization="smallest"} }))
    local consolidated = u.assert(raw :newconsolidation("ram:consolidated-first",
        { {name="temp", serialization="deltasvector", consolidation="median", factor=0.1 } }))
    u.assert(raw :row{ temp=15.3 } :row{ temp=18.6 } :row{ temp=16.1 })
    u.assert(raw :consolidate() :reset())
    u.assert(raw :row{ temp=15.3 } :row{ temp=18.6 } :row{ temp=17.3 })
    u.assert(raw :consolidate())
    local y2 = flush_data(consolidated).temp
    u.assert_equal(161, y2.start)
    u.assert_equal(12, y2.deltas[1])
end

function t :test_conso_empty()
    local raw = u.assert(stagedb("ram:raw", { "temp" }))
    local consolidated = u.assert(raw :newconsolidation("ram:consolidated",
        { { name='temp', consolidation='min', serialization='fastest' } }))
    u.assert_equal(0, consolidated:state().nrows)
    u.assert_nil(raw :consolidate())
    u.assert_equal(0, consolidated:state().nrows)
end

-- test methods that use copy_data function
function t :test_conso_copy_data()
    local raw = u.assert(stagedb("ram:raw", { "temp" }))
    local consolidated = u.assert(raw :newconsolidation("ram:consolidated",
        { { name='temp', consolidation='middle', serialization='fastest' } }))
    u.assert(raw :row{ temp=15.3 } :row{ temp=16.1 } :row{ temp=18.6 })
    u.assert(raw :consolidate() :reset())
    u.assert(raw :row{ temp=15.3 } :row{ temp=17.3 } :row{ temp=18.6 })
    u.assert(raw :consolidate())

    u.assert_clone_tables({ 16.1, 17.3 }, flush_data(consolidated).temp)
end

--test writing of stagedb data when we close it
function t :test_broken()
    local t = u.assert(stagedb("ram:raw", { "temp" }))
    u.assert(t :close())
    u.assert_nil(t :row{ temp=12 })
    u.assert(t :close()) -- check that close a table twice is a no-op

    -- try with a file based table
    local path = os.tmpname()
    t = u.assert(stagedb("file:"..path, { "temp" }))
    u.assert(t :close())
    u.assert_nil(t :row{ temp=12 })
    u.assert(t :close())

    -- error in consolidation spec
    local tbl = stagedb('ram:tbl', {'a', 'b'})
    u.assert_error(function() tbl:newconsolidation('ram:conso', { a='min', c='max' }) end)
    u.assert(os.remove(path))
end

--- @field #stagedb_containers
local cont_ts = u.newtestsuite("stagedb containers")

-- Deltas Vector
local function test_deltasvector_factory(n, blocksize, divisor)
    divisor = divisor or 1
    return function()
        local db = stagedb("ram:foo.db", { { name="col", serialization="deltasvector", factor=10/divisor } })
        if blocksize then db.BLOCKSIZE = blocksize end
        local deltas = { }
        db:row{ col=20/divisor }
        for i=1, n do
            db:row{ col=30/divisor }:row{ col=50/divisor }:row{ col=20/divisor }
            deltas[#deltas + 1], deltas[#deltas + 2], deltas[#deltas + 3] = 1, 2, -3
        end
        local result = flush_data(db)
        u.assert_clone_tables({
            col = {
                start = 2,
                factor = 10/divisor,
                deltas = deltas,
                __class = "DeltasVector",
            },
        }, result)
    end
end

cont_ts.test_deltas_vector_basic = test_deltasvector_factory(1)
cont_ts.test_deltas_vector_10_1 = test_deltasvector_factory(10, 1)
cont_ts.test_deltas_vector_100_5 = test_deltasvector_factory(100, 5)
cont_ts.test_deltas_vector_1k_16 = test_deltasvector_factory(1000, 16)
cont_ts.test_deltas_vector_10k_64k = test_deltasvector_factory(10000, 0xffff)
cont_ts.test_deltas_vector_with_doubles  = test_deltasvector_factory(1, nil, 100)

function cont_ts.test_deltasvector_nil()
    local db = stagedb("ram:foo.db", { { name="col", serialization="deltasvector", factor=10 } })
    db:row{ col=12 }:row{ col="foo" }:row{ col=0 }  -- we give a different type
    u.assert_nil(flush_data(db))  -- we expected a nil or a error??
    --u.assert_error(function() flush_data(db) end)
end

--QuasiPeriodic Vector
local function test_qpv_factory(n, blocksize)
    local period = 20
    local sequence = { 0,0,0,1,0,0,-2 }
    local shifts = { 3,1, 2,-2 }
    local startvalue = 143

    return function()
        local db = stagedb("ram:foo.db", { { name="col", serialization="quasiperiodicvector", period=20 } })
        if blocksize then db.BLOCKSIZE = blocksize end
        local value, allshifts = startvalue, { }
        db:row{ col=value }
        for i=1, n do
            for _, shift in ipairs(sequence) do
                value = value + period + shift
                db:row{ col=value }
            end
            for _,v in ipairs(shifts) do allshifts[#allshifts+1] = v end
        end
        allshifts[#allshifts+1] = 0
        local result = flush_data(db)
        u.assert_clone_tables({
            col = {
                start = 143,
                period = 20,
                shifts = allshifts,
                __class = "QuasiPeriodicVector",
            },
        }, result)
    end
end

cont_ts.test_quasiperiodic_vector_basic = test_qpv_factory(1, 1024)
cont_ts.test_quasiperiodic_vector_10_1 = test_qpv_factory(10, 1)
cont_ts.test_quasiperiodic_vector_100_5 = test_qpv_factory(100, 5)
cont_ts.test_quasiperiodic_vector_1k_16 = test_qpv_factory(1000, 16)
cont_ts.test_quasiperiodic_vector_10k_64k = test_qpv_factory(10000, 0xffff)

function cont_ts.test_quasiperiodic_vector_negative()
    local db = stagedb("ram:foo.db", { { name="col", serialization="quasiperiodicvector", period=0 } })
    db :row{ col=-10 } :row{ col=-10 } :row{ col=-10 } :row{ col=-11 }: row{ col = -11 }
    local result = flush_data(db)
    u.assert_clone_tables({
        col = {
            start = -10,
            period = 0,
            shifts = { 2, -1, 1 },
            __class = "QuasiPeriodicVector",
        },
    }, result)
end

-- Test encoding as 32bit float
function cont_ts.test_32bit_float()
    local function asfloat(d) return (select(2, string.pack("f", d):unpack("f"))) end
    local db, result, dsize, fsize
    -- test with plain doubles (first 3 rows exceeds 32bit float precision)
    local db = stagedb("ram:foo.db", { { name="col", serialization="list", asfloat=false } })
    db :row{ col = 0.123456789 } :row{ col = 9876.54321 } :row{ col = 1e-100 } :row{ col = 0.5 }
    result, dsize = flush_data(db)
    u.assert_clone_tables({ col = {0.123456789, 9876.54321, 1e-100, 0.5} }, result)

    -- now with float shrinking
    db = stagedb("ram:foo.db", { { name="col", serialization="list", asfloat=true } })
    -- these numbers cannot be represented exactly in 32bit float
    db :row{ col = 0.123456789 } :row{ col = 9876.54321 } :row{ col = 1e-100 } :row{ col = 0.5 }
    result, fsize = flush_data(db)
    u.assert_clone_tables({ col = {asfloat(0.123456789), asfloat(9876.54321), 0, 0.5} }, result)
    -- 2 numbers has been shrinked from 9 bytes to 5 and the last to 1 byte
    -- so 2*4 + 8 = 16 saved bytes compared to plain doubles
    u.assert_equal(dsize - 16, fsize)
end

function cont_ts.test_smallest_basic()
    local db = stagedb("ram:foo.db", {
        { name="timestamp", serialization="smallest" },    -- QPV expected (constant period)
        { name="temp",      serialization="smallest" },    -- QPV expecred (quasi constant data)
        { name="growing",   serialization="smallest" },    -- DV expected (no fixed period)
        { name="mixed",     serialization="smallest" },    -- List expected (mixed types)
    })

    db :row{ timestamp=os_time(),     temp=35, growing=12,  mixed=6 }
        :row{ timestamp=os_time()+60,  temp=35, growing=42,  mixed="foo" }
        :row{ timestamp=os_time()+119, temp=35, growing=124, mixed=math.pi }
        :row{ timestamp=os_time()+179, temp=33, growing=126, mixed=0 }
        :row{ timestamp=os_time()+240, temp=33, growing=137, mixed="finished!" }
        :row{ timestamp=os_time()+300, temp=33, growing=138, mixed=true }
    local result = flush_data(db)

    -- just test serialization method and automated computations, data correctness is tested elsewhere
    --FIXME AWTDA3 QPV
    u.assert_equal("QuasiPeriodicVector", result.timestamp.__class)
    u.assert_equal(60, result.timestamp.period)

    --FIXME AWTDA3 QPV
    u.assert_equal("QuasiPeriodicVector", result.temp.__class)
    u.assert_equal(0, result.temp.period)

    u.assert_equal("DeltasVector", result.growing.__class)
    u.assert_equal(1, result.growing.factor)

    u.assert_true(tableutils.isarray(result.mixed))

    --*now add some more values and serialize again*--
    db :row{ timestamp=os_time()+361, temp=34,       growing=201,  mixed=false }
        :row{ timestamp=os_time()+421, temp="failed", growing=211,  mixed=false }
    result = flush_data(db)

    --FIXME AWTDA3 QPV
    u.assert_equal("QuasiPeriodicVector", result.timestamp.__class)
    u.assert_equal(60, result.timestamp.period)

    u.assert_true(tableutils.isarray(result.temp)) -- last value caused a fallback to list ??

    u.assert_equal("DeltasVector", result.growing.__class)
    u.assert_equal(1, result.growing.factor)

    u.assert_true(tableutils.isarray(result.mixed))
end

--FIXME AWTDA3 QPV
--[[
-- Some more detailed tests for QPV
function cont_ts.test_smallest_advanced_qpv()
local db = stagedb("ram:foo.db", { { name="col", serialization="smallest" } })
-- corner case: deltasum < n*period
db :row{ col=10 } :row{ col=20 } :row{ col=29 } :row{ col=39 } :row{ col=48 } :row{ col=58 } :row{ col=68 }
local result = flush_data(db)
u.assert_equal(10, result.col.period)
end
--]]

function cont_ts.test_smallest_with_float()
    local function feedandflush(db)
        db :row{ col=10 } :row{ col=20.1 } :row{ col=30.7 } :row{ col=39.6 } :row{ col=50.1 }
        return flush_data(db)
    end
    local db, result

    -- without forced factor
    db = stagedb("ram:foo.db", { { name="col", serialization="smallest" } })
    result = feedandflush(db)
    u.assert_clone_tables({ col = { 10, 20.1, 30.7, 39.6, 50.1 } }, result)

    -- with forced factor
    db = stagedb("ram:foo.db", { { name="col", serialization="smallest", factor=0.1 } })
    result = feedandflush(db)
    u.assert_clone_tables({
        col = {
            start = 100,
            factor = 0.1,
            deltas = { 101, 106, 89, 105 },
            __class = "DeltasVector",
        },
    }, result)
end

function cont_ts.test_smallest_with_factor()
    -- 1st case: factor makes the DV smaller
    local db = stagedb("ram:foo.db", { { name="col", serialization="smallest", factor=10 } })
    db :row{ col = 1000 } :row{ col = 1011 } :row{ col = 1020 } :row{ col = 1031 } :row{ col = 1040 }
    local result = flush_data(db)
    u.assert_clone_tables({
        col = {
            start = 100,
            factor = 10,
            deltas = { 1, 0, 2, 0 }, -- TODO: this result is correct but is it really what we want ?
            __class = "DeltasVector",
        },
    }, result)

    -- 2nd case: even with factor, QPV is smaller
    --FIXME AWTDA3 QPV
    db = stagedb("ram:foo.db", { { name="col", serialization="smallest", factor=10 } })
    db :row{ col = 1000 } :row{ col = 1010 } :row{ col = 1020 } :row{ col = 1030 } :row{ col = 1040 }
    result = flush_data(db)
    u.assert_equal(result.col.__class,  "QuasiPeriodicVector")
end

function cont_ts.test_smallest_with_table_reset()
    local db = stagedb("ram:foo.db", { { name="col", serialization="smallest" } })
    -- fill the table with a period of 10
    db :row{ col = 1000 } :row{ col = 1010 } :row{ col = 1020 } :row{ col = 1030 } :row{ col = 1040 }
    local result = flush_data(db)
    --FIXME AWTDA3 QPV
    u.assert_equal(1000, result.col.start)
    u.assert_equal(10, result.col.period)

    -- reset and fill with a period of 12
    db :reset() :row{ col = 1000 } :row{ col = 1012 } :row{ col = 1024 } :row{ col = 1036 } :row{ col = 1048 }
    result = flush_data(db)
    --FIXME AWTDA3 QPV
    u.assert_equal(1000, result.col.start)
    u.assert_equal(12, result.col.period)
end

function cont_ts.test_smallest_with_4bytes_floats()
    local db = stagedb("ram:foo.db", { { name="col", serialization="smallest", factor=0.02, asfloat=true } })
    local result
    -- fill with data set which is shorter in DV than in vector as 4 bytes float
    db :row{ col = 0.2 } :row{ col = 0.6 } :row{ col = 1.2 } :row{ col = 1.6 } :row{ col = 2.2 }
    result = flush_data(db)
    u.assert_equal("DeltasVector", result.col.__class)

    -- fill with data set which is shorter in vector as 4 bytes float than in DV
    db :reset()
    db :row{ col = 0.2 } :row{ col = 30154.6 } :row{ col = -56034.5 } :row{ col = 12049.4 } :row{ col = 86514.2 }
    result = flush_data(db)
    u.assert_true(tableutils.isarray(result.col))
end

function cont_ts.test_smallest_with_file_storage()
    local filename = os.tmpname()
    local db = stagedb("file:"..filename, { { name="col", serialization="smallest" } })
    --FIXME AWTDA3 QPV
    local expected = {
        col = {
            start = 1000,
            period = 10,
            shifts = { 4 },
            __class = "QuasiPeriodicVector",
        },
    }

    -- fill db with some data and close it
    db :row{ col = 1000 } :row{ col = 1010 } :row{ col = 1020 } :row{ col = 1030 } :row{ col = 1040 }
    local result = flush_data(db)
    u.assert_clone_tables(expected, result)
    db :close()

    -- reload db and serialize again, we really expected this "expected" value ???
    expected = {
        col = {
            1000,
            1010,
            1020,
            1030,
            1040 } }

    db = stagedb("file:"..filename, { { name="col", serialization="smallest" } })
    result = flush_data(db)
    u.assert_clone_tables(expected, result)

    db :close()
    u.assert(os.remove(filename)) -- FIXME: this is not executed if test fails !
end
