#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "Supervisor Test Failed!"
}

# List of apps
appsList="FaultApp RestartApp StopApp NonSandboxedFaultApp NonSandboxedRestartApp NonSandboxedStopApp"

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

echo "******** Supervisor Test Starting ***********"

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

echo "Install all the apps."
appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
cd "$appDir"
CheckRet
for app in $appsList
do
    InstallApp ${app}
done

InstallApp ForkChildApp
InstallApp NonSandboxedForkChildApp

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

echo "Testing handling of forked child in sandbox apps."

ssh root@$targetAddr  "$BIN_PATH/app start ForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app stop ForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app start ForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app stop ForkChildApp"
CheckRet

echo "Testing handling of forked child in nonsandbox apps."

ssh root@$targetAddr  "$BIN_PATH/app start NonSandboxedForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app stop NonSandboxedForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app start NonSandboxedForkChildApp"
CheckRet

ssh root@$targetAddr  "$BIN_PATH/app stop NonSandboxedForkChildApp"
CheckRet

ClearLogs

echo "Run the apps."
for app in $appsList
do
    ssh root@$targetAddr  "$BIN_PATH/app start $app"
    CheckRet
done

# Wait for all the apps to finish running.
sleep 3

echo "Uninstall all apps."
ssh root@$targetAddr  "$BIN_PATH/app stop \"*\""

echo "Grepping the logs to check the results."
CheckLogStr "==" 1 "======== Start 'FaultApp/noExit' Test ========"
CheckLogStr "==" 1 "======== Start 'FaultApp/noFault' Test ========"
CheckLogStr "==" 1 "======== Test 'FaultApp/noFault' Ended Normally ========"
CheckLogStr ">" 1 "======== Start 'FaultApp/progFault' Test ========"
CheckLogStr ">" 1 "======== Start 'FaultApp/sigFault' Test ========"
CheckLogStr ">" 1 "======== Start 'RestartApp/noExit' Test ========"
CheckLogStr "==" 1 "======== Start 'StopApp/noExit' Test ========"

CheckLogStr "==" 1 "======== Start 'NonSandboxedFaultApp/noExit' Test ========"
CheckLogStr "==" 1 "======== Start 'NonSandboxedFaultApp/noFault' Test ========"
CheckLogStr "==" 1 "======== Test 'NonSandboxedFaultApp/noFault' Ended Normally ========"
CheckLogStr ">" 1 "======== Start 'NonSandboxedFaultApp/progFault' Test ========"
CheckLogStr ">" 1 "======== Start 'NonSandboxedFaultApp/sigFault' Test ========"
CheckLogStr ">" 1 "======== Start 'NonSandboxedRestartApp/noExit' Test ========"
CheckLogStr "==" 1 "======== Start 'NonSandboxedStopApp/noExit' Test ========"

echo "Supervisor Test Passed!"
exit 0
