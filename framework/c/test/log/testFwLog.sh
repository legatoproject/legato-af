#!/bin/bash

echo ""
echo "*** Unit Test for le_log, log daemon and log control tool. ***"

# Settings

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "$LOGDAEMON_PATH" ]; then
    LOGDAEMON_PATH=$SCRIPT_DIR/../../logDaemon
fi
if [ -z "$LOGTOOL_PATH" ]; then
    LOGTOOL_PATH=$SCRIPT_DIR/../../log
fi
if [ -z "$LOG_STDERR_PATH" ]; then
    LOG_STDERR_PATH=logStdErr.txt
fi
if [ -z "$LOGTEST_PATH" ]; then
    LOGTEST_PATH=$SCRIPT_DIR/logTest
    echo "LOGTEST_PATH not set, default to $LOGTEST_PATH"
fi

LOGDAEMON_SOCKET="/tmp/le_LogDaemon"
LOGTOOL_SOCKET="/tmp/logTool"

echo "LOGDAEMON_PATH=$LOGDAEMON_PATH"
echo "LOGTOOL_PATH=$LOGTOOL_PATH"
echo "LOG_STDERR_PATH=$LOG_STDERR_PATH"

if [[ ! -f $LOGDAEMON_PATH ]] || [[ ! -f $LOGTOOL_PATH ]] || [[ ! -f $LOGTEST_PATH ]]; then
    echo "A path is not good"
    exit -1
fi

echo "..."

function cleanup {
    if [[ -e $LOGDAEMON_SOCKET ]]; then
        echo "Removing $LOGDAEMON_SOCKET"
        ls -al $LOGDAEMON_SOCKET
        rm -f $LOGDAEMON_SOCKET
    fi

    if [[ -e $LOGTOOL_SOCKET ]]; then
        echo "Removing $LOGTOOL_SOCKET"
        ls -al $LOGTOOL_SOCKET
        rm -f $LOGTOOL_SOCKET
    fi
}


if [[ -e $LOGDAEMON_SOCKET ]] || [[ -e $LOGTOOL_SOCKET ]]; then
    echo "Daemon or tool socket exist at $LOGDAEMON_SOCKET or $LOGTOOL_SOCKET, trying to remove them."
    cleanup

    if [[ -e $LOGDAEMON_SOCKET ]] || [[ -e $LOGTOOL_SOCKET ]]; then
        echo "Unable to delete them, failing."
        file $LOGDAEMON_SOCKET $LOGTOOL_SOCKET
        exit -1
    fi
fi

if [[ -f $LOG_STDERR_PATH ]]; then
    echo "Removing previous stderr log"
    rm -f $LOG_STDERR_PATH
fi

# Processing test

testResult=0

function check_ret {
    RETVAL=$?
    if [ $RETVAL -ne 0 ]; then
        echo -e $COLOR_ERROR "Exit Code $RETVAL" $COLOR_RESET
        testResult=$RETVAL
    fi
}

function check_daemons {
    kill -0 $LOGDAEMON_PID
    if [[ $? -ne 0 ]]; then
        echo "Log daemon is dead"
        testResult=-1
    fi

    kill -0 $SERVICE_DIRECTORY_PID
    if [[ $? -ne 0 ]]; then
        echo "Service Directory is dead"
        testResult=-1
    fi
}

echo "Starting the Service Directory."
$SERVICE_DIRECTORY_PATH &
SERVICE_DIRECTORY_PID=$!
echo "... with PID $SERVICE_DIRECTORY_PID"

# Wait for the Service Directory to start.
sleep 0.5

echo "Starting the log daemon."
$LOGDAEMON_PATH &
LOGDAEMON_PID=$!
echo "... with PID $LOGDAEMON_PID"

check_daemons

if [[ $testResult -eq 0 ]]; then
    echo "Setting all log output locations to stderr."
    $LOGTOOL_PATH location stderr
    testResult=$?
fi

if [[ $testResult -eq 0 ]]; then
    echo "Setting all log formats."
    $LOGTOOL_PATH format "%P, %t, %F, %f, %u, %n, %L"
    testResult=$?
fi

if [[ $testResult -eq 0 ]]; then
    echo "Setting all filters to EMERGENCY."
    $LOGTOOL_PATH level EMERGENCY
    testResult=$?
fi

if [[ $testResult -eq 0 ]]; then
    # Wait for the log daemon to process the commands.
    sleep 0.1

    echo "Starting the log test components."
    echo "Redirecting stderr to a file for checking."
    $LOGTEST_PATH 2> $LOG_STDERR_PATH
    testResult=$?
fi

check_daemons

if [[ $testResult -eq 0 ]]; then
    # Kill started processes
    kill -9 $LOGDAEMON_PID
    kill -9 $SERVICE_DIRECTORY_PID
    sleep 0.5
fi

# Cleanup
cleanup

if [[ $testResult -eq 0 ]]; then
    echo "*** Log testing Passed. ***"
else
    echo "*** Log testing Failed.  Check the last line of '$LOG_STDERR_PATH' for more details. ***"
fi

exit $testResult
