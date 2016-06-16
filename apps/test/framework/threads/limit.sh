#!/bin/bash

LOG_PATH=$PWD/threadLimitLog
echo "Log Path: $LOG_PATH"

if [ -z "$1" ]; then
    echo "ERROR: No path given"
    exit 1
fi

echo "Executing $1 ..."
$1 2>&1 | tee $LOG_PATH
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "ERROR: Process was successful, that's unexpected"
    exit 1
fi

echo "Checking log ..."
if ! grep "code 11" $LOG_PATH; then
    echo "ERROR: Expected to see code 11 in the log"
    exit 1
fi

exit 0
