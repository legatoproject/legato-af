#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "Inspect Test Failed"
}

appDir="$LEGATO_ROOT/build/$targetType/bin/tests"
appName="SubpoolFlux"

ashScriptPath=targetAshScripts

targetTestScripts=(
    sampleMemPools
    v
    inspectTestTarget.sh
    testInspectInterval.sh
    testInspectComplete.sh
    testInspectRace.sh
)

targetFileDir=__InspectTargetTestsDir_deleteme

# Install the SubpoolFlux test app
echo "Installing the test app [$appName]"
instapp $appDir/$appName.$targetType $targetAddr
CheckRet


# Copy over the on-target test script files
echo "Creating a dir on target [$targetAddr] to store the on target scripts [<home dir>/$targetFileDir]"
ssh root@$targetAddr "mkdir -p $targetFileDir"
CheckRet

for script in ${targetTestScripts[@]}
do
    script="$ashScriptPath/$script"
    if [ ! -e $script ]
    then
        echo "Script [$script] does not exist"
        exit 1
    else
        scp $script root@$targetAddr:~/$targetFileDir
        CheckRet
    fi
done


# Run the tests
echo "Running the Inspect tests"
ssh root@$targetAddr "cd $targetFileDir && ./inspectTestTarget.sh"
CheckRet


# Clean up
echo "Cleaning up after the Inspect tests"
rmapp $appName $targetAddr
CheckRet
ssh root@$targetAddr "rm -rf $targetFileDir"
CheckRet

