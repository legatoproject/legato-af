#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "Limits Test Failed!"
}

OnExit() {
# Uninstall all the apps.
    rmapp limitsTest.$targetType $targetAddr
    rmapp largeValuesTest.$targetType $targetAddr
    rmapp zeroFileSysSize.$targetType $targetAddr
}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd $scriptDir

# Build all the apps first.
mkapp largeValuesTest.adef -t $targetType
CheckRet
mkapp limitsTest.adef -t $targetType
CheckRet
mkapp zeroFileSysSize.adef -t $targetType
CheckRet

# Install all the apps.
instapp limitsTest.$targetType $targetAddr
CheckRet
instapp largeValuesTest.$targetType $targetAddr
CheckRet
instapp zeroFileSysSize.$targetType $targetAddr
CheckRet

# Stop all other apps.
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

# Clear the logs.
ssh root@$targetAddr "killall syslogd"
# Must restart syslog this way so that it gets the proper SMACK label.
ssh root@$targetAddr "/mnt/flash/startup/fg_02_RestartSyslogd"

# Run the apps.
ssh root@$targetAddr  "/usr/local/bin/app start limitsTest"
ssh root@$targetAddr  "/usr/local/bin/app start largeValuesTest"
ssh root@$targetAddr  "/usr/local/bin/app start zeroFileSysSize"

# Wait for all the apps to finish running.
sleep 2

# Grep the logs to check the results.
CheckLogStr ">" 0 ".* limitsTest.* | ======== Passed ========"
CheckLogStr ">" 0 ".* largeValuesTest.* | ======== Passed ========"
CheckLogStr ">" 0 ".* zeroFileSysSizeTest.* | ======== Passed ========"


echo "Limits Test Passed!"
exit 0
