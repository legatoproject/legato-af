# dogTestRevertAfterTimeout
# This script watches the output of the dogTestRevertAfterTimeout
# If a le_wdog_Timeout is called the next le_wdog_Kick should revert the
# timeout interval to the configured timeout not the one used in the
# call to le_wdog_Timeout()
# The test app calls le_wdog_Timeout with a timeout and sleep time passed as
# args in the adef. The sleep should be less than timeout so the watchdog doesn't
# expire but longer than the time configured in watchdogTimeout:
# After the sleep the app gives the watchdog a regular kick and we expect the watchdog
# to time out as per the configured watchdogTimeout rather than repeat the length of
# the previous le_wdog_Timeout.

TEST_NAME='dogTestRevertAfterTimeout'
test_pid='XXXXXXXXXXX'

match_start_timeout="Starting timeout ([0-9]*) milliseconds then sleep for ([0-9]*)"
match_start_kick="Kicking with configured timeout then sleep for ([0-9]*)"

start_timeout_time=0
start_kick_time=0
timedout_time=0

#find where the supervisor starts the test and get the pid
start_match="supervisor.*\| Starting process $TEST_NAME with pid ([0-9]*)"

while read line
do
if [[ $line =~ $start_match ]]; then
    test_pid=${BASH_REMATCH[1]}
    echo "--$TEST_NAME started with pid ${test_pid}"
fi

if [[ $line =~ $test_pid ]]; then
# These are potential lines of interest
    echo "---$line"

    if [[ $line =~ $match_start_timeout ]]; then
        start_timeout_time=$(date +%s)
    fi

    if [[ $line =~ $match_start_kick ]]; then
        expected_sleep=${BASH_REMATCH[1]}
        start_kick_time=$(date +%s)
    fi

# Currently this app outputs the FAIL line because the funtionality to get the supervisor
# to kill it is missing. TODO
#    if [[ $line =~ 'FAIL' ]]; then
#        exit 1
#    fi

    timedout_match="proc ${test_pid} timed out"
    if [[ $line =~ $timedout_match ]]; then
        timedout_time=$(date +%s)
        let expiry_time=$timedout_time-$start_kick_time
        if [[ $expiry_time -lt $expected_sleep ]]; then
            echo "--PASS"
            exit 0
        else
            echo "--FAIL"
            exit 1
        fi
    fi
fi
done
