#!/bin/bash

targetAddr=$1


# Checks if the logStr is in the logs.  If not then exit with an error code.
#
# params:
#       logStr      The log string to search for.
checkLogStr () {
    ssh root@$targetAddr "grep \"$1\" /var/log/messages"

    if [ $? != 0 ]
    then
        echo "'$1' was not in the logs."
        echo "Limits Test Failed!"
        exit 1
    fi
}


# Build all the apps first.
mkapp defaultLimitsTest.adef -t ar7
mkapp largeValuesTest.adef -t ar7
mkapp limitsTest.adef -t ar7
mkapp zeroFileSysSize.adef -t ar7

# Install all the apps.
instapp defaultLimitsTest.ar7 $targetAddr
instapp limitsTest.ar7 $targetAddr
instapp largeValuesTest.ar7 $targetAddr
instapp zeroFileSysSize.ar7 $targetAddr

# Stop all other apps.
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

# Clear the logs.
ssh root@$targetAddr "rm /var/log/messages"

# Run the apps.
ssh root@$targetAddr  "/usr/local/bin/app start defaultLimitsTest"
ssh root@$targetAddr  "/usr/local/bin/app start limitsTest"
ssh root@$targetAddr  "/usr/local/bin/app start largeValuesTest"
ssh root@$targetAddr  "/usr/local/bin/app start zeroFileSysSize"

# Wait for all the apps to finish running.
sleep 2

# Grep the logs to check the results.
checkLogStr "_defaultLimitsTestDefault_Init_Function() .* | ======== Passed ========"
checkLogStr "_limitsTestDefault_Init_Function() .* | ======== Passed ========"
checkLogStr "_largeValuesTestDefault_Init_Function() .* | ======== Passed ========"
checkLogStr "_zeroFileSysSizeTestDefault_Init_Function() .* | ======== Passed ========"


echo "Limits Test Passed!"
exit 0
