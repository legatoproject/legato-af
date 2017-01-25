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
}

#---------------------------------------------------------------------------------------------------
#  Run our test apps on the device.
#---------------------------------------------------------------------------------------------------
function runApps
{
    echo "##########  Stopping all other apps on device '$targetAddr'."
    ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
    sleep 1

    ClearLogs

    echo "##########  Run all the apps."
    for app in $appList
    do
        ssh root@$targetAddr "$BIN_PATH/app start $app"
        CheckRet
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


runApps


echo "##########  Checking log results."
CheckLogStr "==" 1 "=====  Read ACL Test on tree: cfgSelfRead, successful.  ====="
CheckLogStr "==" 0 "=====  Write ACL Test on tree: cfgSelfWrite, successful.  ====="
CheckLogStr "==" 0 "=====  Read ACL Test on tree: system, successful.  ====="
CheckLogStr "==" 0 "=====  Write ACL Test on tree: system, successful.  ====="



echo "##########  Updating app permissions."
rconfigset "/apps/cfgSelfWrite/configLimits/acl/cfgSelfWrite" "write"
rconfigset "/apps/cfgSystemRead/configLimits/allAccess" "read"
rconfigset "/apps/cfgSystemWrite/configLimits/allAccess" "write"



runApps


echo "##########  Checking log results."
CheckLogStr "==" 1 "=====  Read ACL Test on tree: cfgSelfRead, successful.  ====="
CheckLogStr "==" 1 "=====  Write ACL Test on tree: cfgSelfWrite, successful.  ====="
CheckLogStr "==" 1 "=====  Read ACL Test on tree: system, successful.  ====="
CheckLogStr "==" 1 "=====  Write ACL Test on tree: system, successful.  ====="



