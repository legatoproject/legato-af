#!/bin/sh

if [ $# -ne 4 ]
then
    echo "Usage: $0 [inspect cmd] [deleted thread idx] [thread obj num] [thread num]"
    echo "deleted thread idx - the thread index whose thread objects or the thread itself are deleted."
    echo "thread obj num - number of objects belonging to a thread, such as timer/mutex/semaphore."
    echo "thread num - number of threads that the thread objects are evenly spreaded out to."
    exit 1
fi


logFileName=__Inspect_testDeleteThread_log_deleteme
cmd=$1
delThreadIdx=$2
threadObjNum=$3
threadNum=$4


case "$cmd" in
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
        echo "[FAILED] invalid inspect cmd [$appName]."
        exit 1
        ;;
esac


inspect $cmd `ps -ef | grep $appName | grep -v grep | awk '{print $2}'` >> $logFileName


# NOTE: "ThreadX" is artificially planted in the test app. In the future the Inspect tool might
# report thread names. The test will then need to be modified accordingly.
if grep -q "Thread${delThreadIdx}" "$logFileName"
then
    echo "[FAILED] Inspect [$cmd] still has thread objects in a deleted thread, or a thread with supposedly empty thread object list."
    exit 1
fi


remThreadObjNum=$(( threadObjNum - (threadObjNum / threadNum) ))

if [ $(grep -c "Thread" "$logFileName") -ne $remThreadObjNum ]
then
    echo "[FAILED] Inspect [$cmd] - incorrect remaining thread object number [$remThreadObjNum]"
    exit 1
fi

# clean up
rm $logFileName

echo "[PASSED] Inspect [$cmd] successfully reported thread objects when some thread objects belonging to a thread are all deleted [$delThreadIdx]."
exit 0
