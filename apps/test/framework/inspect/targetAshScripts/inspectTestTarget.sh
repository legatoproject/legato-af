#!/bin/sh

export PATH=$PATH:/usr/local/bin:/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin

Fail() {
    RetVal=$?
    echo "Inspect Test Target Failed"
    exit $RetVal
}

# Test Case: Test if the inspect tool report a completed list correctly
testInspectComplete()
{
    subPoolNum=1
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/1" None
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/3" $subPoolNum
    app restart SubpoolFlux

    ./testInspectComplete.sh $((subPoolNum - 1)) || Fail
}

# Test Case: Perform inspection while mem pools are deleted.
testInspectRaceCondition()
{
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/1" 1toN
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/3" 100000
    app restart SubpoolFlux

    ./testInspectRace.sh || Fail
}

testInspectInterval()
{
    # Create 1 subpool
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/1" None
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/3" 1
    app restart SubpoolFlux

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
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/1" 1toN
    config set "apps/SubpoolFlux/procs/SubpoolFlux/args/3" 100000
    app restart SubpoolFlux

    ./testInspectInterval.sh flux || Fail
}


#######################
# script body #########
#######################

testInspectComplete
# This test is very long (20+ minutes).
#testInspectRaceCondition
testInspectInterval

# clean up
app stop SubpoolFlux


exit 0




