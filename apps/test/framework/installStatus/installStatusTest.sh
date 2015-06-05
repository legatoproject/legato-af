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
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

appDir="$LEGATO_ROOT/build/$targetType/bin/tests"
cd "$appDir"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

echo "Install all the installStatusTest app."
instapp installStatusTest.$targetType $targetAddr
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app start installStatusTest"
CheckRet

echo "Installing and uninstalling small test apps multiple times."
instapp testApp1.$targetType $targetAddr
CheckRet

instapp testApp2.$targetType $targetAddr
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app remove testApp1"
CheckRet

instapp testApp1.$targetType $targetAddr
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app remove testApp2"
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app remove testApp1"
CheckRet

ssh root@$targetAddr  "/usr/local/bin/app remove installStatusTest"
CheckRet

echo "Grepping the logs to check the results."
CheckLogStr "==" 2 "========== App testApp1 installed."
CheckLogStr "==" 2 "========== App testApp1 uninstalled."
CheckLogStr "==" 1 "========== App testApp2 installed."
CheckLogStr "==" 1 "========== App testApp2 uninstalled."

echo "Install Status Test Passed!"
exit 0
