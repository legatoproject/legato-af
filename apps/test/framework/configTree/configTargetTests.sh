#!/bin/bash

#---------------------------------------------------------------------------------------------------
#  Bash script for running the configTree ACL tests on target.
#
#  Copyright (C) Sierra Wireless Inc.
#
#---------------------------------------------------------------------------------------------------

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

SetTargetIP "$targetAddr"

appList="cfgSelfRead cfgSelfWrite cfgSystemRead cfgSystemWrite"

OnFail() {
    echo "ConfigTree Test Failed!"
}

OnExit() {
    # best effort - clean up
    echo "##########  Removing the apps."
    for app in $appList
    do
        app remove $app $targetAddr
    done

    ssh root@$targetAddr "$BIN_PATH/legato restart"
}

#---------------------------------------------------------------------------------------------------
#  Run our test apps on the device.
#---------------------------------------------------------------------------------------------------
function RunAclApps
{
    echo "##########  Stopping all other apps on device '$targetAddr'."
    for app in $(app list $targetAddr); do
        ssh root@$targetAddr "$BIN_PATH/app stop '$app'"
        if IsAppRunning $app; then
            echo "Unable to stop app $app"
            exit 1
        fi
    done

    ClearLogs

    sleep 1

    echo "##########  Run all the apps."
    for app in $appList
    do
        ssh root@$targetAddr "$BIN_PATH/app start $app"
    done
}

#---------------------------------------------------------------------------------------------------
#  Exec the config tool on the device to set a string in a config tree.
#---------------------------------------------------------------------------------------------------
function rconfigset
{
    ssh root@$targetAddr "$BIN_PATH/config set $1 $2"
}

#---------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------
#  Test case to test access control.
#---------------------------------------------------------------------------------------------------
TestAcl() {
    echo "*****************  configTree ACL Tests started.  *****************"

    echo "##########  Make sure Legato is running on device '$targetAddr'."
    ssh root@$targetAddr "$BIN_PATH/legato start"
    CheckRet

    echo "##########  Install all the apps to device '$targetAddr'."
    appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
    cd "$appDir"
    CheckRet
    for app in $appList
    do
        InstallApp ${app}
    done

    echo "##########  Listing apps on device '$targetAddr'."
    app list $targetAddr

    RunAclApps

    echo "##########  Checking log results."
    CheckLogStr "==" 1 "=====  Read ACL Test on tree: cfgSelfRead, successful.  ====="
    CheckLogStr "==" 0 "=====  Write ACL Test on tree: cfgSelfWrite, successful.  ====="
    CheckLogStr "==" 0 "=====  Read ACL Test on tree: system, successful.  ====="
    CheckLogStr "==" 0 "=====  Write ACL Test on tree: system, successful.  ====="

    echo "##########  Updating app permissions."
    rconfigset "/apps/cfgSelfWrite/configLimits/acl/cfgSelfWrite" "write"
    rconfigset "/apps/cfgSystemRead/configLimits/allAccess" "read"
    rconfigset "/apps/cfgSystemWrite/configLimits/allAccess" "write"

    RunAclApps

    echo "##########  Checking log results."
    CheckLogStr "==" 1 "=====  Read ACL Test on tree: cfgSelfRead, successful.  ====="
    CheckLogStr "==" 1 "=====  Write ACL Test on tree: cfgSelfWrite, successful.  ====="
    CheckLogStr "==" 1 "=====  Read ACL Test on tree: system, successful.  ====="
    CheckLogStr "==" 1 "=====  Write ACL Test on tree: system, successful.  ====="
}

CheckExportedTree() {
    local path=$1

    ssh root@$targetAddr "grep -q myvalue $path"
    CheckRet
}

#---------------------------------------------------------------------------------------------------
#  Test case to test 'config import'/'config export'.
#---------------------------------------------------------------------------------------------------
TestConfigToolImportExport() {
    echo "*****************  configTree import/export Tests started.  *****************"

    # Create/clear empty tree
    ssh root@$targetAddr "$BIN_PATH/config clear testcfg:/"
    CheckRet

    # Add an entry
    ssh root@$targetAddr "$BIN_PATH/config set testcfg:/mykey myvalue"
    CheckRet

    # export with absolute path
    ssh root@$targetAddr "rm -rf /tmp/testcfgabs && $BIN_PATH/config export testcfg:/ /tmp/testcfgabs"
    CheckRet
    CheckExportedTree "/tmp/testcfgabs"

    # export with relative path
    ssh root@$targetAddr "cd /tmp && rm -rf testcfgrel && $BIN_PATH/config export testcfg:/ /tmp/testcfgrel"
    CheckRet
    CheckExportedTree "/tmp/testcfgrel"

    # export with symlink path
    ssh root@$targetAddr "cd /tmp && rm -rf testcfgsym* && mkdir testcfgsym && ln -s testcfgsym testcfgsym2 && $BIN_PATH/config export testcfg:/ testcfgsym2/test"
    CheckRet
    CheckExportedTree "/tmp/testcfgsym/test"

    # Override entry
    ssh root@$targetAddr "$BIN_PATH/config set testcfg:/mykey newvalue"
    CheckRet

    # import
    ssh root@$targetAddr "$BIN_PATH/config import testcfg:/ /tmp/testcfgabs"
    CheckRet
    ssh root@$targetAddr "test \"\$($BIN_PATH/config get testcfg:/mykey)\" = 'myvalue'"
    CheckRet

    # Remove
    ssh root@$targetAddr "$BIN_PATH/config rmtree testcfg"
    CheckRet

    echo "TestConfigToolImportExport passed"
}

#---------------------------------------------------------------------------------------------------
#  Test case to test 'config import'/'config export' with JSON format.
#---------------------------------------------------------------------------------------------------
TestConfigToolImportExportJSON() {
    echo "*****************  configTree import/export JSON Tests started.  *****************"

    # Create/clear empty tree
    ssh root@$targetAddr "$BIN_PATH/config clear testcfg"
    CheckRet

    # Add entries
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey1 myvalue"
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey2 myvalue string"
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey3 true bool"
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey4 false bool"
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey5 1025 int"
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey6 3.1415 float"
    CheckRet

    # export as JSON
    ssh root@$targetAddr "rm -rf /tmp/testcfgjson && $BIN_PATH/config export testcfg /tmp/testcfgjson --format=json"
    CheckRet
    CheckExportedTree "/tmp/testcfgjson"

    # Override entry
    ssh root@$targetAddr "$BIN_PATH/config set testcfg/mykey1 newvalue"
    CheckRet

    # import from JSON
    ssh root@$targetAddr "$BIN_PATH/config import testcfg /tmp/testcfgjson --format=json"
    CheckRet

    ssh root@$targetAddr "test \"\$($BIN_PATH/config get testcfg/mykey1)\" = 'myvalue'"
    CheckRet

    # Remove
    ssh root@$targetAddr "$BIN_PATH/config delete testcfg"
    CheckRet

    echo "TestConfigToolImportExportJSON passed"
}

TestAcl
TestConfigToolImportExport
TestConfigToolImportExportJSON

