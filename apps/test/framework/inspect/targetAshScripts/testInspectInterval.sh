#!/bin/sh


if [ $# -ne 1 ]
then
    echo "Usage: $0 [interval under test | f | flux]"
    exit 1
fi


# implementation dependent ##########
defaultRefreshInterval=3
defaultRetryInterval=0.5
#####################################

logFileName=__InspectMemoryPool_testInterval_log_deleteme
appName=subpoolFlux
intervalUnderTest=$1

if [ "$intervalUnderTest" == "f" ]
then
    inspectOption="-f"
    intervalUnderTest=$defaultRefreshInterval

elif [ "$intervalUnderTest" == "flux" ]
then
    # doesn't matter if it's the f or interval option.
    inspectOption="-f"
    intervalUnderTest=$defaultRetryInterval

elif ! echo "$intervalUnderTest" | grep -q "^[0-9]\+$"
then
    echo "bad inspect option"
    exit 1

else
    inspectOption="--interval=${intervalUnderTest}"

    # If the supplied interval is 0, then pass that to inspect which should revert to the default value.
    # However we do need to adjust the corresponding variable in this test script so the later calculation
    # is correct.
    if [ $intervalUnderTest -eq 0 ]
    then
        intervalUnderTest=$defaultRefreshInterval
    fi
fi


testDuration=`echo "$intervalUnderTest" | awk '{ print $1 * 10 }'`

# This is the key phrase to look for to determine a refresh has occured.
# --- NOTE --- that this does not verify the content validity or completeness of each refresh.
inspectionEndKeyPhrase="Legato Memory Pools Inspector"


# start inspection in the background and save its pid
inspect pools $inspectOption `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` > $logFileName &
pid=$!


# abandon the first sample
while [ ! -s $logFileName ]
do
    usleep 10000

    # This is to catch the case when inspect immediately exits due to for example bad params.
    if ! ps -ef | grep $pid | grep -vq grep
    then
        echo "inspect failed to run"
        exit 1
    fi
done

# idling for the defined test duration
# TODO: convert testDuration to microsecs and use usleep since testDuration might not be integer
sleep $testDuration

# stop the inspection
kill $pid


actualOccurances=`grep "$inspectionEndKeyPhrase" $logFileName | wc -l`

if [ $actualOccurances -eq 0 ]
then
    echo "[FAILED] Either Inspect is not refreshing _at all_ or the key phrase to look for is wrong."
    exit 1
fi

# Note again that the first sample is abandoned (hence subtracting 1 from actual occurances)
aveInterval=`echo "$testDuration $actualOccurances" | awk '{ print( $1 / ($2 - 1) ) }'`

echo "actual occurances is [$actualOccurances] times for [$testDuration] secs; actual ave interval is [$aveInterval] secs"

if echo "$aveInterval $intervalUnderTest" | awk '{ if ($1 < $2) exit 0; else exit 1 }'
then
    echo "[FAILED]. Actual ave interval is [$aveInterval], interval under test is [$intervalUnderTest]"
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED]. Inspect successfully refreshed with interval [$intervalUnderTest]"
exit 0


