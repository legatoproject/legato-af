# dogTestWatcher
# This script watches the output of the dogTest.
# dogTest is a simple test that increments a sleep timer between kicks until the watchdog
# times out. The test is successful if the watchdog times out once we sleep for more
# than the configured timeout value but never for less.
# We need to know the configured timeout value when we run. This is exported by the controlling
# script. For this reason, and the fact that dogTest is a single app, one instance executable
# on the target, multiple dogTestWatchers should never be run concurrently.

# There is some slop that is to be expected when timeout time and sleep time
# are getting close. We could get into the situation where see a timeout and yet we wake up
# because we haven't been killed yet. However, we shouldn't be able to do this more than once.
# If we get two sleeps longer than the timeout without timeout - we have definitely failed.

# DOG_TEST_TIMEOUT (from env) is in milliseconds but test measures are microseconds
let target_timeout=${DOG_TEST_TIMEOUT}*1000

TEST_NAME='dogTest'
test_pid='XXXXXXXXXXX'

sleep_time=0
wake_time=0
unexpected_wakes=0
expect_timeout='false'

echo "--dogTestWatcher.sh starting"
#find where the supervisor starts the test and get the pid
start_match="supervisor.*\| Starting process $TEST_NAME with pid ([0-9]*)"

match_kick="le_wdog_Kick then sleep for ([0-9]*) usec"
match_wake="slept for ([0-9]*) usec"

while read line
do
if [[ $line =~ $start_match ]]; then
    test_pid=${BASH_REMATCH[1]}
    echo "--$TEST_NAME started with pid ${test_pid}"
fi
if [[ $line =~ $test_pid ]]; then
# These are potential lines of interest
    echo "---$line"

    if [[ $line =~ $match_kick ]]; then
        sleep_time=${BASH_REMATCH[1]}
        if [[ $sleep_time -ge $target_timeout ]]; then
            echo "--setting expect_timeout true"
            expect_timeout='true'
        fi
    fi

    if [[ $line =~ $match_wake ]]; then
        wake_time=${BASH_REMATCH[1]}
        if [[ $expect_timeout == 'true' ]]; then
            echo "--Got unexpected wake"
            let unexpected_wakes=$unexpected_wakes+1
            if [[ $unexpected_wakes -gt 0 ]]; then
                echo '--FAIL'
                exit 1
            fi
        fi
    fi

    timedout_match="proc ${test_pid} timed out"
    if [[ $line =~ $timedout_match ]]; then
        echo "--Got a time out"
        if [[ $expect_timeout = 'true' ]]; then
            echo "--PASS"
            exit 0
        else
            echo "--FAIL"
            exit 1
        fi
    fi
fi
done
