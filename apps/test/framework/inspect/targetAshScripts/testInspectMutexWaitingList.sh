#!/bin/sh

logFileName=__Inspect_testMutexWaitingList_log_deleteme

# Note that this verification script is hardcoded to test a specific scenario.
#
# Cheat sheet - this is the table to look for.
#                    NAME | LOCK COUNT | RECURSIVE | TRACEABLE |             WAITING LIST |
#                  Mutex3 |          1 |         0 |         0 |                  Thread5 |
#                                                                                 Thread4
#                  Mutex2 |          1 |         0 |         0 |                   (null) |
#                  Mutex1 |          1 |         0 |         0 |                  Thread3 |
#                                                                                 Thread2

inspect mutexes `ps -ef | grep mutexFlux | grep -v grep | awk '{print $2}'` >> $logFileName


if  ! grep Mutex3 "$logFileName" | grep -q Thread5 ||
    ! grep Mutex3 -A 1 "$logFileName" | grep -q Thread4
then
    echo "[FAILED] Mutex3 does not have Thread4 or Thread5 on its waiting list."
    exit 1
fi

if  ! grep Mutex2 "$logFileName" | grep -q null
then
    echo "[FAILED] Mutex2 does not have null on its waiting list."
    exit 1
fi

if  ! grep Mutex1 "$logFileName" | grep -q Thread3 ||
    ! grep Mutex1 -A 1 "$logFileName" | grep -q Thread2
then
    echo "[FAILED] Mutex1 does not have Thread3 or Thread2 on its waiting list."
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect mutexes successfully displays its waiting lists."
exit 0
