#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "SMACK API Test Failed!"
}

# Copy the test executable to the target.
scp $LEGATO_ROOT/build/$targetType/tests/bin/smackApiTest root@$targetAddr:/tmp
CheckRet

scp $LEGATO_ROOT/build/$targetType/tests/lib/libComponent_smackAPI.so root@$targetAddr:/tmp
CheckRet

echo "******** SMACK API Test Starting ***********"

# Run the test.
ssh root@$targetAddr "LD_LIBRARY_PATH=/tmp /tmp/smackApiTest"
CheckRet

echo "SMACK API Test Passed!"
exit 0

