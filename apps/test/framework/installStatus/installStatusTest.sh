#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "Install Status Test Failed!"
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

echo "******** Install Status Test Starting ***********"

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
cd "$appDir"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

ClearLogs

echo "Install all the installStatusTest app."
InstallApp installStatusTest

ssh root@$targetAddr  "$BIN_PATH/app start installStatusTest"
CheckRet

echo "Installing and uninstalling small test apps multiple times."
InstallApp testApp1
InstallApp testApp2

ssh root@$targetAddr  "$BIN_PATH/app remove testApp1"
CheckRet

InstallApp testApp1

ssh root@$targetAddr  "$BIN_PATH/app remove testApp2"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app remove testApp1"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app remove installStatusTest"
CheckRet

echo "Grepping the logs to check the results."
CheckLogStr "==" 2 "========== App testApp1 installed."
CheckLogStr "==" 2 "========== App testApp1 uninstalled."
CheckLogStr "==" 1 "========== App testApp2 installed."
CheckLogStr "==" 1 "========== App testApp2 uninstalled."

echo "Install Status Test Passed!"
exit 0

