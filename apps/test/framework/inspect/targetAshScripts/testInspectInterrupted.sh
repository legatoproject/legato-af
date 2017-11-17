#!/bin/sh

if [ $# -ne 1 ]
then
    echo "Usage: $0 [inspect cmd]"
    exit 1
fi

logFileName=__Inspect_testInterrupted_log_deleteme
cmd=$1
DetectedChangesPattern=">>> Detected list changes. Stopping inspection. <<<"

case "$cmd" in
    pools)
        appName="subpoolFlux"
        ;;
    threads)
        appName="threadFlux"
        ;;
    timers)
        appName="timerFlux"
        ;;
    mutexes)
        appName="mutexFlux"
        ;;
    semaphores)
        appName="semaphoreFlux"
        ;;
    *)
        echo "[FAILED] unknown inspect cmd [$appName]."
        exit 1
        ;;
esac

inspect $cmd `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` >> $logFileName

retryMax=10
retryCnt=1
while ! grep -q "$DetectedChangesPattern" "$logFileName" &&
      [ $retryCnt -le $retryMax ]
do
    inspect $cmd `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` >> $logFileName
    retryCnt=$((retryCnt+1))
done

if [ $retryCnt -gt $retryMax ]
then
    echo "[FAILED] Inspect [$cmd] does not report inspection being interrupted after [$retryMax] retries."
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect [$cmd] successfully report inspection being interrupted."
exit 0
