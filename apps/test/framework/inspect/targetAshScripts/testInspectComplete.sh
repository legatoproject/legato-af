#!/bin/sh

if [ $# -ne 2 ]
then
    echo "Usage: $0 [inspect cmd] [expected number of inspect rows]"
    exit 1
fi


logFileName=__Inspect_testComplete_log_deleteme
cmd=$1
expectedNumRows=$2
actualNumRows=

# The pattern for matching a row.
rowPattern=

case "$cmd" in
    pools)
        appName="SubpoolFlux"
        rowPattern="Subpool[0-9]"
        ;;
    threads)
        appName="ThreadFlux"
        rowPattern="Thread[0-9]"
        ;;
    timers)
        appName="TimerFlux"
        rowPattern=".*_[0-9]"
        ;;
    mutexes)
        appName="MutexFlux"
        rowPattern="Mutex[0-9]"
        ;;
    semaphores)
        appName="SemaphoreFlux"
        rowPattern="Sem[0-9]"
        ;;
    *)
        echo "[FAILED] unknown inspect cmd [$appName]."
        exit 1
        ;;
esac

inspect $cmd `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` >> $logFileName

actualNumRows=$(grep -c "$rowPattern" "$logFileName")
if [ $actualNumRows -ne $expectedNumRows ]
then
    echo "[FAILED] Inspect [$cmd] declares false list completion. Expected [$expectedNumRows] rows. Actual [$actualNumRows]."
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect [$cmd] successfully declared correct completion [$expectedNumRows]."
exit 0
