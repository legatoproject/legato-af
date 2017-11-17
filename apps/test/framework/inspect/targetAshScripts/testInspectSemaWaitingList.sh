#!/bin/sh

logFileName=__Inspect_testSemaWaitingList_log_deleteme

# Note that this verification script is hardcoded to test a specific scenario.
#
# Cheat sheet - this is the table to look for.
#                            NAME | TRACEABLE |             WAITING LIST |
#                      Semaphore1 |         0 |                  Thread3 |
#                                                                Thread2
#                                                                Thread1
#                      Semaphore1 |         0 |                  Thread3 |
#                                                                Thread2
#                                                                Thread1
#                      Semaphore1 |         0 |                  Thread3 |
#                                                                Thread2
#                                                                Thread1
#                      Semaphore2 |         0 |                  Thread4 |
#                      Semaphore3 |         0 |                  Thread5 |

inspect semaphores `ps -ef | grep semaphoreFlux | grep -v grep | awk '{print $2}'` >> $logFileName


if [ $(grep Semaphore1 "$logFileName" | grep -c Thread3) -ne 3 ] ||
   [ $(grep Semaphore1 -A 1 "$logFileName" | grep -c Thread2) -ne 3 ] ||
   [ $(grep Semaphore1 -A 2 "$logFileName" | grep -c Thread1) -ne 3 ]
then
    echo "[FAILED] Semaphore1 does not have Thread1, Thread2, or Thread3 on its waiting list."
    exit 1
fi

if  ! grep Semaphore2 "$logFileName" | grep -q Thread4
then
    echo "[FAILED] Semaphore2 does not have Thread4 on its waiting list."
    exit 1
fi

if  ! grep Semaphore3 "$logFileName" | grep -q Thread5
then
    echo "[FAILED] Semaphore3 does not have Thread5 on its waiting list."
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect semaphores successfully displays its waiting lists."
exit 0
