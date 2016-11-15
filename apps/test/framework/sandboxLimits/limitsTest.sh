#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

appList="largeValuesTest limitsTest zeroFileSysSize"

OnFail() {
    echo "Limits Test Failed!"
}

OnExit() {
    # Uninstall all the apps.
    for app in $appList; do
        app remove ${app} $targetAddr
    done
}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd $scriptDir

for app in $appList; do
    echo "Build $app"
    mkapp ${app}.adef -t $targetType
    CheckRet
done

# Install all the apps.
for app in $appList; do
    InstallApp ${app}
done

# Stop all other apps.
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

# Clear the logs.
ClearLogs

# Run the apps.
for app in $appList; do
    ssh root@$targetAddr  "$BIN_PATH/app start $app"
done

# Wait for all the apps to finish running.
sleep 2

# Grep the logs to check the results.
CheckLogStr ">" 0 ".* limitsTest.* | ======== Passed ========"
CheckLogStr ">" 0 ".* largeValuesTest.* | ======== Passed ========"
CheckLogStr ">" 0 ".* zeroFileSysSizeTest.* | ======== Passed ========"

echo "Limits Test Passed!"
exit 0
