#!/bin/bash
# log_result output the message and save it for summary output at the end.
# All messages that start with FAIL: will be counted as fatal.
# Messages that start with other strings can be used for tests that are either informational
# or test features not yet implemented.

LoadTestLib

app_file_list=""
app_installed_list=""
bin_path="/legato/systems/current/bin/"
exit_code=0
declare -A app_pids
declare -A app_results
declare -A app_test_message

declare -a errors_summary
#************************************
TARGET_ADDR=$1
TARGET_TYPE=$2

if [ -z "$TARGET_TYPE" ];then
    echo "Error: No target type specified"
    exit 1
fi
if [ -z "$TARGET_ADDR" ]; then
    echo "Error: No target address specified"
    exit 1
fi

# need to export this so the launcher script can find the target to listen to
export TARGET_IP_ADDR=$TARGET_ADDR

echo "Running watchdog tests on target ${TARGET_TYPE} at ${TARGET_ADDR}"
#***************************************
function build_apps
{
    echo "Building watchdog test apps"

    # this makes all the apps we need at the moment.
    make targ=${TARGET_TYPE}
    on_fail "One or more watchdog test apps could not be built"

    # build the apps_list from the ls *.${TARGET_TYPE} results and check future tests against people trying to
    # call apps that aren't made
    app_file_list=*.${TARGET_TYPE}.update
}

function install_apps
{
    echo "Installing watchdog test apps on target"
    for app_file in $app_file_list
    do
        instapp $app_file ${TARGET_ADDR}
        on_fail "Failed trying to install $app_file"
        app_name=$(echo "$app_file" | sed 's/\..*$//')
        app_installed_list="$app_installed_list $app_name"
    done
}

function set_test_message
{
    app_test_message[$1]=$2
}

function log_result
{
    echo "$1"
# add the error message to the last position in the error summary array
    errors_summary[${#errors_summary[@]}]="$1"
}

function launch
{
    echo "Launching test $1 $2 $3"
    # first arg is app name
    for app in $app_installed_list
    do
        if [[ $1 == $app ]]; then
            bash -x test_launcher.sh $1 $2 $3 &
            app_pids[$app]=$!
            return
        fi
    done
# if we are here someone tried to launch an app that wasn't installed
    echo "attempt to launch $1 - not an installed app"
    cleanup
    exit 1
}

function wait_for_results
{
    echo "Waiting for results"
    for key in "${!app_pids[@]}"
    do
        wait ${app_pids[$key]}
        app_results[$key]=$?
        unset app_pids[$key]
        if [[ ${app_results[$key]} -ne 0 ]]; then
            log_result "FAIL: ${app_test_message[$key]}"
        else
            log_result "PASS: ${app_test_message[$key]}"
        fi
        echo "$key exited with result ${app_results[$key]}"
    done
}

function cleanup
{
    echo "Cleaning up"
    # we are shutting down. Best effort here, ignore failures.
    #stop all apps
    ssh root@${TARGET_ADDR} "${bin_path}app stop '*'"
    # remove installed test apps
    for app in $app_installed_list
    do
        app remove $app ${TARGET_ADDR}
    done
}

function on_fail
{
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo $1
        cleanup
        exit $exit_code
    fi
}

function config_args
{
    app_name=$1
    proc_name=$2
    args=$3
    arg_num=1

    for arg_value in $args
    do
        base_command="${bin_path}config set apps/${app_name}/procs/${proc_name}/args/${arg_num} ${arg_value}"
        if [[ $config_command == "" ]]; then
            config_command="$base_command"
        else
            config_command="$config_command; $base_command"
        fi
        let arg_num=$arg_num+1
    done

    ssh root@${TARGET_ADDR} ${config_command}
}

# make sure legato is running on target. If start fails maybe it's already running.
#
output=$(ssh root@${TARGET_ADDR} "${bin_path}legato start")
on_fail "ssh root@target legato start failed"
echo $output
legato_start_match="already running|Starting the supervisor"
if ! [[ $output =~ $legato_start_match ]]; then
    echo "Couldn't start legato on target"
    cleanup
    exit 1
fi

# stop all apps on target
ssh root@${TARGET_ADDR} "${bin_path}app stop '*'"
#ignore exit code. Returns failure if apps aren't running on target but this is not an error

build_apps

install_apps

# Test if dogTest has a watchdogTimeout: s/b added by mkapp
wdog_timeout_value=$(ssh root@${TARGET_ADDR} "${bin_path}config get apps/dogTest/watchdogTimeout")
ssh root@${TARGET_ADDR} "${bin_path}config get apps/dogTest/watchdogTimeout"
if [[ $wdog_timeout_value == "" ]]; then
    log_result "FAIL: mkapp didn't pass the watchdogTimeout: config from dogTest.adef"
else
    if [[ $wdog_timeout_value -ge 0 ]]; then
        log_result "PASS: watchdogTimeout appears in config"
    else
        log_result "FAIL: watchdogTimeout has nonsense value in config"
    fi
fi

# Test if dogTestNever has "watchdogTimeout: never" :s/b added by mkapp
wdog_timeout_never_value=$(ssh root@${TARGET_ADDR} "${bin_path}config get apps/dogTestNever/watchdogTimeout")
if [[ $wdog_timeout_never_value != "-1" ]]; then
    log_result "FAIL: mkapp didn't pass the 'watchdogTimeout: never' config from dogTestNever.adef"
else
    log_result "PASS: dogTestNever has the correct config watchdogTimeout value"
fi

# Test if dogTest has a watchdogAction: s/b added by mkapp
wdog_action_value=$(ssh root@${TARGET_ADDR} "${bin_path}config get apps/dogTest/watchdogAction")
if [[ $wdog_action_value == "" ]]; then
    log_result "FAIL: mkapp didn't pass the watchdogAction: config from dogTest.adef"
else
    log_result "PASS: found watchdogAction in dogTest config"
fi

# test that watchdog expires at configured timeout
if [[ ${wdog_timeout_value} != "" ]]; then
    export DOG_TEST_TIMEOUT=${wdog_timeout_value}
    let first_time=$wdog_timeout_value-100
    let test_time_max=$wdog_timeout_value*3
    if [[ ${first_time} -le 0 ]]; then
        log_result "FAIL: Watchdog config watchdogTimeout value set wrong for this test"
    else
        config_args dogTest dogTest "${first_time} 200"
        set_test_message dogTest "Test if watchdog times out as configured by watchdogTimeout:"
        launch dogTest dogTestWatcher.sh 10
        wait_for_results
    fi
else
    log_result "WARN: Skipping test using the timeout value that mkapp passed from the adef because it didn't pass one"
fi

# test that watchdog can handle a different (arbitrary) timeout
export DOG_TEST_TIMEOUT=500
ssh root@${TARGET_ADDR} "${bin_path}config set apps/dogTest/watchdogTimeout 500 int"
on_fail "couldn't configure watchdogTimeout on target"
config_args dogTest dogTest "400 50"
set_test_message dogTest "Test if watchdog times out as per new timeout value we write to config watchdogTimeout:"
launch dogTest dogTestWatcher.sh 10
wait_for_results

# test that watchdog uses default when there is no watchdogTimeout: configured
export DOG_TEST_TIMEOUT=30000
ssh root@${TARGET_ADDR} "${bin_path}config delete apps/dogTest/watchdogTimeout"
config_args dogTest dogTest "29950 100"
# after setting up the test we can run it concurrently with the rest of the tests below
set_test_message dogTest "Test if watchdog times out at 30 second default with missing watchdogConfig:"
launch dogTest dogTestWatcher.sh 90
sleep 2 # need a little time for each test to launch or ssh connection setups saturate the target

set_test_message dogTestNever "Testing config watchdogTimeout: never"
launch dogTestNever dogTestNeverWatcher.sh 70
sleep 2

set_test_message dogTestNeverNow "Test watchdog special values TIMEOUT_NEVER/TIMEOUT_NOW"
launch dogTestNeverNow dogTestNeverNowWatcher.sh 180
sleep 2

set_test_message dogTestRevertAfterTimeout "Test if watchdog reverts to regular timeout after extended timeout"
launch dogTestRevertAfterTimeout dogTestRevertAfterTimeoutWatcher.sh 120
sleep 2

wait_for_results

cleanup

# Count the ones that actually say FAIL:
# Then set the error code based on that - which will allow WARN: or PROVISIONAL_PASS: or similar to be used for tests that currently don't work.
if [[ ${#errors_summary[@]} -gt 0 ]]; then
    echo "================================================================="
    echo "watchdog test suit error summary"
    for error in "${errors_summary[@]}"
    do
        if [[ $error =~ ^FAIL: ]]; then
            exit_code=1
        fi
        echo "    > $error"
    done
    echo "================================================================="
fi

if [[ $exit_code -ne 0 ]]; then
    echo "FAIL: watchdog test set contains failed tests"
else
    echo "PASS: all watchdog tests passed"
fi

exit $exit_code
