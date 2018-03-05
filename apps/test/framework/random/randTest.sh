#!/bin/bash

#---------------------------------------------------------------------------------------------------
#  Bash script for running the random framework tests.
#
#  Copyright (C) Sierra Wireless Inc.
#
#---------------------------------------------------------------------------------------------------

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

SetTargetIP "$targetAddr"

set -x

OnFail() {
    echo "Randomness Test Failed!"
}

OnExit() {
    # best effort - clean up
    echo "##########  Removing the exe."
    SshToTarget "rm -f testFwRand"
}

TestPerf()
{
    ScpToTarget "$LEGATO_ROOT/build/$targetType/tests/bin/testFwRand" "testFwRand"
    CheckRet

    SshToTarget "./testFwRand --performance"
    CheckRet
}

if [[ "$targetType" != "virt"* ]]; then
    TestPerf
fi
