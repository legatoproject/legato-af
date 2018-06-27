#!/bin/bash

LoadTestLib

targetAddr="$1"
targetType="${2:-ar7}"

SetTargetIP "$targetAddr"

OnFail() {
    echo "App Fall Back Test Failed!"
}

function CheckAppIsInstalled
{
    appName=$1

    numMatches=$(SshToTarget "$BIN_PATH/app status $appName | grep -c \"not installed\"")

    if [ $numMatches -ne 0 ]
    then
        echo -e $COLOR_ERROR "App $appName should be installed but isn't." $COLOR_RESET
        exit 1
    fi
}


function CheckAppIsRunning
{
    appName=$1

    numMatches=$(SshToTarget "$BIN_PATH/app status $appName | grep -c \"running\"")

    if [ $numMatches -eq 0 ]
    then
        echo -e $COLOR_ERROR "App $appName is not running." $COLOR_RESET
        exit 1
    fi
}


echo "******** App Fall Back Test Starting ***********"

if [ "$LEGATO_ROOT" == "" ]
then
    if [ "$WORKSPACE" == "" ]
    then
        echo "Neither LEGATO_ROOT nor WORKSPACE are defined." >&2
        exit 1
    else
        LEGATO_ROOT="$WORKSPACE"
    fi
fi

scriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
appDir="$LEGATO_ROOT/build/$targetType/samples"

echo "Make sure Legato is running."
SshToTarget "$BIN_PATH/legato start"
CheckRet

echo "Install helloWorld app from sample apps."
cd "$appDir"
InstallApp helloWorld

echo "Check that the app installed properly."
CheckAppIsInstalled "helloWorld"

echo "Attempt to install a corrupted version of the helloWorld app."
cd "$scriptDir"
InstallApp helloWorld.ar7 false
if [ $? -eq 0 ]; then
    echo "Expected the install to fail"
    exit 1
fi

echo "Check that the helloWorld app is still installed."
CheckAppIsInstalled "helloWorld"

echo "Check that the app is still running."
CheckAppIsRunning "helloWorld"

echo "Check helloWorld app is still functional."
SshToTarget "$BIN_PATH/app restart helloWorld"
CheckRet

echo "App Fall Back Test Passed!"
exit 0

