#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

SetTargetIP "${targetAddr}"

tempFiles=""

OnExit() {
    if [ -n "$tempFiles" ]; then
        ssh root@$targetAddr rm -rf $tempFiles
    fi
}

OnFail() {
    echo "Core Test Failed!"
# Try to remove apps we may have installed
# No checking returns. We are already dead.
    app remove badAppNSB $targetAddr
    app remove badAppSB $targetAddr
}

function RemoteCmd
{
    ssh root@$targetAddr "$1"
    CheckRet
}


function inst
{
    appName=$1
    appPath="$LEGATO_ROOT/build/$targetType/bin/tests/${appName}.$targetType"

    echo "Installing test app $appName from $appPath."
    instapp "$appPath" $targetAddr
    CheckRet
}


function uninst
{
    appName=$1

    echo "Removing test app $appName."
    app remove "$appName" $targetAddr
    CheckRet
}


function testApp
{
    appName=$1

    echo "Clear out old logs."
    logLoc="/tmp/legato_logs/syslog-$appName-badExe-"

    RemoteCmd "rm -rf $logLoc"

    # just in case we fail - this will be cleaned up on exit
    tempFiles+="$logLoc "

    echo "Now running $appName."
    app start $appName $targetAddr
    CheckRet

    CheckLogfileStr "$logLoc" "==" 1 "Something wicked this way comes."

    RemoteCmd "rm -rf $logLoc"
    tempFiles=""
}


echo "Make sure Legato is running."
ssh root@$targetAddr "$BIN_PATH/legato start"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "$BIN_PATH/app stop \"*\""
sleep 1

echo "Clear the logs."
ClearLogs

inst badAppSB
testApp badAppSB
uninst badAppSB

inst badAppNSB
testApp badAppNSB
uninst badAppNSB

