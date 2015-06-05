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
appDir="$LEGATO_ROOT/build/$targetType/bin/tests"
cd "$appDir"
CheckRet
for app in $appsList
do
    echo "  Packing '$appDir/$app.$targetType'"
    update-pack $targetType -ai $app.$targetType -o $app.update
    echo "  Encrypting '$appDir/$app.$targetType.sec'"
    security-pack $app.update
    CheckRet
done

echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

echo "Install all the apps."
for app in $appsList
do
    echo "  Installing '$appDir/$app.update.sec'"
    cat $app.update.sec| ssh root@$targetAddr "/usr/local/bin/update"
    CheckRet
done

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

echo "Clear the logs."
ssh root@$targetAddr "killall syslogd"
CheckRet

# Must restart syslog this way so that it gets the proper SMACK label.
ssh root@$targetAddr "/mnt/flash/startup/fg_02_RestartSyslogd"

echo "Run the apps."
for app in $appsList
do
    ssh root@$targetAddr  "/usr/local/bin/app start $app"
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
cat removeAll.uinst.sec | ssh root@$targetAddr  "/usr/local/bin/update"
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

