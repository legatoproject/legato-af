#!/bin/bash

targetAddr=$1
targetType=${2:-ar7}

function CheckRet
{
    RETVAL=$?
    if [ $RETVAL -ne 0 ]; then
        echo -e $COLOR_ERROR "Exit Code $RETVAL" $COLOR_RESET
        exit $RETVAL
    fi
}

# Copy the test executable to the target.
scp $LEGATO_ROOT/build/$targetType/bin/tests/smackApiTest root@$targetAddr:/tmp/smackApiTest

echo "******** SMACK API Test Starting ***********"

# Run the test.
ssh root@$targetAddr "/tmp/smackApiTest"
CheckRet

echo "SMACK API Test Passed!"
exit 0
