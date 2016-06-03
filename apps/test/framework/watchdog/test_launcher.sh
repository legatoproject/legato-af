#!/bin/bash
# launches an ssh session to read the output of logread -f
# launches the log watcher for the current test.
# hooks the stdout of the ssh logread to the stdin of the log watching script by fifo
# starts the test app on the target
# waits for the log watcher to finish with a result or kill it if it's taking too long
# report the result of the log watcher

# expects TARGET_IP_ADDR to be exported and that it is the IP address of the target machine

LoadTestLib

target_test_name=$1
log_watcher=$2
max_elapsed_time=$3


pipe_name="${target_test_name}_pipe"

function cleanup
{
    # Get rid of the pipe
    if [ -p ${pipe_name} ]; then
        echo "-removing pipe ${pipe_name}"
        rm "${pipe_name}"
    fi

    # Make sure the app is stopped
    ssh root@${TARGET_IP_ADDR} "$BIN_PATH/app stop ${target_test_name}"

    return 0
}

function on_fail ()
{
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo $1
        cleanup
        exit $exit_code
    fi
}

echo "-Creating pipe ${pipe_name}"
mkfifo "${pipe_name}"
on_fail "-Couldn't create pipe ${pipe_name}"

#-o StrictHostKeyChecking=no will cause yes/no to be suppressed and keys to be added without intervention
ssh -o StrictHostKeyChecking=no root@${TARGET_IP_ADDR} /sbin/logread -f >${pipe_name} &
on_fail "-ssh couldn't connect to ${TARGET_IP_ADDR}"
ssh_pid=$!

bash ${log_watcher} < ${pipe_name} &
on_fail "-${log_watcher} failed to start"
watcher_pid=$!

# start the test app
ssh root@${TARGET_IP_ADDR} "$BIN_PATH/app start ${target_test_name}"
on_fail "-ssh couldn't start ${target_test_name} on ${TARGET_IP_ADDR}"

elapsed_time=0
while [[ 1 ]]
do
    sleep 5
    let elapsed_time=$elapsed_time+5
    if ps -p $watcher_pid > /dev/null
    then
        # log watching task is still alive
        if [[ $elapsed_time -gt ${max_elapsed_time} ]]; then
            # but it should have stopped by now. Remove it.
            echo "-Test app exceeded max test time. Terminating"
            kill $watcher_pid
            kill $ssh_pid
            cleanup
            exit 1
            break
        fi
    else
        # log watching task has died a natural death
        kill $ssh_pid
        break
    fi
done

wait $ssh_pid
wait $watcher_pid
watcher_exit_value=$?

cleanup

exit $watcher_exit_value
