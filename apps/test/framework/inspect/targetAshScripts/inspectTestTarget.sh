#!/bin/sh

export PATH=$PATH:/legato/systems/current/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin

Fail()
{
    RetVal=$?
    echo "Inspect Test Target Failed"
    exit $RetVal
}

# TODO: all inspect tests should test columnar data for all possible data value or strings,
# like testInspectMutexColumns.sh and testInspectSemaColumns.sh


########################################
# Inspect Mempools Tests ###############
########################################

runInspectPools()
{
    deleteStrat=$1
    deleteInterval=$2
    subPoolNum=$3
    config set "apps/subpoolFlux/procs/subpoolFlux/args/1" $deleteStrat
    config set "apps/subpoolFlux/procs/subpoolFlux/args/2" $deleteInterval
    config set "apps/subpoolFlux/procs/subpoolFlux/args/3" $subPoolNum
    app restart subpoolFlux
}

testInspectPoolsInterrupted()
{
    runInspectPools "1toN" 50000000 1000
    ./testInspectInterrupted.sh pools || Fail
}

# Test Case: Test if the inspect tool report a completed list correctly
testInspectPoolsComplete()
{
    # 1 item
    runInspectPools "None" 0 1
    ./testInspectComplete.sh pools $subPoolNum || Fail

    # large-ish number
    runInspectPools "None" 0 90
    ./testInspectComplete.sh pools $subPoolNum || Fail
}

# Test Case: Perform inspection while mem pools are deleted.
testInspectPoolsRaceCondition()
{
    runInspectPools "1toN" 0 100000
    ./testInspectRace.sh || Fail
}

testInspectPoolsInterval()
{
    # Create 1 subpool
    runInspectPools "None" 0 1

    # Test case 1: see if inspect can handle an interval of 0
    ./testInspectInterval.sh 0 || Fail

    # Test case 2: interval of 1 is the smallest interval one can set - this represents a boundary case.
    ./testInspectInterval.sh 1 || Fail

    # Test case 3: interval of 5 is one that's larger than the default of 3.  This can be even bigger but
    # the test duration might be too long.
    ./testInspectInterval.sh 5 || Fail

    # Test case 4: this tests the "-f" option which should default to an interval of 3 secs.
    ./testInspectInterval.sh f || Fail

    # Test case 5: this tests the default retry interval of 0.5 sec when there are changes to the item under inspection
    # Create 100k subpools and delete from 1 to N, in order to create the scenario of constantly
    # changing pool list.
    runInspectPools "1toN" 0 100000

    ./testInspectInterval.sh flux || Fail
}


########################################
# Inspect Timers Tests #################
########################################

runInspectTimers()
{
    deleteStrat=$1
    deleteInterval=$2
    timerNum=$3
    threadNum=$4

    config set "apps/timerFlux/procs/timerFlux/args/1" $deleteStrat
    config set "apps/timerFlux/procs/timerFlux/args/2" $deleteInterval
    config set "apps/timerFlux/procs/timerFlux/args/3" $timerNum
    config set "apps/timerFlux/procs/timerFlux/args/4" $threadNum
    app restart timerFlux
}

# TODO: For "list in list" such as mutex, timer, and semaphore, two specific sceanrios need to be tested.
# 1. Deleting 1 to N-1 thread objects. (currently covered)
# 2. Deleting 1 to N-1 threads. (currently lacking)
testInspectTimersInterrupted()
{
    runInspectTimers "1toN-1" 50000000 1000 1
    ./testInspectInterrupted.sh timers || Fail
}

testInspectTimersComplete()
{
    # 1 item
    runInspectTimers "None" 0 1 1
    ./testInspectComplete.sh timers $timerNum || Fail

    # large-ish number, in 1 thread
    runInspectTimers "None" 0 101 1
    ./testInspectComplete.sh timers $timerNum || Fail

    # large-ish number, in multiple threads
    runInspectTimers "None" 0 101 7
    ./testInspectComplete.sh timers $timerNum || Fail
}

testInspectTimersDelAllTimersIn1stThread()
{
    runInspectTimers "AllTimers1stThread" 0 20 5
    ./testInspectDeleteThread.sh timers 0 $timerNum $threadNum || Fail
}

testInspectTimersDelAllTimersInMidThread()
{
    runInspectTimers "AllTimersMidThread" 0 20 5
    ./testInspectDeleteThread.sh timers 2 $timerNum $threadNum || Fail
}

testInspectTimersDel1stThread()
{
    runInspectTimers "1stThread" 0 20 5
    ./testInspectDeleteThread.sh timers 0 $timerNum $threadNum || Fail
}

testInspectTimersDelMidThread()
{
    runInspectTimers "MidThread" 0 20 5
    ./testInspectDeleteThread.sh timers 2 $timerNum $threadNum || Fail
}


########################################
# Inspect Threads Tests ################
########################################

runInspectThreads()
{
    deleteStrat=$1
    deleteInterval=$2
    threadNum=$3
    config set "apps/threadFlux/procs/threadFlux/args/1" $deleteStrat
    config set "apps/threadFlux/procs/threadFlux/args/2" $deleteInterval
    config set "apps/threadFlux/procs/threadFlux/args/3" $threadNum
    app restart threadFlux
}

testInspectThreadsInterrupted()
{
    runInspectThreads "1toN" 50000000 350
    ./testInspectInterrupted.sh threads || Fail
}

testInspectThreadsComplete()
{
    # 1 item
    runInspectThreads "None" 0 1
    ./testInspectComplete.sh threads $threadNum || Fail

    # large-ish number
    runInspectThreads "None" 0 103
    ./testInspectComplete.sh threads $threadNum || Fail
}


########################################
# Inspect Mutexes Tests ################
########################################

runInspectMutexes()
{
    testType=$1
    deleteInterval=$2
    mutexNum=$3
    threadNum=$4

    config set "apps/mutexFlux/procs/mutexFlux/args/1" $testType

    # Set the option only if it's provided, and if it's not, actively delete the argument.
    # There could be left-over arguments from previous tests. Unhandled arguments make
    # test app barf.
    [ "$deleteInterval" ] && { config set "apps/mutexFlux/procs/mutexFlux/args/2" $deleteInterval; true; }\
                          || config delete "apps/mutexFlux/procs/mutexFlux/args/2"

    [ "$mutexNum" ] && { config set "apps/mutexFlux/procs/mutexFlux/args/3" $mutexNum; true; }\
                    || config delete "apps/mutexFlux/procs/mutexFlux/args/3"

    [ "$threadNum" ] && { config set "apps/mutexFlux/procs/mutexFlux/args/4" $threadNum; true; }\
                     || config delete "apps/mutexFlux/procs/mutexFlux/args/4"

    app restart mutexFlux
}

# TODO: For "list in list" such as mutex, timer, and semaphore, two specific sceanrios need to be tested.
# 1. Deleting 1 to N-1 thread objects. (currently covered)
# 2. Deleting 1 to N-1 threads. (currently lacking)
testInspectMutexesInterrupted()
{
    runInspectMutexes "1toN-1" 50000000 1000 5
    ./testInspectInterrupted.sh mutexes || Fail
}

testInspectMutexesComplete()
{
    # 1 item
    runInspectMutexes "None" 0 1 1
    ./testInspectComplete.sh mutexes $mutexNum || Fail

    # large-ish number, in 1 thread
    runInspectMutexes "None" 0 75 1
    ./testInspectComplete.sh mutexes $mutexNum || Fail

    # large-ish number, in multiple threads
    runInspectMutexes "None" 0 75 5
    ./testInspectComplete.sh mutexes $mutexNum || Fail
}

testInspectMutexesDelAllMutexesIn1stThread()
{
    runInspectMutexes "AllMutexes1stThread" 0 23 4
    ./testInspectDeleteThread.sh mutexes 0 $mutexNum $threadNum || Fail
}

testInspectMutexesDelAllMutexesInMidThread()
{
    runInspectMutexes "AllMutexesMidThread" 0 23 4
    ./testInspectDeleteThread.sh mutexes 2 $mutexNum $threadNum || Fail
}

testInspectMutexesWaitingList()
{
    runInspectMutexes "TestWaitingList"
    ./testInspectMutexWaitingList.sh || Fail
}

testInspectMutexesRecursive()
{
    runInspectMutexes "TestRecursive"
    ./testInspectMutexColumns.sh "RecursiveMutex1" "recursive" 3 || Fail
}


########################################
# Inspect Semaphores Tests #############
########################################

# TODO: change other runInspectXXX to this style
runInspectSemaphores()
{
    testType=$1
    config set "apps/semaphoreFlux/procs/semaphoreFlux/args/1" $testType

    if [ $# -eq 1 ]
    then
        config delete "apps/semaphoreFlux/procs/semaphoreFlux/args/2"
        config delete "apps/semaphoreFlux/procs/semaphoreFlux/args/3"
    elif [ $# -eq 2 ]
    then
        threadNum=$2
        config set "apps/semaphoreFlux/procs/semaphoreFlux/args/2" $threadNum
        config delete "apps/semaphoreFlux/procs/semaphoreFlux/args/3"
    elif [ $# -eq 3 ]
    then
        deleteInterval=$2
        threadNum=$3
        config set "apps/semaphoreFlux/procs/semaphoreFlux/args/2" $deleteInterval
        config set "apps/semaphoreFlux/procs/semaphoreFlux/args/3" $threadNum
    else
        echo "runInspectSemaphores - bad argument list."
        false || Fail
    fi

    app restart semaphoreFlux
}

testInspectSemaphoresInterrupted()
{
    runInspectSemaphores "1toN-1Threads" 50000 100
    ./testInspectInterrupted.sh semaphores || Fail

    runInspectSemaphores "Sem1toN-1Threads" 50000 100
    ./testInspectInterrupted.sh semaphores || Fail
}

testInspectSemaphoresComplete()
{
    # 1 item
    runInspectSemaphores "None" 1
    ./testInspectComplete.sh semaphores $threadNum || Fail

    # large-ish number, in multiple threads
    runInspectSemaphores "None" 100
    ./testInspectComplete.sh semaphores $threadNum || Fail
}

testInspectSemaphoresPostSemIn1stThread()
{
    runInspectSemaphores "Sem1stThread" 10
    ./testInspectDeleteThread.sh semaphores 0 $threadNum $threadNum || Fail
}

testInspectSemaphoresPostSemInMidThread()
{
    runInspectSemaphores "SemMidThread" 10
    ./testInspectDeleteThread.sh semaphores 5 $threadNum $threadNum || Fail
}

testInspectSemaphoresDel1stThread()
{
    runInspectSemaphores "1stThread" 10
    ./testInspectDeleteThread.sh semaphores 0 $threadNum $threadNum || Fail
}

testInspectSemaphoresDelMidThread()
{
    runInspectSemaphores "MidThread" 10
    ./testInspectDeleteThread.sh semaphores 5 $threadNum $threadNum || Fail
}

testInspectSemaphoresWaitingList()
{
    runInspectSemaphores "TestWaitingList"
    ./testInspectSemaWaitingList.sh || Fail
}


#######################
# script body #########
#######################


### Inspect Pool tests #######
testInspectPoolsInterrupted
testInspectPoolsComplete
testInspectPoolsInterval

# This test is very long (20+ minutes).
#testInspectPoolsRaceCondition

# clean up
app stop subpoolFlux



### Inspect Timers tests #######
testInspectTimersInterrupted
testInspectTimersComplete

testInspectTimersDelAllTimersIn1stThread
testInspectTimersDelAllTimersInMidThread
testInspectTimersDel1stThread
testInspectTimersDelMidThread

# clean up
app stop timerFlux



### Inspect Threads tests #######
testInspectThreadsInterrupted
testInspectThreadsComplete

# clean up
app stop threadFlux



### Inspect Mutex tests #######
testInspectMutexesInterrupted
testInspectMutexesComplete

testInspectMutexesDelAllMutexesIn1stThread
testInspectMutexesDelAllMutexesInMidThread

testInspectMutexesWaitingList
testInspectMutexesRecursive

# clean up
app stop mutexFlux



### Inspect Semaphore tests #######
testInspectSemaphoresInterrupted
testInspectSemaphoresComplete

testInspectSemaphoresPostSemIn1stThread
testInspectSemaphoresPostSemInMidThread
testInspectSemaphoresDel1stThread
testInspectSemaphoresDelMidThread

testInspectSemaphoresWaitingList

# clean up
app stop semaphoreFlux





exit 0




