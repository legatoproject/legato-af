-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--asset tests the parameters return in hook function, see manifest file for more information
--be aware that testname is used as asset name, and must be set accordingly in Manifest
local testname="update_test_05_assetupdatehook_rm_cmp"
local racon = require"racon"
local sched = require"sched"

local asset

local function uhook(name, version, path, parameters)
    local param = {name = name, version = version, path = path, parameters = parameters}
    p(testname, param)

    if not param then log(testname, "ERROR", "not param") return end
    if param.name and param.name~="update" then log(testname, "ERROR", "name in param is invalid") return end
    if param.version and param.parameters and param.parameters.reason~="test" then log(testname,"ERROR", "parameters inconsistent with Manifest")  return end
    if not param.version and param.parameters and param.parameters.reason~="bye bye" then log(testname,"ERROR", "parameters inconsistent with Manifest")  return end

    sched.signal(testname, "ok")
    return 200
end

local function unload()
    asset:close()
end

local function load()
    asset  = assert(racon.newasset(testname))
    assert(asset:setupdatehook(uhook))
    assert(asset:start())
end


--see update unit test api in update.lua unit test

--(Note: there two components being installed 
-- during this update, the "success" signal in sent by the last component update hook,
-- let set the timeout a little bit bigger than usual)
return { timeout=4, signalname=testname, unload =  unload, load = load, status="success"}
