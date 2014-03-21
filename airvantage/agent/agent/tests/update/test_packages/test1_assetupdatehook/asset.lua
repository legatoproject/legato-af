-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

--be aware that testname is used as asset name, and must be set accordingly in Manifest
local testname="update_test_01_assetupdatehook"
local racon = require"racon"
local sched = require"sched"

local asset

local function uhook(name, version, path, parameters)
    local _, output = require"utils.system".pexec("readlink -f -n .") -- get the absolu path of runtime dir
    local absolupath = output .. "/update/tmp/pkg_standalone_pkg/."  -- "." corresponds to the location of package in the manifest file
    if name~="update" or version~="1" or absolupath~=path or not parameters.autostart or parameters.bar ~=42 or parameters.foo~='test' then
        log(testname, "ERROR", "%s : parameters in package are not valid", testname)
        return --by default, a code 471 (User update callback failed) is return to RA
    end
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


--see api in unit test
return { timeout=3, signalname=testname, unload =  unload, load = load, status="success"}
