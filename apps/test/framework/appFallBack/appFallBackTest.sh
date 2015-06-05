#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "App Fall Back Test Failed!"
}

function CheckAppIsInstalled
{
    appName=$1

    numMatches=$(ssh root@$targetAddr "/usr/local/bin/app status $appName | grep -c \"not installed\"")

    if [ $numMatches -ne 0 ]
    then
        echo -e $COLOR_ERROR "App $appName should be installed but isn't." $COLOR_RESET
        exit 1
    fi
}


function CheckAppIsRunning
{
    appName=$1

    numMatches=$(ssh root@$targetAddr "/usr/local/bin/app status $appName | grep -c \"running\"")

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
appDir="$LEGATO_ROOT/build/$targetType/bin/samples"

echo "Make sure Legato is running."
ssh root@$targetAddr "/usr/local/bin/legato start"
CheckRet

echo "Install helloWorld app from sample apps."
cd "$appDir"
instapp helloWorld.$targetType $targetAddr
CheckRet

echo "Check that the app installed properly."
CheckAppIsInstalled "helloWorld"

echo "Attempt to install a corrupted version of the helloWorld app."
cd "$scriptDir"
instapp helloWorld.ar7 $targetAddr

echo "Check that the helloWorld app is still installed."
CheckAppIsInstalled "helloWorld"

echo "Check helloWorld app is still functional."
ssh root@$targetAddr "/usr/local/bin/app start helloWorld"
CheckAppIsRunning "helloWorld"

echo "App Fall Back Test Passed!"
exit 0
