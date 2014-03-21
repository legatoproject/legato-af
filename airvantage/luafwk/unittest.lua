-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Cuero Bugot    for Sierra Wireless - initial API and implementation
--     Silvere Chabal for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

require 'coxpcall' -- TODO remove when Lua 5.2
require 'print'
local tableutils = require'utils.table'
local coxpcall = coxpcall
local copcall = copcall
local oassert = assert
local os = os
local print = print
local sprint= sprint
local string = string
local table = table
local ipairs = ipairs
local pairs = pairs
local error = error
local debug = debug
local getmetatable = getmetatable
local setmetatable = setmetatable
local tostring = tostring
local type = type
local next = next
local fmt = string.format


module(...)



testsuites = {}
local stats = {}
local isFinished = false
local testfailuretoken = {}
local startRA = {}

function addtestsuite(name, t)
    if testsuites[name] then return nil, "already existing" end
    testsuites[name] = t
    return t
end


function newtestsuite(name)
    t = {}
    return addtestsuite(name, t)
end

function resettestsuite()
  testsuites = {}
end




-------------------------------------------------------------------
-- assert functions
-------------------------------------------------------------------
local ts = tostring
local nbofassert

local function wraptest(flag, msg, reason)
    nbofassert = nbofassert +1
    if not flag then
        if msg then reason=reason.." - "..tostring(msg) end
        t = {msg = debug.traceback(reason, 3)}
        error(setmetatable(t, testfailuretoken), 0)
    end
    return flag
end

function assert(cond, msg)
    return wraptest(cond, msg, "Assert Failed")
end

-- Abort all tests from that testsuite.
-- when teardown function is present, it is executed nonetheless.
function abort(msg)
    if msg then msg = " - "..msg end
    msg = "Testcase aborted"..(msg or "")
    error(setmetatable({abort=true, msg=msg}, testfailuretoken), 0)
end

---Fail a test.
function fail(msg)
   wraptest(false, msg, "Failure")
end


---got == true.
function assert_true(got, msg)
   wraptest(got==true, msg, "Expected true, got "..ts(got))
end

---got == false.
function assert_false(got, msg)
   wraptest(got==false, msg, "Expected false, got "..ts(got))
end

--got == nil
function assert_nil(got, msg)
   wraptest(got == nil, msg, "Expected nil, got "..ts(got))
end

--got ~= nil
function assert_not_nil(got, msg)
   wraptest(got ~= nil, msg, "Expected non-nil value")
end

---exp == got.
function assert_equal(exp, got, msg)
   wraptest(exp == got, msg, fmt("Expected %q, got %q", ts(exp), ts(got)))
end

---exp ~= got.
function assert_not_equal(exp, got, msg)
   wraptest(exp ~= got, msg, "Expected something other than " .. ts(exp))
end

---val > lim.
function assert_gt(lim, val, msg)
   wraptest(val > lim, msg, fmt("Expected a value > %s, got %s", ts(lim), ts(val)))
end

---val >= lim.
function assert_gte(lim, val, msg)
   wraptest(val >= lim, msg, fmt("Expected a value >= %s, got %s", ts(lim), ts(val)))
end

---val < lim.
function assert_lt(lim, val, msg)
   wraptest(val < lim, msg, fmt("Expected a value < %s, got %s", ts(lim), ts(val)))
end

---val <= lim.
function assert_lte(lim, val, msg)
   wraptest(val <= lim, msg, fmt("Expected a value <= %s, got %s", ts(lim), ts(val)))
end

---#val == len.
function assert_len(len, val, msg)
   wraptest(#val == len, msg, fmt("Expected #val == %d, was %d", len, #val))
end

---#val ~= len.
function assert_not_len(len, val, msg)
   wraptest(#val ~= len, msg, fmt("Expected length other than %d", len))
end

---Test that the string s matches the pattern exp.
function assert_match(pat, s, msg)
   s = tostring(s)
   wraptest(type(s) == "string" and s:match(pat), msg,
            fmt("Expected string to match pattern %s, was %s",
            pat,
            (s:len() > 30 and (s:sub(1,30) .. "...") or s)))
end

---Test that the string s doesn't match the pattern exp.
function assert_not_match(pat, s, msg)
   wraptest(type(s) ~= "string" or not s:match(pat), msg, "Should not match pattern "..pat)
end


-- Test that val is a type or not a type
do
    local gtype = {"boolean", "number", "string", "table", "function", "thread", "userdata"}
    for _, t in ipairs(gtype) do
        _M["assert_"..t] =
        function(val, msg)
             wraptest(type(val) == t, msg, "Expected type "..t.." but got "..type(val))
        end

        _M["assert_not_"..t] =
        function(val, msg)
             wraptest(type(val) ~= t, msg, "Expected type other than "..t.." but got "..type(val))
        end
    end
end


---Test that a value has the expected metatable.
function assert_metatable(exp, val, msg)
   local mt = getmetatable(val)
   wraptest(mt == exp, msg, fmt("Expected metatable %s but got %s", ts(exp), ts(mt)))
end

---Test that a value does not have a given metatable.
function assert_not_metatable(exp, val, msg)
   local mt = getmetatable(val)
   wraptest(mt ~= exp, msg, "Expected metatable other than "..ts(exp))
end

---Test that the function raises an error when called.
function assert_error(f, msg)
   local ok, err = copcall(f)
   wraptest(not ok, msg, "Expected an error but got none")
end

function assert_error_match(pat, f, msg)
   local ok, err = copcall(f)
   wraptest(not ok and err:match(pat), msg, fmt("Expected an error that matches pattern %s, but got error %s", pat, ts(err)))
end

function assert_no_error(f, msg)
   local ok, err = copcall(f)
   wraptest(ok, msg, "Expected no error but got "..ts(err))
end

function assert_clone_tables(exp, got, msg)
    local d = tableutils.diff(exp, got)
    wraptest(nil == next(d), msg, fmt("Expected clone tables, but keys { %s } are different, got %s", table.concat(d, ', '), sprint(got)))
end


local function printstats(stats)

    print(" ")
    print(string.rep("=", 41).." Unit Test Report "..string.rep("=", 41))
    local totalnbtests =  stats.nbpassedtests + stats.nbfailedtests + stats.nberrortests
    print(fmt("Run xUnit tests of %d testsuites and %d testcases (%d assert conditions) in %d seconds", stats.nbtestsuites, totalnbtests, stats.nbofassert, stats.endtime-stats.starttime))
    print(fmt("\t%d passed, %d failed, %d errors", stats.nbpassedtests, stats.nbfailedtests, stats.nberrortests))
    if stats.nbfailedtests + stats.nberrortests > 0 then
        print(string.rep("-", 100))
        print("Detailed error log:")
        for i, err in ipairs(stats.failedtests) do
            print(fmt("%d) %s - %s.%s", i, err.type, err.testsuite, err.test))
            print(err.msg)
        end
    end
    if stats.nbabortedtestsuites > 0 then
        print(string.rep("-", 100))
        print(fmt("This test sequences contains %d testsuites that were aborted:", stats.nbabortedtestsuites))
        for _, name in ipairs(stats.abortedtestsuites) do print("\t"..name) end
    end
    print(string.rep("=", 100))
    print(" ")
end


local function id(...) return ... end


local function runtest(stats, testsuitename, testname, test)
    local s, err = coxpcall(test, id)
    local type
    local ret
    if not s then
        if getmetatable(err) == testfailuretoken then -- this is a test failure (i.e. unittest assertion that was not satisfied)
            if err.abort then
                type = "Abort"
                ret = "A"
                stats.nbabortedtestsuites = stats.nbabortedtestsuites + 1
                table.insert(stats.abortedtestsuites, testsuitename)
            else
                type = "Failure"
                ret = "F"
            end
            stats.nbfailedtests = stats.nbfailedtests + 1
            err = err.msg
        else -- this is a standard Lua (unexpected) error
            stats.nberrortests = stats.nberrortests + 1
            type = "Error"
            ret = "E"
        end
        table.insert(stats.failedtests, {testsuite = testsuitename, test=testname, type = type, msg=err})
    else
        stats.nbpassedtests = stats.nbpassedtests + 1
    table.insert(stats.passedtests, {testsuite = testsuitename, test=testname, type = type, msg="OK"})
        ret = "."
    end
    print(ret, testname)
    return ret
end

local function listtests(filterpattern)
    local list = {}
    local tinsert = table.insert
    for testsuitename, testsuite in pairs(testsuites) do
        local ts = {}

        for testname, test in tableutils.sortedpairs(testsuite) do
            if testname:match("^test") and (not filterpattern or (testsuitename..'.'..testname):match(filterpattern)) then
                tinsert(ts, testname)
            end
        end
        ts.setup = testsuite.setup
        ts.teardown = testsuite.teardown

        if #ts>0 then list[testsuitename] = ts end
    end
    return list
end

function run(filterpattern)
    stats = { nbpassedtests = 0,
          passedtests = {},
              nbfailedtests = 0,
              nberrortests  = 0,
              failedtests   = {},
              nbtestsuites  = 0,
              nbabortedtestsuites = 0,
              abortedtestsuites = {},
              starttime = os.time()}
    nbofassert = 0 -- module local to count asserts


    local l = listtests(filterpattern)

    for tsn, tsl in tableutils.sortedpairs(l) do
        print(fmt("Running [%s] testsuite:", tsn))
        stats.nbtestsuites = stats.nbtestsuites+1

        local s
        if tsl.setup then s = runtest(stats, tsn, "setup", tsl.setup) end
        if s ~= "A" then -- only run test until abort is encountered
            local testsuite = testsuites[tsn]
            for _, tn in ipairs(tsl) do
                s = runtest(stats, tsn, tn, testsuite[tn])
                if s=='A' then break end
            end
        end
        if tsl.teardown then runtest(stats, tsn, "teardown", tsl.teardown) end
    end

    stats.nbofassert = nbofassert

    stats.endtime = os.time()
    printstats(stats)
end

function list(p)
    local l = listtests(p)

    for tsn, ts in tableutils.sortedpairs(l) do
        print("* "..tsn)
        if ts.setup then print("\t+ ".."setup") end
        for _, tn in ipairs(ts) do print("\t- "..tn) end
        if ts.teardown then print("\t+ ".."teardown ") end
    end
end


function getStats()
  return stats
end

function isFinished()
  return isFinished
end

function configureReadyAgent(target, path, hostname, port)
  startRA.target = target
  startRA.path = path
  startRA.hostname = hostname or 'localhost'
  startRA.port = port or 1999
end

function resetReadyAgentConfig()
  startRA.target = nil
  startRA.path = nil
  startRA.hostname = nil
  startRA.port = nil
end

local function runReadyAgent()
  assert_not_nil(startRA.target)
  assert_not_nil(startRA.path)
  assert_not_nil(startRA.hostname)
  assert_not_nil(startRA.port)
  assert(0 == system.execute(startRA.path))
end