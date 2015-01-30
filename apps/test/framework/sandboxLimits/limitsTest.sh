#!/bin/bash

targetAddr=$1
targetType=${2:-ar7}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Checks if the logStr is in the logs.  If not then exit with an error code.
#
# params:
#       logStr      The log string to search for.
checkLogStr () {
    ssh root@$targetAddr "/sbin/logread | grep \"$1\""

    if [ $? != 0 ]
    then
        echo "'$1' was not in the logs."
        echo "Limits Test Failed!"
        exit 1
    fi
}

function CheckRet
{
    RETVAL=$?
    if [ $RETVAL -ne 0 ]; then
        echo -e $COLOR_ERROR "Exit Code $RETVAL" $COLOR_RESET
        exit $RETVAL
    fi
}

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
checkLogStr ".* limitsTest.* | ======== Passed ========"
checkLogStr ".* largeValuesTest.* | ======== Passed ========"
checkLogStr ".* zeroFileSysSizeTest.* | ======== Passed ========"


echo "Limits Test Passed!"
exit 0
