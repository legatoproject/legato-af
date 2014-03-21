#!/bin/bash

targetAddr=$1


# Checks if the logStr is in the logs.
#
# params:
#       comparison      Either "==", ">", "<", "!="
#       numMatches      The number of expected matches.
#       logStr          The log string to search for.
#
checkLogStr () {
    numMatches=$(ssh root@$targetAddr "/sbin/logread | grep -c \"$3\"")

    echo "$numMatches matches for '$3' "

    if [ ! $numMatches $1 $2 ]
    then
        echo "$numMatches occurances of '$3' but $1 $2 expected"
        echo "Supervisor Test Failed!"
        exit 1
    fi
}

# List of apps
appsList="FaultApp RestartApp StopApp NonSandboxedFaultApp NonSandboxedRestartApp NonSandboxedStopApp"

# Build all the apps.
for app in $appsList
do
    mkapp $app.adef -t ar7
done

# Make sure Legato is running.
ssh root@$targetAddr "/usr/local/bin/legato start"

# Install all the apps.
for app in $appsList
do
    instapp $app.ar7 $targetAddr
done

# Stop all other apps.
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

# Clear the logs.
ssh root@$targetAddr "killall syslogd"
ssh root@$targetAddr "/sbin/syslogd -C1000"

# Run the apps.
for app in $appsList
do
    ssh root@$targetAddr  "/usr/local/bin/app start $app"
done

# Wait for all the apps to finish running.
sleep 3

# Uninstall all apps.
ssh root@$targetAddr  "/usr/local/bin/app stop \"*\""

# Grep the logs to check the results.
checkLogStr "==" 1 "======== Start 'FaultApp/noExit' Test ========"
checkLogStr "==" 1 "======== Start 'FaultApp/noFault' Test ========"
checkLogStr "==" 1 "======== Test 'FaultApp/noFault' Ended Normally ========"
checkLogStr ">" 1 "======== Start 'FaultApp/progFault' Test ========"
checkLogStr ">" 1 "======== Start 'FaultApp/sigFault' Test ========"
checkLogStr ">" 1 "======== Start 'RestartApp/noExit' Test ========"
checkLogStr "==" 1 "======== Start 'StopApp/noExit' Test ========"

checkLogStr "==" 1 "======== Start 'NonSandboxedFaultApp/noExit' Test ========"
checkLogStr "==" 1 "======== Start 'NonSandboxedFaultApp/noFault' Test ========"
checkLogStr "==" 1 "======== Test 'NonSandboxedFaultApp/noFault' Ended Normally ========"
checkLogStr ">" 1 "======== Start 'NonSandboxedFaultApp/progFault' Test ========"
checkLogStr ">" 1 "======== Start 'NonSandboxedFaultApp/sigFault' Test ========"
checkLogStr ">" 1 "======== Start 'NonSandboxedRestartApp/noExit' Test ========"
checkLogStr "==" 1 "======== Start 'NonSandboxedStopApp/noExit' Test ========"

echo "Supervisor Test Passed!"
exit 0
