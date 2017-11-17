#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

printBanner()
{
    echo "---------------------------------------------------------------------------"
    echo "---------------------------------- $1"
    echo "---------------------------------------------------------------------------"
}

OnFail()
{
    printBanner "Inspect Test Failed"
}

appDir="$LEGATO_ROOT/build/$targetType/tests/apps"

targetTestApps=(
    subpoolFlux
    threadFlux
    timerFlux
    mutexFlux
    semaphoreFlux
)

ashScriptPath=targetAshScripts

targetTestScripts=(
    sampleMemPools
    v
    inspectTestTarget.sh
    testInspectInterval.sh
    testInspectInterrupted.sh
    testInspectComplete.sh
    testInspectRace.sh
    testInspectDeleteThread.sh
    testInspectMutexWaitingList.sh
    testInspectMutexColumns.sh
    testInspectSemaWaitingList.sh
    testInspectSemaColumns.sh
)

targetFileDir=__InspectTargetTestsDir_deleteme



printBanner "Begin All Inspector Tests"

# Install the test apps
printBanner "Installing All Test Apps"
for app in ${targetTestApps[@]}
do
#    echo "Installing the test app [$app]"
    instapp $appDir/$app.${targetType}.update $targetAddr
    CheckRet
done


# Copy over the on-target test script files
printBanner "Copying Over On-Target Test Script Files"
echo "Creating a dir on target [$targetAddr] to store the on-target scripts [<home dir>/$targetFileDir]"
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
printBanner "Running the Inspector Tests"
ssh root@$targetAddr "cd $targetFileDir && ./inspectTestTarget.sh"
CheckRet


# Clean up
printBanner "Cleaning up after the Inspector Tests"
for app in ${targetTestApps[@]}
do
    app remove $app $targetAddr
    CheckRet
done

ssh root@$targetAddr "rm -rf $targetFileDir"
CheckRet


printBanner "All Inspector Tests Are Successfully Finished"



