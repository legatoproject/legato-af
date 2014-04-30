
--- Unittest suite for uninstallcmp API testing
-- Some tests (timeout) can take some time, so the test suite
-- won't be run oncommit.

local sched = require"sched"
require"print"
local u = require"unittest"
local config = require"agent.config"
local log  = require"log"

local update = require "agent.update"
local ucommon = require"agent.update.common"
local umgr = require"agent.update.updatemgr"
local upkg = require "agent.update.pkgcheck"


local tu = u.newtestsuite("update_uninstallcmp")

local initswlistuninstall

function tu:setup()
    initswlistuninstall = ucommon.data.swlist
end

function tu:teardown()
    ucommon.data.swlist = initswlistuninstall
end

function tu:test1_invalidparam()
  
    u.assert_error_match("string expected, got nil", update.uninstallcmp)

    local res, err = update.uninstallcmp("")
    u.assert_nil(res)
    u.assert_match("BAD_PARAMETER", err)
end

function tu:test2_invalidswstatus()
    --`platform` component exists in default swlist
    -- but no asset exists to execute update request
    local res, err = update.uninstallcmp("platform")
    u.assert_nil(res)
    u.assert_match("SENDING_UPDATE_TO_ASSET_FAILED", err)
    
    -- Basic dep check
    res,err = update.uninstallcmp("@sys")
    u.assert_nil(res)
    u.assert_match("DEPENDENCY_ERROR", err)
    -- "Real" dep check
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
    --can't remove cmp1 providing feat1 needed by cmp2.
    res,err = update.uninstallcmp("cmp1")
    u.assert_nil(res)
    u.assert_match("DEPENDENCY_ERROR", err)
end

function tu:test3_uninstallinprogress()

    --start update
    --this update must take some time to execute
    --
        ucommon.data.swlist = {

        components={
            {
                name = "testupdateuninstallcmp",
                provides = { feat1="1.0"},
                version = "0.1"
            }
        }
    }
    
    local racon =  require"racon"
    u.assert(racon.init())
    local asset, err = racon.newasset("testupdateuninstallcmp")
    u.assert(asset, err)
    
    local function updatehook()
        sched.wait(20)
        return 200
    end
    u.assert(asset:setupdatehook(updatehook))
    u.assert(asset:start())
    
    --Send update request to asset 
    --Need to start request asynchronously as the asset is meant to take some time 
    --to answer to the update request.
    --Also the first uninstall request must succeed
    local function rununinstall()
        local res,err = update.uninstallcmp("testupdateuninstallcmp")
        u.assert(res, err)
    end
    
    sched.run(rununinstall)
    sched.wait(1) -- give a chance to `rununinstall` to run
    
    local res, err = update.uninstallcmp("test")
    u.assert_nil(res)
    u.assert_match("OTHER_UPDATE_IN_PROGRESS", err)
    
    u.assert(asset:close())
    
    update.getstatus("sync")
end

function tu:test4_assettimeout()
    
    ucommon.data.swlist = {

        components={
            {
                name = "testupdateuninstallcmp",
                provides = { feat1="1.0"},
                version = "0.1"
            }
        }
    }
    
    local racon =  require"racon"
    u.assert(racon.init())
    local asset, err = racon.newasset("testupdateuninstallcmp")
    u.assert(asset, err)
    
    local function updatehook()
        return "async" --no auto update result, so that wa can test timeout
    end
    u.assert(asset:setupdatehook(updatehook))
    u.assert(asset:start())
    
    --can't remove cmp1 providing feat1 needed by cmp2.
    local res,err = update.uninstallcmp("testupdateuninstallcmp")
    u.assert_nil(res)
    u.assert_match("timeout", err)
    u.assert_match("SENDING_UPDATE_TO_ASSET_FAILED", err)
    
    u.assert(asset:close())
end

function tu:test5_asseterror()
    
    ucommon.data.swlist = {

        components={
            {
                name = "testupdateuninstallcmp",
                provides = { feat1="1.0"},
                version = "0.1"
            }
        }
    }
    
    local racon =  require"racon"
    u.assert(racon.init())
    local asset, err = racon.newasset("testupdateuninstallcmp")
    u.assert(asset, err)
    
    local function updatehook()
        return 499 --PROJECT_DEPENDENT_ERROR code
    end
    u.assert(asset:setupdatehook(updatehook))
    u.assert(asset:start())
    
    --can't remove cmp1 providing feat1 needed by cmp2.
    local res,err = update.uninstallcmp("testupdateuninstallcmp")
    u.assert_nil(res)
    u.assert_match("PROJECT_DEPENDENT_ERROR", err)
    
    u.assert(asset:close())
end

--simple success
function tu:test6_success()

    ucommon.data.swlist = {

        components={
            {
                name = "testupdateuninstallcmp2.toto",
                provides = { feat1="1.0"},
                version = "0.1"
            }
        }
    }
    
    local racon =  require"racon"
    u.assert(racon.init())
    local asset, err = racon.newasset("testupdateuninstallcmp2")
    u.assert(asset, err)
    
    local function updatehook(name, version, path, parameters)
        assert(name == "toto")
        assert(not version)
        assert(not path)
        assert(not parameters)
        return 200
    end
    u.assert(asset:setupdatehook(updatehook))
    u.assert(asset:start())

    local res,err = update.uninstallcmp("testupdateuninstallcmp2.toto")
    u.assert_equal("ok", res, err)
    
    --assert removed component is not in software list anymore
    --We started the test with only one component, the list should be empty now.
    u.assert_nil(next(ucommon.data.swlist.components), "Components list not empty after uninstall")
    
    u.assert(asset:close())
end



