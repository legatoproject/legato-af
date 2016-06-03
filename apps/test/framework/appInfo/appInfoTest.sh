#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "App Info Test Failed!"
}

function CheckAppIsRunning
{
    appName=$1

    numMatches=$(ssh root@$targetAddr "$BIN_PATH/app status $appName | grep -c \"running\"")

    if [ $numMatches -eq 0 ]
    then
        echo -e $COLOR_ERROR "App $appName is not running." $COLOR_RESET
        exit 1
    fi
}

if [ "$LEGATO_ROOT" == "" ]
then
    if [ "$WORKSPACE" == "" ]
    then
        echo "Neither LEGATO_ROOT nor WORKSPACE are defined." >&2
        exit 1
    else
        LEGATO_ROOT="$WORKSPACE"
    fi
fi

echo "******** App Info Test Starting ***********"

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
cd "$appDir"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

echo "Install the testAppInfo app."
InstallApp testAppInfo
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app start testAppInfo"
CheckRet

sleep 1

CheckAppIsRunning testAppInfo

ssh root@$targetAddr  "$BIN_PATH/app remove testAppInfo"
CheckRet

echo "App Info Test Passed!"
exit 0
