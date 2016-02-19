# dogTestNeverNow
# This script watches the output of the dogTestNeverNow.
# If the TIMEOUT_NEVER runs for close to a minute and the
# TIMEOUT_NOW causes an immediate timeout then all is well.
# (Due to the granularity of 'date' I leave a second of slack
# in the comparisons)
# TODO
# The test app itself outputs FAIL if it is not killed by the supervisor
# but that functionality is not yet implemented to kill the app.
# When it is it would be nice to note the point at which the app was
# successfully expunged.

TEST_NAME='dogTestNeverNow'
test_pid='XXXXXXXXXXX'

never_time=0
now_time=0

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

    if [[ $line =~ 'TIMEOUT_NEVER' ]]; then
        never_time=$(date +%s)
    fi

    if [[ $line =~ 'TIMEOUT_NOW' ]]; then
        now_time=$(date +%s)
    fi

# Currently this app outputs the FAIL line because the funtionality to get the supervisor
# to kill it is missing. TODO
#    if [[ $line =~ 'FAIL' ]]; then
#        exit 1
#    fi

    timedout_match="proc ${test_pid} timed out"
    if [[ $line =~ $timedout_match ]]; then
        time_of_death=$(date +%s)
        if [[ $(($now_time-$never_time)) -gt 59 &&
                $(($time_of_death-$now_time)) -le 1 ]]; then
            echo "--PASS"
            exit 0
        else
            echo "--FAIL"
            exit 1
        fi
    fi
fi
done
