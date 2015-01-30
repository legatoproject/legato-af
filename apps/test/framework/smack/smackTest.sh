#!/bin/bash

targetAddr=$1
targetType=${2:-ar7}

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

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


function CheckRet
{
    RETVAL=$?
    if [ $RETVAL -ne 0 ]; then
        echo -e $COLOR_ERROR "Exit Code $RETVAL" $COLOR_RESET
        exit $RETVAL
    fi
}


# List of apps
appsList="fileClient fileServer"

echo "******** SMACK Test Starting (on $targetType @ $targetAddr) ***********"

cd $scriptDir

echo "Build all the apps."
for app in $appsList
do
    mkapp $app.adef -t $targetType
    CheckRet
done

echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

echo "Stop all other apps."
ssh root@$targetAddr "/usr/local/bin/app stop \"*\""
sleep 1

echo "Clear the logs."
ssh root@$targetAddr "killall syslogd"
CheckRet

# Must restart syslog this way so that it gets the proper SMACK label.
ssh root@$targetAddr "/mnt/flash/startup/fg_02_RestartSyslogd"
CheckRet

echo "Install the fileClient app."
instapp fileClient.$targetType $targetAddr
CheckRet

echo "Run the fileClient app."
ssh root@$targetAddr  "/usr/local/bin/app start fileClient"
CheckRet

echo "Install the fileServer app."
instapp fileServer.$targetType $targetAddr
CheckRet

echo "Run the fileServer app."
ssh root@$targetAddr  "/usr/local/bin/app start fileServer"
CheckRet

# Wait for all the apps to finish running.
sleep 1

echo "Copying rogue process to target."
scp $LEGATO_ROOT/build/$targetType/bin/tests/rogue root@$targetAddr:/tmp/rogue

echo "Running rogue process"
ssh root@$targetAddr "/tmp/rogue"
CheckRet

echo "Stop all apps."
ssh root@$targetAddr  "/usr/local/bin/app stop \"*\""

echo "Grepping the logs to check the results."
checkLogStr "==" 1 "File descriptor was passed correctly."

echo "SMACK Test Passed!"
exit 0
