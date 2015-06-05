#!/bin/sh


if [ $# -ne 1 ]
then
    echo "Usage: $0 [expected last subpool number]"
    exit 1
fi


appName=SubpoolFlux
logFileName=__InspectMemoryPool_testComplete_log_deleteme
expectedLastSubpoolNum=$1

# The line that indicates the list is complete.
CompleteListLine="End of List"

# The last subpool - which represents the end of list
lastSubPool="Subpool${expectedLastSubpoolNum}"


inspect pools `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` > $logFileName

if ! grep -q "$CompleteListLine" "$logFileName"
then
    echo "[FAILED] Inspect does not report the line declaring inspect completion [$CompleteListLine]"
    exit 1
fi

if grep -q "$CompleteListLine" "$logFileName" &&
   ! tail -n 2 "$logFileName" | grep -q "$lastSubPool"
then
    echo "[FAILED] Inspect declares false list completion"
    exit 1
fi

# clean up
rm $logFileName

exit 0
