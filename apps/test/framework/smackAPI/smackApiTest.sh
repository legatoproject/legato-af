#!/bin/bash

LoadTestLib

targetAddr="$1"
targetType="${2:-ar7}"

SetTargetIP "$targetAddr"

OnFail() {
    echo "SMACK API Test Failed!"
}

echo "******** SMACK API Test Starting ***********"

# Test if SMACK is disabled
SshToTarget '[ ! -e "/legato/SMACK_DISABLED" ] || exit 42'
if [ $? -eq 42 ]; then
    echo "Skipping SMACK API Testing as SMACK is disabled on target."
    exit 0
fi

# Copy the test executable to the target.
ScpToTarget "$LEGATO_ROOT/build/$targetType/tests/bin/smackApiTest" /tmp
CheckRet

ScpToTarget "$LEGATO_ROOT/build/$targetType/tests/lib/libComponent_smackAPI.so" /tmp
CheckRet

# Run the test.
SshToTarget "LD_LIBRARY_PATH=/tmp /tmp/smackApiTest"
CheckRet

echo "SMACK API Test Passed!"
exit 0

