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

local u = require"unittest"
local config = require"agent.config"
local log  = require"log"

local update = require "agent.update"
local ucommon = require"agent.update.common"
local umgr = require"agent.update.updatemgr"
local upkg = require "agent.update.pkgcheck"

----------------------------------------------------------------------
----------------------------------------------------------------------
-- manifest checks : format, deps, version comparisons
--
----------------------------------------------------------------------
----------------------------------------------------------------------

local t = u.newtestsuite("update.manifest")
local initswlist
local curupdate

-- Those tests are likely to be converted to real packages
-- containing harmless update scripts.
-- Those real package would be test using local install.

function t:setup()

    initswlist = ucommon.data.swlist
    curupdate = ucommon.data.currentupdate

end


function t:test_01_package_format()
    --TODO
end


local manifest
--util to run an assert_nil, then assert_match on sereval error patterns given as parameters
local function testfail(...)
    local patterns = {...}
    local res, errcode, errstr = upkg.checkmanifest(manifest)
    log("UPDATE-TEST", "DEBUG", "testfail: upkg.checkmanifest res=%s, errcode=%s, errstr=%s", tostring(res), tostring(errcode), tostring(errstr))
    local err = errstr or errcode--error string to parse/match can be in errcode (some assert failed), or in errstr: internally caught/tested error)
    u.assert_nil(res, "must have an error")
    for k,pattern in pairs(patterns) do
        u.assert_match(pattern, err)
    end
end


function t:test_02_manifest_format()
    ucommon.data.swlist = { components={ {name = "toto", version ="1.0"}} }
    --next one is need for location test
    ucommon.data.currentupdate = { update_directory = "/tmp"}

    -- device type must be set
    manifest = { components={ { name = "titi", version = "1.1" } } }
    -- version is optional but must not be ""
    manifest.version = ""
    testfail("version", "manifest")
    --set correct value for version
    manifest.version = "versiontest"
    --force must be a boolean when set
    manifest.force = "fdsfsd"
    testfail("force", "manifest")
    --set correct value for
    manifest.force=false
    -- components mandatory
    manifest.components = nil
    testfail("components", "manifest")
    -- components must be a table
    manifest.components = "fsdqklfjds"
    testfail("components", "manifest")
    -- now do checks for component field
    -- name mandatory
    manifest.components={ { version = "1.1" } }
    testfail("name")
    -- name must different from ""
    manifest.components={ { name = "", version = "1.1" } }
    testfail("name", "component")
    -- version must be a string when set
    manifest.components={ { name = "titi", version = {} } }
    testfail("version", "titi")
    --set version correctly
    --location must be a string when set
    manifest.components={ { name = "titi", version = "1.2", location = 3 } }
    testfail("location", "titi")
    --location must point to a valid file, /tmp/test_update_package_cheking file must NOT exist
    manifest.components={ { name = "titi", version = "1.2", location = "test_update_package_cheking" } }
    testfail("location", "titi")
    --location must be set when version is set
    manifest.components={ { name = "titi", version = "1.2"} }
    testfail("location", "titi")
    --set location correctly: this depends on the fact that /tmp/. should always exist.
    --no depends and no provides when cmp is removed (i.e. when no version)
    manifest.components={ { name = "titi", location = ".", provides = {} } }
    testfail("provides","titi")
    manifest.components={ { name = "titi", location = ".", depends = {} } }
    testfail("depends","titi")
    manifest.components={ { name = "titi", location = ".", provides = {}, depends =  {} } }
    testfail("provides","titi")
    manifest.components={ { name = "titi", location = ".", depends = {}, provides =  {} } }
    testfail("depends","titi")

    --now put back version, and set depends and provides fields: check those two fields
    -- provides/depends must be table with key and value as strings
    manifest.components={ { name = "titi", version = "1.2", location = ".", depends = { "foo", "bar" } } }
    testfail("depends","titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", depends = { foo ="bar", d=1 } } }
    testfail("depends","titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { "foo", "bar" } } }
    testfail("provides","titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { foo ="bar", d=1 } } }
    testfail("provides","titi")
    --provides list must not contain the name of the component
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { titi ="bar", d="1" } } }
    testfail("provides","titi")
    -- key or values cannot be empty strings
    manifest.components={ { name = "titi", version = "1.2", location = ".", depends = { [""] = "bar" } } }
    testfail("depends","titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", depends = { foo = "" } } }
    testfail("depends","titi")
    -- key or values cannot be empty strings
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { [""] = "bar" } } }
    testfail("provides","titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { foo = "" } } }
    testfail("provides","titi")

    --set provides and depends correctly
    --now only dependencies issues should remain
    -- this will be addressed in other tests cases
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { foo ="bar", d="1" }, depends = { foo ="bar", d="1" } } }
    testfail("depend")

    --versions fields (global, per cmp and per feature) cannot contain invalid char like <, = , &, ...
    manifest.version = "toto="
    testfail("version", "manifest")
    --put global version to a correct value
    manifest.version = "versiontest"
    --tesqt invalid version in component
    manifest.components={ { name = "titi", version = "1.2>", location = ".", provides = { foo ="bar", d="1" }, depends = { foo ="bar", d="1" } } }
    testfail("version", "titi")
    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { foo ="bar", d="1<" }, depends = { foo ="bar", d="1" } } }
    testfail("provides", "titi")

    --force parameter must disable dependence checking
    manifest.force = true

    manifest.components={ { name = "titi", version = "1.2", location = ".", provides = { foo ="bar", d="1" }, depends = { foo ="bar", d="1" } } }
    u.assert(upkg.checkmanifest(manifest))

    --note cannot test component location field correclty in this test case where upkg.checkmanifest fct is called directly but no package
    -- have been extracted in appropriate folder.
    --it has to be done in a real pkg to check the location file/folder exists
end


function t:test_03_cmp_version()

    u.assert(upkg.cmp_version( "1.0", "=1.0"))
    u.assert(upkg.cmp_version( "1.0", ">=1.0"))
    u.assert(upkg.cmp_version( "1.0", "<=1.0"))
    u.assert(upkg.cmp_version( "1.1", ">1.0"))
    u.assert(upkg.cmp_version( "1.1", ">=1.0"))
    u.assert_not_equal(upkg.cmp_version("1.1", "=1.0"))
    u.assert_not_equal(upkg.cmp_version("1.1", "<=1.0"))
    u.assert_not_equal(upkg.cmp_version("1.1", "<1.0"))
    -- not accepted character:
    u.assert_nil(upkg.cmp_version("1.1", "<=1.&0"))
    u.assert_nil(upkg.cmp_version("1.1", ">1.*0"))
    u.assert_nil(upkg.cmp_version("1.1", ">=1.*0"))
    u.assert_nil(upkg.cmp_version("1.1", "=1.&0"))
    u.assert_nil(upkg.cmp_version("1.1", "=1,0"))
    --invalid ops
    u.assert_nil(upkg.cmp_version("1.1", "<<1.0"))
    u.assert_nil(upkg.cmp_version("1.1", "=>1.0"))
    u.assert_nil(upkg.cmp_version("1.1", ">>1.0"))
    u.assert_nil(upkg.cmp_version("1.1", "=<1.0"))
    u.assert_nil(upkg.cmp_version("1.1", "==1.0"))
    --operator is mandatory:
    u.assert_nil(upkg.cmp_version("1.1", "1.1"))
    --ascii order:
    --Example: "2.1" > "1.3", "3.a" > "3.9", "1.a" > "1.Z" etc.
    u.assert(upkg.cmp_version("2.1", ">1.3"))
    u.assert(upkg.cmp_version("3.a", ">3.9"))
    u.assert(upkg.cmp_version("1.a", ">1.Z"))
    u.assert(upkg.cmp_version("1.9", ">1.10"))
    u.assert(upkg.cmp_version("1.09", "<1.10  "))
    --not equal test
    u.assert(upkg.cmp_version("2.1", "!=3.3"))
    u.assert(upkg.cmp_version("toto", "!=titi"))
    u.assert_nil(upkg.cmp_version("1.1", "!=1.1"))
    --list
    u.assert(upkg.cmp_version("2.1", "#2.1,1.2"))
    u.assert(upkg.cmp_version("2.1", "#2.1"))
    u.assert(upkg.cmp_version("2.1", "#2.1,"))
    u.assert(upkg.cmp_version("2.1", "#2.1, "))
    u.assert_nil(upkg.cmp_version("2.1", "#2.2, "))
    u.assert_nil(upkg.cmp_version("2.1", "#2.2"))
    u.assert(upkg.cmp_version("2.1", "#4.2,2.1,1.2"))
    u.assert(upkg.cmp_version("2.1", "#4.2,6.1,1.2,2.1"))
    u.assert(upkg.cmp_version("2.1", "#4.2,6.1,1.2,2.1 <3.1"))
    u.assert_nil(upkg.cmp_version("2.1", "#4.2,6.1,1.2,2.1 >3.1"))
    --using list still make a logicial AND on all deps (even if this not really useful)
    u.assert_nil(upkg.cmp_version("2.1", ">1.2 #foo,2.1,2.2 toto"))
    u.assert_nil(upkg.cmp_version("2.1", ">1.2 #foo,2#1,2.2"))
    u.assert_nil(upkg.cmp_version("2.1", ">1.2 #fo&o,21,2.2"))
    u.assert_nil(upkg.cmp_version("2.1", ">1.2 #foo,21,2.2<"))
end

function t:test_04_manifest_deps_new_cmp()

    ucommon.data.currentupdate = { update_directory = "/tmp"}
    ucommon.data.swlist = {

        components={
            {
                name = "cmp1",
                provides = { feat1="1.0"},
                version = "0.1"
            },
            {   name = "cmp2",
                depends = { feat1=">0.1 <2.0" },
                provides = {   feat2= "1.0" },
                version = "0.2"
            }
        }
    }

    manifest = {
        -- Global information
        version = "1.1",

        -- Components information
        components =
        {
            {   name = "cmp3",
                location = ".",
                depends = { feat1="=1.0" },
                provides = { feat3="1"},
                version = "1"
            }
        }
    }

    -- simple update, one new cmp, manifest and software list are ok
    u.assert(upkg.checkmanifest(manifest))

    --test unsatisfied dependency
    manifest.components[1].depends.feat1="=2.0"
    testfail("cmp3")

    -- manifest and software list are ok: put several dependencies, with one dep on a cmp
    manifest.components[1].depends = { feat1="=1.0",  feat2=">0.1", cmp1 = "=0.1" }
    u.assert(upkg.checkmanifest(manifest))
    -- put one bad dep in several deps
    manifest.components[1].depends = { feat1="=1.0",  cmp1 = "=0.1", featfoo=">0.1" }
    testfail("featfoo", "cmp3")

    -- correct depends
    -- new cmp provides a feature already provided
    manifest.components[1].depends = { feat1="=1.0",  feat2=">0.1", cmp1 = "=0.1" }
    manifest.components[1].provides = { feat3="1", feat1="2.3"}
    u.assert(upkg.checkmanifest(manifest))

    -- new cmp cannot provide a feature with a name used by installed component
    manifest.components[1].provides = { cmp2="2.3"}
    testfail("feature", "cmp3")

    --can not install a cmp with the same name than an installed feature
    manifest.components =  {
        {
            name = "feat2",
            location = ".",
            depends = { feat1="=1.0" },
            provides = { feat3="1"},
            version = "1"
        }
    }
    testfail("feat2", "feature")

end

function t:test_05_manifest_deps_cmp_remove()
    ucommon.data.currentupdate = { update_directory = "/tmp"}
    ucommon.data.swlist = {

        components={
            {
                name = "cmp1",
                provides = { feat1="1.0"},
                version = "0.1"
            },
            {   name = "cmp2",
                depends = { feat1=">0.1 <2.0" },
                provides = {   feat2= "1.0" },
                version = "0.2"
            },

            {   name = "cmp3",
                depends = { cmp2=">0.1" },
                provides = {  },
                version = "0.2"
            }
        }
    }

    manifest = {
        -- Global information
        version = "1.1",

        -- Components information
        components =
        {
            {   name = "cmp3"
            }
        }
    }

    --valid component removal
    u.assert(upkg.checkmanifest(manifest))
    -- cannot remove cmp1, cmp2 depends on a feature provided by cmp1
    manifest.components = { {   name = "cmp1" }}
    testfail("cmp1", "element")


    -- cannot remove cmp2, cmp3 depends on cmp2 (directly)
    manifest.components = { {   name = "cmp2" }}
    testfail("cmp2", "element")

    -- cannot remove not installed component
    manifest.components = { {   name = "foo" }}
    testfail("foo", "installed", "removed")
    -- cannot remove a feature directly
    manifest.components = { {   name = "feat2" }}
    testfail("feat2", "installed", "removed")
    --]]
end

function t:test_06_manifest_deps_cmp_update()
    ucommon.data.currentupdate = { update_directory = "/tmp"}
    ucommon.data.swlist = {

        components={
            {
                name = "cmp1",
                provides = { feat1="1.0"},
                version = "0.1"
            },
            {   name = "cmp2",
                depends = { feat1=">0.1 <2.0" },
                provides = {   feat2= "1.0" },
                version = "0.2"
            },

            {   name = "cmp3",
                depends = { cmp2=">0.1" },
                provides = {  },
                version = "0.2"
            }
        }
    }

    manifest = {
        -- Global information
        version = "1.1",

        -- Components information
        components =
        {
            {   name = "cmp3",
                location = ".",
                depends = { cmp2=">0.1" },
                provides = { feat3="foo"  },
                version = "0.2"
            }
        }
    }

    --valid component update
    u.assert(upkg.checkmanifest(manifest))
    -- cannot update cmp1, new depends are not ok
    manifest.components = {
        {
            name = "cmp1",
            location = ".",
            depends = {foo="=1", bar=">4"},
            provides = { feat1="1.0", feat44="44"},
            version = "0.1"
        }}
    testfail("cmp1", "depend")
    --new depends are not ok
    -- valid update cmp1, because it provides a new feature already provided by another cmp
    manifest.components = {
        {
            name = "cmp1",
            location = ".",
            depends = {feat1=">=1.0"},
            provides = { feat1="1.4", feat2="3.0"},
            version = "0.4"
        }}
    u.assert(upkg.checkmanifest(manifest))
    -- cannot update cmp1, because it provides a new feature with same name as a component installed
    manifest.components = {
        {
            name = "cmp1",
            location = ".",
            depends = {feat1="=1.0"},
            provides = { cmp2="foo" },
            version = "0.4"
        }}
    testfail("cmp1", "feature")

    -- cannot update cmp1, because it doesn't provides a feature already needed by another cmp
    manifest.components = {
        {
            name = "cmp1",
            location = ".",
            depends = {feat1="=1.0"},
            provides = { feat66="3.0"},
            version = "0.4"
        }}
    testfail("cmp1", "element")
    -- cannot update cmp1, because it changes the version of afeature already needed by another cmp in other version
    manifest.components = {
        {
            name = "cmp1",
            location = ".",
            depends = {feat1="=1.0"},
            provides = { feat1="2.1", feat66="3.0"},
            version = "0.4"
        }}
    testfail("cmp1", "feature")
    -- cannot update feat1, because it's a feature, not a cmp
    manifest.components = {
        {
            name = "feat1",
            location = ".",
            depends = {feat1="=1.0"},
            provides = { feat1="2.1", feat66="3.0"},
            version = "0.4"
        }}
    testfail("feat1", "feature")

end


function t:test_07_manifest_deps_mixed_update_remove()

    ucommon.data.currentupdate = { update_directory = "/tmp"}
    ucommon.data.swlist = {

        components={
            {
                name = "cmp1",
                provides = { feat1="1.0"},
                version = "0.1"
            },
            {   name = "cmp2",
                depends = { feat1=">0.1 <2.0" },
                provides = {   feat2= "1.0" },
                version = "0.2"
            },

            {   name = "cmp3",
                depends = { cmp2=">0.1" },
                provides = {  },
                version = "0.2"
            }
        }
    }

    manifest = {
        -- Global information
        version = "1.1",

        -- Components information
        components =
        {
            {   name = "cmp3"
            },

            {   name = "cmp3",
                location = ".",
                depends = { cmp2=">0.1" },
                provides = { feat3="foo"  },
                version = "0.4"
            },
            {   name = "cmp4",
                location = ".",
                depends = { cmp3=">0.3" },
                provides = { featfoobar="foo"  },
                version = "0.1"
            },
            {   name = "cmp5",
                location = ".",
                depends = { featfoobar="#2.3,foo,1.2" },
                version = "0.1"
            }
        }
    }

    u.assert(upkg.checkmanifest(manifest))

    manifest = {
        -- Global information
        version = "1.1",

        -- Components information
        components =
        {
            {   name = "cmp3"
            },

            {   name = "cmp3",
                location = ".",
                depends = { cmp2=">0.1" },
                provides = { feat3="foo"  },
                version = "0.4"
            },
            {   name = "cmp4",
                location = ".",
                depends = { cmp3=">0.3" },
                provides = { featfoobar="foo"  },
                version = "0.1"
            },
            {   name = "cmp4"
            },
            {   name = "cmp5",
                location = ".",
                depends = { featfoobar="#2.3,foo,1.2" },
                version = "0.1"
            }
        }
    }

    testfail("cmp5", "element")

end


function t:teardown()
    ucommon.data.swlist = initswlist
    ucommon.data.currentupdate = curupdate
end




----------------------------------------------------------------------
----------------------------------------------------------------------
-- package checks
--
----------------------------------------------------------------------
----------------------------------------------------------------------


local sched = require"sched"
local system = require"utils.system"
local udrop = ucommon.dropdir
local createpkg = require"agent.update.tools.createpkg"



-- asset test API:
-- a table with following fields:
---
-- timeout: time to wait before compute update result
-- signalname: signal name sent at the end of the update (by some asset, appcon app, depends on the test purpose)
-- unload: optionnal function to call when cleaning the test
-- load: optionnal function to call when setting up the test
-- status: expected status of the update
--

--packages are copied to this location by cmake
local packages_location = "./update/tmp/test_packages/"

local function getcurrentswlist()
    return ucommon.data.swlist
end
local function setswlist(swlist)
    ucommon.data.swlist=swlist
    ucommon.saveswlist()
    return "ok"
end



local function loadtest(name, test)

    test.testdata = {}
    local testdata = test.testdata

    local pkgpath = packages_location.."/"..name
    local swlistpath= pkgpath.."/swlist.lua"
    local assetpath= pkgpath.."/asset.lua"

    --create the package
    testdata.generateddata = pkgpath.."/data"
    --preventive remove
    u.assert_equal(0, os.execute("rm -rf "..testdata.generateddata))
    --
    u.assert_equal(0, os.execute("mkdir "..testdata.generateddata))
    u.assert(createpkg.createpkg(pkgpath.."/pkg", testdata.generateddata))

    testdata.packagepath = testdata.generateddata.."/pkg_standalone_pkg.tar"

    testdata.initswlist = getcurrentswlist()
    testdata.testswlist = dofile(swlistpath)


    local assetdata = dofile(assetpath)
    u.assert_equal("table", type(assetdata))
    testdata.signalname = assetdata.signalname
    testdata.timeout = assetdata.timeout
    testdata.load = assetdata.load
    testdata.unload = assetdata.unload
    testdata.status = assetdata.status
    --async or sync, default on async
    testdata.type = ( assetdata.type=="sync" or assetdata.type=="async") and assetdata.type or "async"


    u.assert(setswlist(testdata.testswlist))
    u.assert(testdata.testswlist)

    --start asset code
    if testdata.load then testdata.load() end

    return "ok"
end

local function unloadtest(testdata)
    u.assert(setswlist(testdata.initswlist))
    u.assert_equal(0, system.pexec("rm -rf "..testdata.generateddata))
    if testdata.unload then testdata.unload() end
    return 'ok'
end

--Wait for all the signals (not only one) given as parameters
--@param sigs table containing each signal to wait and to receive at least once
--@param timeout in second
--@param action the function to run that should provoke signals given in `sigs` table
--@return "ok" when all signals have received before timeout, nil+error otherwise. 
--doesnt deal with usecase where you want to want to get several time one of the signal in sigs
local function waitmultisig(sigs, timeout, action)
    local sigres={}

    --to speed up tests: if state is valid stop waiting
    local function validstate()
        local res = true
        for k, v in pairs(sigres) do
            if not v.res then res = false return end
        end
        if res then sched.signal("waitmultisig", "done") end
    end

    for k, v in pairs(sigs) do
        local em = v.em
        local ev = v.ev
        --p("waitmultisig setting up:", em, ev)
        sigres[em..ev]={}
        sigres[em..ev].hook = sched.sighook(em, ev, function(ev, args) sigres[em..ev].res=true; validstate() end)
    end

    sched.run(action)
    sched.wait("waitmultisig", "done", timeout)

    local res = true

    --cancel all hooks
    -- and compute global result
    --maybe improved: doesnot take care (use) of validstate function
    for k, v in pairs(sigres) do
        sched.kill(v.hook)
        if not v.res then res = false end
    end

    return res and "ok" or nil, "didn't get all signals in time"
end

local function runlocaltest(name)

    local t = u.newtestsuite("update_pkg_"..name)

    local test={}

    local function setup()
        u.assert(update.init())
        u.assert(config.set("update.localpkgname", "pkg_standalone_pkg.tar")) --set same value as file name (not path) as put in testdata.packagepath
        u.assert(loadtest(name, test))
    end

    local function test_test1()
        local cmd = "cp "..test.testdata.packagepath.." "..udrop
        u.assert_equal(0, os.execute(cmd))
        if "async" == test.testdata.type then
            --test failure/success is determined by signal reception
            local sigs = {}
            u.assert(test.testdata.status)
            table.insert(sigs, {em="updateresult", ev=test.testdata.status} )

            if test.testdata.signalname then
                table.insert(sigs, {em=test.testdata.signalname, ev="ok"} )
            end

            if not next(sigs) then u.abort("bad test: no condition for success/failure") end

            u.assert(waitmultisig(sigs, test.testdata.timeout, update.localupdate))
        else
            --just call
            local res =  update.localupdate(nil, true)
            u.assert( (res and test.testdata.status == "success") or (not res and test.testdata.status == "failure"), "unattended sync result")
        end

        test.res="ok"

    end

    local function teardown()
        u.assert(test, "test case was aborted during setup")
        if test.res ~="ok" then
            --TODO: find the best solution to abrutly stop and update with current state mgmt.
            --umgr.finishupdate(600, "unittest "..name.." failed so the update is interrupted now.")
            --manually stop the update to prevent retries etc that can last for long
        --so that everthing is cleaned after the teardown
        --only do it when the test fails
        end
        u.assert(unloadtest(test.testdata))
        u.assert(config.set("update.localpkgname" , nil))
    end

    t.setup=setup
    t["test_"..name]=test_test1
    t.teardown = teardown

    return "ok"
end

runlocaltest("test1_assetupdatehook")
runlocaltest("test2_installscript")
runlocaltest("test3_badlocationinpkg")
runlocaltest("test5_assetupdatehook_rm_cmp")




