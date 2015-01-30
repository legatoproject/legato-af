#!/bin/bash

targetAddr=$1
targetType=${2:-ar7}

# Checks if the logStr is in the logs.
#
# params:
#       logPath         Path to the log file to check.
#       comparison      Either "==", ">", "<", "!="
#       numMatches      The number of expected matches.
#       logStr          The log string to search for.
#
function checkLogStr
{
    logPath=$1
    comparison=$2
    expectedMatches=$3
    logStr=$4

    numMatches=$(ssh root@$targetAddr "cd $(dirname $logPath) && ls -1 | grep '$(basename $logPath)' | xargs grep -c '$logStr'")

    echo "$numMatches matches for '$logStr' with logPath='$logPath'"

    if [ ! $numMatches $comparison $expectedMatches ]
    then
        echo "$numMatches occurrences of '$logStr' but $comparison $expectedMatches expected"
        echo "Supervisor Test Failed!"
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
    rmapp "$appName" $targetAddr
    CheckRet
}


function testApp
{
    appName=$1

    echo "Clear out old logs."
    logLoc="/tmp/legato_logs/syslog-$appName-badExe-"

    RemoteCmd "rm -rf $logLoc"

    echo "Now running $appName."
    startapp $appName $targetAddr
    CheckRet

    checkLogStr "$logLoc" "==" 1 "Something wicked this way comes."
}


echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

echo "Clear the logs."
ssh root@$targetAddr "killall syslogd"
CheckRet

ssh root@$targetAddr "/mnt/flash/startup/fg_02_RestartSyslogd"
CheckRet


inst badAppSB
testApp badAppSB
uninst badAppSB

inst badAppNSB
testApp badAppNSB
uninst badAppNSB

