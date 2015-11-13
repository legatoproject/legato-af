# dogTestNever
# This script watches the output of the dogTestNever.
# We can't realistically wait forever but ...
# If the TIMEOUT_NEVER runs for close to a minute then
# it has exceeded the default timeout of 30 seconds.
# Assume that the "never" value was correctly read and is
# being honoured.

TEST_NAME='dogTestNever'
test_pid='XXXXXXXXXXX'

never_time=0
now_time=0

#find where the supervisor starts the test and get the pid
start_match="supervisor.*\| Starting process $TEST_NAME with pid ([0-9]*)"
timedout_match="proc ${test_pid} timed out"

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

    if [[ $line =~ 'dogTestNever still alive' ]]; then
        now_time=$(date +%s)
        if [[ $(($now_time-$never_time)) -gt 31 ]]; then
            echo "--PASS"
            exit 0
        else
            echo "--FAIL: test ended unexpectedly early"
            exit 1
        fi
    fi

    if [[ $line =~ $timedout_match ]]; then
        echo "--FAIL: unexpected timeout"
        exit 1
    fi
fi
done
