#!/bin/bash

STUB_APP_PATH=$1
TEST_APP_PATH=$2
TEST_XML=$3
TEST_GNSS=$4
TEST_GNSS_BLOCK=$5
TEST_SOCKET=$6

if [ -z $STUB_APP_PATH ] || [ -z $TEST_APP_PATH ] || [ -z $TEST_XML ] || [ -z $TEST_GNSS ] || [ -z $TEST_GNSS_BLOCK ] || [ -z $TEST_SOCKET ]; then
    echo "Usage: STUB_APP_PATH TEST_APP_PATH TEST_XML TEST_GNSS TEST_GNSS_BLOCK TEST_SOCKET must be set"
    exit -1
fi

# Launch stub in bg
echo "Stub: $STUB_APP_PATH $TEST_XML $TEST_GNSS $TEST_GNSS_BLOCK $TEST_SOCKET "
$STUB_APP_PATH $TEST_XML $TEST_GNSS $TEST_GNSS_BLOCK $TEST_SOCKET &
STUB_RETVAL=$?
STUB_PID=$!

if [ $STUB_RETVAL -ne 0 ]; then
    echo "Stub has returned with error"
    exit -1
fi

# Wait for the stub to set up
sleep 1

# Launch test
$TEST_APP_PATH
TEST_RETVAL=$?

if [ $TEST_RETVAL -ne 0 ]; then
    echo "Test has FAILED: returned with error ... $TEST_RETVAL"
    echo "Killing stub ... @$STUB_PID"
    kill $STUB_PID
else
    echo "Test has PASSED"
    kill $STUB_PID
fi

echo "Test retval $TEST_RETVAL"
exit $TEST_RETVAL



