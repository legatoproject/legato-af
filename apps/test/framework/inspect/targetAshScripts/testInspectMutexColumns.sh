#!/bin/sh

logFileName=__Inspect_testMutexColumns_log_deleteme

PrintUsage()
{
    echo "Usage: $0 [mutex name] recursive [number of expected locks]"
    echo "       $0 [mutex name] traceable [expected value]"
}


inspect mutexes `ps -ef | grep MutexFlux | grep -v grep | awk '{print $2}'` >> $logFileName


mutexName=$1
rowUnderTest="$(grep "$mutexName" "$logFileName")"

if [ -z "$rowUnderTest" ]
then
    echo "[FAILED] invalid mutex name [$mutexName]"
    exit 1
fi

if [ "$2" == "recursive" ]
then
    testType=$2
    numLocks=$3

    # Test the 5th column "recursive"
    if [ "$(echo $rowUnderTest | awk '{print $5}')" -ne "1" ] ||
    # Test the 3rd column "lock count"
    [ "$(echo $rowUnderTest | awk '{print $3}')" -ne "$numLocks" ]
    then
        echo "[FAILED] Inspect mutexes incorrectly displays column [$testType] and/or lock count."
        exit 1
    fi
elif [ "$2" == "traceable" ]
then
    testType=$2
    expVal=$3

    # Test the 7th column "traceable"
    actualVal=$(echo $rowUnderTest | awk '{print $7}')

    if [ "$actualVal" -ne "$expVal" ]
    then
        echo "[FAILED] Inspect mutexes incorrectly displays column [$testType]. Expected value [$expVal]; Actual value [$actualVal]"
        exit 1
    fi
else
    PrintUsage
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect mutexes successfully displays column [$testType]."
exit 0
