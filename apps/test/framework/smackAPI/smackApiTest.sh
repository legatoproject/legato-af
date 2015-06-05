#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-ar7}

OnFail() {
    echo "SMACK API Test Failed!"
}

# Copy the test executable to the target.
scp $LEGATO_ROOT/build/$targetType/bin/tests/smackApiTest root@$targetAddr:/tmp/smackApiTest

echo "******** SMACK API Test Starting ***********"

# Run the test.
ssh root@$targetAddr "/tmp/smackApiTest"
CheckRet

echo "SMACK API Test Passed!"
exit 0
