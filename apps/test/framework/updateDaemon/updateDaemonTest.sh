#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "UpdateDaemon Test Failed!"
}

# List of apps
appsList="updateFaultApp updateRestartApp updateStopApp updateNonSandboxedFaultApp updateNonSandboxedRestartApp updateNonSandboxedStopApp"

echo "******** UpdateDaemon Test Starting ***********"

echo "Pack all the apps."
appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
cd "$appDir"
CheckRet
for app in $appsList
do
    echo "  Encrypting '$appDir/$app.$targetType.update.sec'"
    security-pack ${app}.${targetType}.update
    CheckRet
done

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

echo "Install all the apps."
for app in $appsList
do
    echo "  Installing '$appDir/${app}.${targetType}.update.sec'"
    cat ${app}.${targetType}.update.sec| ssh root@$targetAddr "$BIN_PATH/update"
    CheckRet
done

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

ClearLogs

echo "Run the apps."
for app in $appsList
do
    ssh root@$targetAddr  "$BIN_PATH/app start $app"
    CheckRet
done

# Wait for all the apps to finish running.
sleep 3

echo "Creating package for removing all apps"
update-pack $targetType -ar  '*' -o removeAll.uinst
CheckRet
echo "  Encrypting 'removeAll.uinst'"
security-pack removeAll.uinst
CheckRet
echo "Uninstall all apps."
cat removeAll.uinst.sec | ssh root@$targetAddr  "$BIN_PATH/update"
CheckRet

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

echo "UpdateDaemon Test Passed!"
exit 0

