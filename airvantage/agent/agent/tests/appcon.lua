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

-------------------------------------------------------------------------------
---- In this unit test file you will find 2 test suites:
---- - ApplicationContainer
---- - Appmon Daemon (deamon used by ApplicationContainer)
---- You will also find utils used by both test suites.
---- Usage constraints:
---- - TO BE RUN AS ROOT :(
----   basically it will be:
----     sudo ./start.sh
----     telnet localhost 2000;
----     require"tests.appcon";
----     unittest.run()
-------------------------------------------------------------------------------

local sched  = require "sched"
local u = require"unittest"
local os = require"os"
local systemutils = require"utils.system"
local log = require"log"
local tableutils = require"utils.table"
local config = require"agent.config"
local lfs = require"lfs"
local appcon = require"agent.appcon"
local math = require"math"

-------------------------------------------------------------------------------
---- Common util for both appmon and appcon test suites
---- Mainly stuff start/stop appmon daemon automatically
-------------------------------------------------------------------------------

local BIND_ERROR = 98
local last_config

--helper function to send command to the appmon_daemon through socket.
-- COPIED from appcon.lua
local function sndcmd(port, str)
    if not str then return nil , "internal error: bad request" end
    str = str.."\n"
    local skt = socket.tcp()
    skt:settimeout(20)
    local res, err = skt:connect("localhost", port)
    if not res then skt:close(); return nil, "Com with appmon_daemon: Cannot connect to appmon daemon, err="..(err or "unknown") end
    res,err = skt:send(str)
    if res~= #str then skt:close(); return nil, "Com with appmon_daemon: Cannot send command to appmon daemon, err="..(err or "unknown") end
    --each command got a response, response separator is "\n", so receive("*l")
    res, err = skt:receive("*l")
    skt:close()
    return res, err
end

local function kill_appmon()
    if last_config then sndcmd(last_config.p, "destroy")
        sched.wait(1)
    end
    os.execute("killall appmon_daemon")
    sched.wait(1)
    os.execute("killall -9 appmon_daemon")
end

--start appmon, finding correct port to listen local function start_appmon(config).
-- The port finding stuff may be not useful anymore since SO_REUSEADDR option is used
-- for AppmonDaemon socket.
local function start_appmon(config)
    log("APPMON-TEST", "DEBUG", "start_appmon port =%d", config.p)
    for i=1,10 do
        local cmd = "bin/appmon_daemon -p "..config.p
        if config.n then cmd = cmd .. " -n "..config.n end
        log("APPMON-TEST", "DEBUG", "start_appmon cmd=%s", tostring(cmd))
        local res, err= os.execute(cmd)
        if res ~= BIND_ERROR then
            u.assert_equal(0, res, string.format("cannot start appmon succesfully, config:%s, res= %s, err=%s", sprint(config), tostring(res), tostring(err)) )
            last_config={};tableutils.copy(config, last_config, true)
            break;
        end
        --else keep seaching for available port
        config.p = config.p+math.random(10)
    end
    local res, out = systemutils.pexec("ps aux | grep [b]in/appmon")
    u.assert_equal(0, res, "cannot start appmon succesfully, pexec returned error")
    u.assert_match("bin/appmon_daemon", out, "cannot start appmon succesfully, appmon is not found using ps command")
    log("APPMON-TEST", "DEBUG", "start_appmon ok with port =%d", config.p)
end




-------------------------------------------------------------------------------
---- Appmon Daemon test suite
-------------------------------------------------------------------------------
local t_appmon=u.newtestsuite("1_appmon")

function t_appmon:setup()
    --activate debug for this test file
    log.setlevel("ALL", "APPMON-TEST")
    log.setlevel("ALL", "APPCON")
    --assert we are running as root !!!!!!
    local res, out = systemutils.pexec("id")
    u.assert( (res == 0) and out , out or "cannot get id")
    local uid=tonumber(out:match("uid=(%d+)%("))
    if uid ~= 0 then u.abort("This test file need to run as root") end
    --kill any running appmon
    kill_appmon()
end




-- return file path where app has been generated or nil,error
local function create_start_app(dconfig)
    local app_content = [[
    #!/bin/sh
    while true
    do
    echo "toto"
    sleep 1
    done
    ]]
    local file_name,err =  os.tmpname();
    u.assert(file_name, err)
    log("APPMON-TEST", "DEBUG", "file_name=%s", file_name)
    local file, err = io.open(file_name, "w+")
    u.assert(file, err)
    u.assert(file:write(app_content))
    u.assert(file:close())
    local res, err = os.execute("chmod 755 "..file_name)
    u.assert_equal(0, res, "cannot chmod tmp app file")

    local res, err = sndcmd(dconfig.p, "setup /tmp "..file_name)
    u.assert(res, err)
    u.assert_equal("1", res, "setup app failed")
    res, err = sndcmd(dconfig.p, "start 1")
    u.assert(res, err)
    u.assert_equal("ok", res, "cannot start app")

    return file_name
end

local function delete_app(file_path)
    local res, err = os.execute("rm "..file_path)
    u.assert_equal(0, res, "cannot delete tmp app file")
end

local function get_process_priority(process_name)
    process_name = "["..process_name:sub(1,1).."]"..process_name:sub(2)
    log("APPMON-TEST", "DEBUG", "get_process_priority: %s", process_name)
    local res, out = systemutils.pexec("ps alx | grep "..process_name)
    u.assert( (res == 0) and out, out or "cannot find priority using ps")
    log("APPMON-TEST", "DEBUG", "get_process_priority: res=%s, out=%s", res, out)
    return tonumber(out:match("%d+%s+%d+%s+%d+%s+%d+%s+%d+%s+(%-?%d+)"));
end

local function get_current_priority()
    -- get current priority
    local res, out = systemutils.pexec("nice")
    u.assert( (res ==0) and out, out or "cannot retrieve agent priority")
    return tonumber(out:match("%d+"));
end


local function renice_daemon(renice_value)
    --get daemon pid first:
    local res, out = systemutils.pexec("ps ao pid,cmd | grep [a]ppmon_daemon")
    local dpid = out:match("(%d+)%s")
    u.assert(dpid, "cannot get daemon pid")
    -- do renice
    u.assert(0 == os.execute("renice -n "..renice_value.." -p "..dpid))
end

local function test_factory(dconfig, expected_daemon_prio, expected_app_prio, renice_value)
    start_appmon(dconfig)
    local res, err = sndcmd(dconfig.p, "list")
    u.assert(res, err)

    if renice_value then renice_daemon(renice_value) end

    local file_path,err =create_start_app(dconfig)

    -- assert daemon nice level is still the same
    u.assert_equal(expected_daemon_prio, get_process_priority("appmon_daemon"), "appmon daemon process priority is incorrect")
    -- assert app nice level is correct regarding daemon config
    u.assert_equal(expected_app_prio, get_process_priority(file_path), "app process priority is incorrect")
    res, err = sndcmd(dconfig.p, "destroy")
    u.assert(res, err)
    kill_appmon()
    delete_app(file_path)
end



-- tests that :
-- - the daemon keeps it process priority when setting the apps one
-- - application are started with correct priority
--  (in this case the priority is correct vs user rights etc)
function t_appmon:test_01()
    local dconfig = {p=7410, n=5} --appmon daemon config

    -- get current priority
    local res, out = systemutils.pexec("nice")
    u.assert( (res == 0) and out, out or "cannot retrieve agent priority")
    local current_priority = tonumber(out:match("%d+"));

    test_factory(dconfig, current_priority, dconfig.n)
end

-- tests that :
-- - the daemon keeps it process priority when setting the apps one
-- - application are started with same priority that the daemon when priority input is not valid
function t_appmon:test_02()
    local dconfig = {p=7410, n=-5} --appmon daemon config

    -- get current priority
    local current_priority = get_current_priority()

    test_factory(dconfig, current_priority, current_priority)
end

-- tests that :
-- - the daemon keeps it process priority when setting the apps one
-- - application are started with same priority that the daemon when priority is not specified
function t_appmon:test_03()
    local dconfig = {p=7410} --appmon daemon config

    -- get current priority
    local current_priority = get_current_priority()

    test_factory(dconfig, current_priority, current_priority)
end

-- tests that :
-- - the daemon keeps it process priority when setting the apps one
-- - application are started with correct priority
--  (in this case the priority is correct vs user rights etc)
-- even if app priority is set to negative value
function t_appmon:test_04()
    local dconfig = {p=7410, n=-5} --appmon daemon config

    test_factory(dconfig, -8, dconfig.n, -8)

end



function t_appmon:teardown()
    kill_appmon()
    last_config=nil
    log.setlevel("INFO", "APPMON-TEST")
    log.setlevel("INFO", "APPCON")
end





-------------------------------------------------------------------------------
---- ApplicationContainer test suite
-------------------------------------------------------------------------------

local t_appcon=u.newtestsuite("2_appcon")



local appcon_config={}
local appmon_config = { p=8888 }

function t_appcon:setup()

    --save persistore
    os.execute("cp -Rp persist persist_saved_by_appcon_unittest")

    log.setlevel("ALL", "APPMON-TEST")
    log.setlevel("ALL", "APPCON")
    assert(config.set('appcon.activate',false))
    u.assert_false(config.appcon.activate, "appcon must not be activated before running the test suite")
    -- kill any running appmon
    kill_appmon()
    --start appmon
    start_appmon(appmon_config)

    log("APPMON-TEST", "appmon_config %s", sprint(appmon_config))
    log("APPMON-TEST", "last_config %s", sprint(appmon_config))

    --start appcon with config according real appmon config
    -- first save initial appcon config
    tableutils.copy(config.appcon, appcon_config)

    config.set("appcon.port", last_config.p)

    u.assert_equal("ok", appcon.init(), "cannot init Application Container")

    --remove any app in appcon!!
    local list = appcon.list()
    for id in pairs(list) do
        u.assert(appcon.uninstall(id))
    end
end



local tmpfile_content1 = "foobarfoobarfoobarfoobarfoobarfoobarfoobar"
local runfile_content1 = [[#!/bin/sh
echo 'app from appcon testsuite'
]]

--creates data in some temp folder with app to install
--the app can be runnable or not, depending on 'runnable' param
local function create_app(runnable)
    local tmpfolder = os.tmpname()
    os.execute("rm -rf "..tmpfolder)
    tmpfolder = tmpfolder..'_dir'
    u.assert(tmpfolder, "cannot create tmp folder to put app")
    local res, err = os.execute("mkdir "..tmpfolder)
    u.assert(res == 0, err or "unknown error")
    local file, err = io.open(tmpfolder.."/file1", "w+")
    u.assert(file, err)
    u.assert(file:write(tmpfile_content1))
    if runnable and runnable=="runnable" then
        file, err = io.open(tmpfolder.."/run", "w+")
        u.assert(file, err)
        u.assert(file:write(runfile_content1))
    end
    return tmpfolder
end

--checks application parameter are correct: access rights, user etc.
local function post_install_tests(id)
    local res, err = systemutils.pexec("whoami")
    u.assert_equal(0, res, "Cannot get current user")
    local user=err:match("(.*)[%c*]$")
    local access_rights = "rwxr%-xr%-x"
    res, err = systemutils.pexec("ls -lRa apps/"..id)
    u.assert(res, err)
    local output = err
    for line in output:gmatch("[^\r\n]+") do
        local res,err =  line:match("^[dlspcb%-].*")
        if line:match("^[dlspcb%-].*") then
            local res, err = line:match("^."..access_rights.."%s+%d+%s+"..user.."%s+"..user..".*")
            u:assert(res, string.format("Bad access rights or owner on file(%s) in app folder", line))
        end
    end

end


function t_appcon:test_01_install_not_runnable()
    local id= "unit_test_app_1"
    local res,err
    --create non runnable app
    local path = create_app("not runnable")
    res,err = appcon.install(id, path)
    u.assert_equal("ok", res, err)
    post_install_tests(id)
    --clean tmp files created for install
    u.assert(0 == os.execute("rm -rf "..path))
    --check app status
    u.assert_equal("Not runnable", appcon.status(id))
    --check start API
    res, err = appcon.start(id)
    u.assert_nil(res)
    u.assert_equal("Not runnable", err)
    --check stop API
    res, err = appcon.stop(id)
    u.assert_nil(res)
    u.assert_equal("Not runnable", err)
    --check uninstall API
    res, err = appcon.uninstall(id)
    u.assert_equal("ok", res)
    u.assert_nil(err)
    --check uninstall removed all files
    res = os.execute("ls apps/"..id)
    u.assert_not_equal(0, res, "uninstall should have removed app folder")
end

function t_appcon:test_02_install_runnable()
    local id= "unit_test_app_2"
    local res,err
    --create runnable app
    local path = create_app("runnable")
    res,err = appcon.install(id, path)
    u.assert_equal("ok", res, err)
    post_install_tests(id)
    --clean tmp files created for install
    u.assert_equal(0, os.execute("rm -rf "..path))
    --check app status
    u.assert_not_equal("Not runnable", appcon.status(id))
    u.assert_match("^prog=%[.*", appcon.status(id), "Cannot get acceptable status from appmon")
    --check start API
    res, err = appcon.start(id)
    u.assert(res and not err, "start on runnable app must succes")
    --check stop API
    res, err = appcon.stop(id)
    u.assert(res and not err, "stop on runnable app must succes")
    --check uninstall API
    res, err = appcon.uninstall(id)
    u.assert_equal("ok", res)
    u.assert_nil(err)
    --check uninstall removed all files
    res = os.execute("ls apps/"..id)
    u.assert_not_equal(0, res, "uninstall should have removed app folder")
end

--no purge spec:
--file that exists in current app folder but not in new folder given as install source is kept
--file that exists in new folder given as install source but not in current app folder is copied
--file that exists in both folder: new file overwrites old one.
function t_appcon:test_03_reinstall_no_purge()
    local id= "unit_test_app_3"
    local res,err
    --create runnable app
    local path = create_app("runnable")
    res,err = appcon.install(id, path)
    u.assert_equal("ok", res, err)
    post_install_tests(id)
    --do not clean tmp files created for install (will be used for reinstall)

    --check app status
    u.assert_not_equal("Not runnable", appcon.status(id))
    u.assert_match("^prog=%[.*", appcon.status(id), "Cannot get acceptable status from appmon")
    --check start API
    res, err = appcon.start(id)
    u.assert(res and not err, "start on runnable app must succes")
    --check stop API
    res, err = appcon.stop(id)
    u.assert(res and not err, "stop on runnable app must succes")

    -- now add new file in app folder (#1)
    res, err = os.execute("touch apps/"..id.."/unit_test_app_3_file_1")
    u.assert_equal( 0, res, err or "cannot add new file in app folder")
    -- add new file in install source folder (#2)
    res, err = os.execute("touch "..path.."/unit_test_app_3_file_2")
    u.assert_equal( 0, res, err or "cannot add new file in install folder")
    -- modify the common file in install source folder (#3)
    res, err = os.execute("echo blabla >"..path.."/file1")
    u.assert_equal( 0, res, err or "cannot modify file in install folder")

    -- now add a folder in app folder with the same name than file #2  (#4)
    res, err = os.execute("mkdir apps/"..id.."/unit_test_app_3_file_2")
    u.assert_equal( 0, res, err or "cannot add new folder in app folder")
    -- add new file in app folder (#5)
    res, err = os.execute("touch apps/"..id.."/unit_test_app_3_file_3")
    u.assert_equal( 0, res, err or "cannot add new file in app folder")
    -- now add new folder in  install folder (#6)
    res, err = os.execute("mkdir "..path.."/unit_test_app_3_file_3")
    u.assert_equal( 0, res, err or "cannot add new file in app folder")

    -- reinstall app
    res,err = appcon.install(id, path)
    u.assert_equal("ok", res, err)
    post_install_tests(id)

    -- check file #1 is here
    res, err = os.execute("ls apps/"..id.."/unit_test_app_3_file_1")
    u.assert_equal( 0, res, err or "new file #1 absent from app folder")
    -- check file #2 is here
    res, err = os.execute("ls apps/"..id.."/unit_test_app_3_file_2")
    u.assert_equal( 0, res, err or "new file #2 absent from app folder")
    -- check file #3 is modified
    local file,err=io.open("apps/"..id.."/file1", "r")
    u.assert(file, err)
    local content, err = file:read("*a")
    u.assert_equal("blabla\n", content)
    u.assert(file:close())
    -- #4 was overwritten by #2 check it is a file
    res,err = lfs.attributes("apps/"..id.."/unit_test_app_3_file_2", "mode")
    u.assert_equal("file", res, string.format("file #2/#4 should be a file and not a %s", res))
    -- #5 was overwritten by #6 check it is a folder
    res,err = lfs.attributes("apps/"..id.."/unit_test_app_3_file_3", "mode")
    u.assert_equal("directory", res, string.format("file #5/#6 should be a directory and not a %s", res))

    --uninstall and clean
    res, err = appcon.uninstall(id)
    u.assert_equal("ok", res)
    u.assert_nil(err)
    --check uninstall removed all files
    res = os.execute("ls apps/"..id)
    u.assert_not_equal(0, res, "uninstall should have removed app folder")
    --finally clean files used for install and reinstall
    u.assert_equal(0, os.execute("rm -rf "..path))
end


function t_appcon:test_04_reinstall_purge()
    local id= "unit_test_app_4"
    local res,err
    --create runnable app
    local path = create_app("runnable")
    res,err = appcon.install(id, path)
    u.assert_equal("ok", res, err)
    post_install_tests(id)
    --do not clean tmp files created for install (will be used for reinstall)

    --check app status
    u.assert_not_equal("Not runnable", appcon.status(id))
    u.assert_match("^prog=%[.*", appcon.status(id), "Cannot get acceptable status from appmon")
    --check start API
    res, err = appcon.start(id)
    u.assert(res and not err, "start on runnable app must succes")
    --check stop API
    res, err = appcon.stop(id)
    u.assert(res and not err, "stop on runnable app must succes")

    -- now add new file in app folder (#1)
    res, err = os.execute("touch apps/"..id.."/unit_test_app_3_file_1")
    u.assert_equal( 0, res, err or "cannot add new file in app folder")
    -- add new file in install source folder (#2)
    res, err = os.execute("touch "..path.."/unit_test_app_3_file_2")
    u.assert_equal( 0, res, err or "cannot add new file in install folder")
    -- modify the common file in install source folder (#3)
    res, err = os.execute("echo blabla >"..path.."/file1")
    u.assert_equal( 0, res, err or "cannot modify file in install folder")
    -- reinstall app with purge param
    res,err = appcon.install(id, path, false, "purge")
    u.assert_equal("ok", res, err)
    post_install_tests(id)
    -- check file #1 is not here
    res, err = os.execute("ls apps/"..id.."/unit_test_app_3_file_1")
    u.assert_not_equal( 0, res, "new file #1 should be absent from app folder")
    -- check file #2 is here
    res, err = os.execute("ls apps/"..id.."/unit_test_app_3_file_2")
    u.assert_equal( 0, res, err or "new file #2 absent from app folder")
    -- check file #3 is modified
    local file,err=io.open("apps/"..id.."/file1", "r")
    u.assert(file, err)
    local content, err = file:read("*a")
    u.assert_equal("blabla\n", content)
    u.assert(file:close())

    --uninstall and clean
    res, err = appcon.uninstall(id)
    u.assert_equal("ok", res)
    u.assert_nil(err)
    --check uninstall removed all files
    res = os.execute("ls apps/"..id)
    u.assert_not_equal(0, res, "uninstall should have removed app folder")
    --finally clean files used for install and reinstall
    u.assert_equal(0, os.execute("rm -rf "..path))
end


function t_appcon:teardown()
    local list = appcon.list()
    for id in pairs(list) do
        u.assert(appcon.uninstall(id))
    end
    --clean appmon config to enable test suite replay
    last_config=nil
    -- revert configuration
    config.set("appcon", appcon_config)

    kill_appmon()
    log.setlevel("INFO", "APPMON-TEST")
    log.setlevel("INFO", "APPCON")

    --restore persist
    os.execute("rm -rf persist")
    os.execute("mv persist_saved_by_appcon_unittest persist")
end


