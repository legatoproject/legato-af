#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "SMACK Test Failed!"
}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# List of apps
appsList="fileClient fileServer"

echo "******** SMACK Test Starting (on $targetType @ $targetAddr) ***********"

cd $scriptDir

echo "Build all the apps."
for app in $appsList
do
    mkapp ${app}.adef -t $targetType
    CheckRet
done

echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

ClearLogs

echo "Install the fileClient app."
InstallApp fileClient

echo "Run the fileClient app."
ssh root@$targetAddr  "$BIN_PATH/app start fileClient"
CheckRet

echo "Install the fileServer app."
InstallApp fileServer

echo "Run the fileServer app."
ssh root@$targetAddr  "$BIN_PATH/app start fileServer"
CheckRet

# Wait for all the apps to finish running.
sleep 1

echo "Copying rogue process to target."
scp $LEGATO_ROOT/build/$targetType/tests/bin/rogue root@$targetAddr:/tmp/rogue

echo "Running rogue process"
ssh root@$targetAddr "/tmp/rogue"
CheckRet

echo "Stop all apps."
ssh root@$targetAddr  "$BIN_PATH/app stop \"*\""

echo "Grepping the logs to check the results."
CheckLogStr "==" 1 "File descriptor was passed correctly."

echo "SMACK Test Passed!"
exit 0
