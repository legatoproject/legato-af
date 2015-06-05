#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "Secure Storage Test Failed!"
}

function IsAppRunning
{
    appName=$1

    numMatches=$(ssh root@$targetAddr "/usr/local/bin/app status $appName | grep -c \"running\"")

    if [ $numMatches -eq 0 ]
    then
        # App is not running.
        return 1
    else
        # App is running.
        return 0
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

echo "******** Secure Storage Test Starting ***********"

echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

appDir="$LEGATO_ROOT/build/$targetType/bin/tests"
cd "$appDir"
CheckRet

echo "Install secStoreTests"
instapp secStoreTest1.$targetType $targetAddr
instapp secStoreTest2.$targetType $targetAddr

echo "Install Secure Storage Daemon in case it's removed by other tests."
instapp $LEGATO_ROOT/build/$targetType/bin/apps/secStore.$targetType $targetAddr

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""

echo "Make sure the Secure Storage Daemon is running."
ssh root@$targetAddr  "/usr/local/bin/app start secStore"
CheckRet

echo "Clear the logs."
ssh root@$targetAddr "killall syslogd"
CheckRet

# Must restart syslog this way so that it gets the proper SMACK label.
ssh root@$targetAddr "/mnt/flash/startup/fg_02_RestartSyslogd"
CheckRet

echo "Starting secStoreTest1."
ssh root@$targetAddr  "/usr/local/bin/app start secStoreTest1"
CheckRet

# Wait for app to finish
IsAppRunning "secStoreTest1"
while [ $? -eq 0 ]; do
    IsAppRunning "secStoreTest1"
done

echo "Starting secStoreTest2."
ssh root@$targetAddr  "/usr/local/bin/app start secStoreTest2"
CheckRet

echo "Re-install secStoreTest1."
ssh root@$targetAddr  "/usr/local/bin/app remove secStoreTest1"
CheckRet
instapp secStoreTest1.$targetType $targetAddr
CheckRet

echo "Starting secStoreTest1 again."
ssh root@$targetAddr  "/usr/local/bin/app start secStoreTest1"
CheckRet

# Wait for app to finish
IsAppRunning "secStoreTest1"
while [ $? -eq 0 ]; do
    IsAppRunning "secStoreTest1"
done

echo "Uninstall all apps."
ssh root@$targetAddr  "/usr/local/bin/app remove secStoreTest1"
ssh root@$targetAddr  "/usr/local/bin/app remove secStoreTest2"

echo "Grepping the logs to check the results."
CheckLogStr "==" 2 "============ SecStoreTest1 PASSED ============="
CheckLogStr "==" 1 "============ SecStoreTest2 PASSED ============="

echo "Secure Storage Test Passed!"
exit 0
