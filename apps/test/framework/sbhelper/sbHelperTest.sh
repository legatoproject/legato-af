#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-wp85}
appName=fileAccess

echo "******** Sandboxing Tools Test Starting ***********"

# Make a copy of the test adef so that we don't overwrite the original.
rm $appName.adef >> /dev/null 2>&1
cp $appName.origAdef $appName.adef

# Start the sbhelper and feed inputs to accept all menu options.
printf "\n\n\n\n\n\n\n" | sbhelper $appName $targetType $targetAddr

ssh root@$targetAddr "$BIN_PATH/app start $appName"

sleep 1

IsAppRunning "$appName"
if [ $? -ne 0 ]; then
    echo "Sandboxing Tools Test Failed!"
    exit 1
fi

echo "Sandboxing Tools Test Passed!"
exit 0
