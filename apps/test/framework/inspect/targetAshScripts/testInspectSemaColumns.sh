#!/bin/sh

# NOTE: This script is currently not used, since the "traceable" column is removed.
# However I am keeping this around in case implementation changes in the future.


logFileName=__Inspect_testSemaColumns_log_deleteme

PrintUsage()
{
    echo "Usage: $0 [sema name] traceable [expected value]"
}


inspect semaphores `ps -ef | grep semaphoreFlux | grep -v grep | awk '{print $2}'` >> $logFileName


semaName=$1
rowUnderTest="$(grep "$semaName" "$logFileName")"

if [ -z "$rowUnderTest" ]
then
    echo "[FAILED] invalid sema name [$semaName]"
    exit 1
fi

if [ "$2" == "traceable" ]
then
    testType=$2
    expVal=$3

    # Test the 3th column "traceable"
    actualVal=$(echo $rowUnderTest | awk '{print $3}')

    if [ "$actualVal" -ne "$expVal" ]
    then
        echo "[FAILED] Inspect semaphores incorrectly displays column [$testType]. Expected value [$expVal]; Actual value [$actualVal]"
        exit 1
    fi
else
    PrintUsage
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect semaphores successfully displays column [$testType]."
exit 0
