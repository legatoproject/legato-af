#!/bin/bash

LoadTestLib

targetAddr=$1
targetType=${2:-wp85}

OnFail() {
    echo "Secure Storage Test Failed!"
}

# TODO: test.shlib might be a better place for this? TBD.
ClearTargetLog()
{
    #Dynamically create script
    local commands='#!/bin/sh\n
                    killall syslogd\n
                    oldlabel=$(cat /proc/self/attr/current)\n
                    echo syslog > /proc/self/attr/current\n
                    /sbin/syslogd -C2000 &\n
                    echo $oldlabel > /proc/self/attr/current'

    echo -e $commands | ssh root@$targetAddr "cat > /tmp/ClearTargetLog.sh"

    #Run the script
    ssh root@$targetAddr "chmod +x /tmp/ClearTargetLog.sh"
    ssh root@$targetAddr "/tmp/ClearTargetLog.sh"

    #Delete the script from target
    ssh root@$targetAddr "rm /tmp/ClearTargetLog.sh"

    return $?
}

SetUp()
{
    if [ "$LEGATO_ROOT" == "" ]
    then
        if [ "$WORKSPACE" == "" ]
        then
            echo "Neither LEGATO_ROOT nor WORKSPACE are defined." >&2
            exit 1
        else
            LEGATO_ROOT="$WORKSPACE"
        fi
    fi

    #TODO: Should this be placed in test.shlib, and/or run this globally in targetUnitTests? TBD.
    # Change the probation timer.
    echo "Changing the probation timer to prevent the system from rolling back."
    ssh root@$targetAddr "export LE_PROBATION_MS=1; $BIN_PATH/legato stop; $BIN_PATH/legato start"

    # Change current working dir to where the test apps are.
    appDir="$LEGATO_ROOT/build/$targetType/tests/apps"
    cd "$appDir"
    CheckRet
}

# This test verifies simple read, write, and delete, as well as secure storage limit, in test 1a.
# Test 1b runs in parallel and verifies that different apps have their separate storage space.
RunSecStoreTest1()
{
    # Install test apps
    echo "Install test apps"
    instapp secStoreTest1a.$targetType.update $targetAddr
    instapp secStoreTest1b.$targetType.update $targetAddr

    # Clear logs
    echo "Clear the logs."
    ClearTargetLog

    # Run test apps
    echo "Starting secStoreTest1a."
    ssh root@$targetAddr  "$BIN_PATH/app start secStoreTest1a"
    CheckRet

    # Wait for app to finish
    IsAppRunning "secStoreTest1a"
    while [ $? -eq 0 ]; do
        IsAppRunning "secStoreTest1a"
    done

    echo "Starting secStoreTest1b."
    ssh root@$targetAddr  "$BIN_PATH/app start secStoreTest1b"
    CheckRet

    # Cleanup
    echo "Uninstall all apps."
    ssh root@$targetAddr "$BIN_PATH/app remove secStoreTest1a"
    ssh root@$targetAddr "$BIN_PATH/app remove secStoreTest1b"

    # Verification
    echo "Grepping the logs to check the results."
    CheckLogStr "==" 1 "============ SecStoreTest1a PASSED ============="
    CheckLogStr "==" 1 "============ SecStoreTest1b PASSED ============="
}

# This test verifies read, write, and delete "0-byte files".
RunSecStoreTest2()
{
    # Install test apps
    echo "Install test apps"
    instapp secStoreTest2.$targetType.update $targetAddr

    # Clear logs
    echo "Clear the logs."
    ClearTargetLog

    # Run test apps
    echo "Starting secStoreTest2."
    ssh root@$targetAddr  "$BIN_PATH/app start secStoreTest2"
    CheckRet

    # Wait for app to finish
    IsAppRunning "secStoreTest2"
    while [ $? -eq 0 ]; do
        IsAppRunning "secStoreTest2"
    done

    # Cleanup
    echo "Uninstall all apps."
    ssh root@$targetAddr "$BIN_PATH/app remove secStoreTest2"

    # Verification
    echo "Grepping the logs to check the results."
    CheckLogStr "==" 1 "============ SecStoreTest2 PASSED ============="
}

# This test verifies read, write, and delete "0-byte files".
# It uses the same executable as secStoreTest2, but built against secStoreGlobal service.
RunSecStoreTestGlobal()
{
    # Install test apps
    echo "Install test apps"
    instapp secStoreTestGlobal.$targetType.update $targetAddr

    # Clear logs
    echo "Clear the logs."
    ClearTargetLog

    # Run test apps
    echo "Starting secStoreTestGlobal."
    ssh root@$targetAddr  "$BIN_PATH/app start secStoreTestGlobal"
    CheckRet

    # Wait for app to finish
    IsAppRunning "secStoreTestGlobal"
    while [ $? -eq 0 ]; do
        IsAppRunning "secStoreTestGlobal"
    done

    # Cleanup
    echo "Uninstall all apps."
    ssh root@$targetAddr "$BIN_PATH/app remove secStoreTestGlobal"

    # Verification
    echo "Grepping the logs to check the results."
    CheckLogStr "==" 1 "============ SecStoreTestGlobal PASSED ============="

    # Check that secure storage can be accessed using secstore
    VALUE=$(ssh root@$targetAddr "$BIN_PATH/secstore ls")
    RETVAL=$?
    if [ $RETVAL -ne 0 ]; then
        echo "secStoreAdmin is not supported. Checking for entries after app uninstall is omitted"
        exit 0
    fi

    # Check that the entries created by the app are still present
    # after uninstall
    KEY=/global/file1
    VALUE=$(ssh root@$targetAddr "$BIN_PATH/secstore read $KEY")
    CheckRet
    if [ -n "$VALUE" ]; then
        echo "Value should have been empty for 0-byte file $KEY"
        exit 1
    fi
    ssh root@$targetAddr "$BIN_PATH/secstore rm $KEY"
    CheckRet

    # AVMS
    KEY=/global/avms/file1
    VALUE=$(ssh root@$targetAddr "$BIN_PATH/secstore read $KEY")
    CheckRet
    if [[ "$VALUE" != "string321" ]]; then
        echo "Unexpected value for entry $KEY: '$VALUE'"
        exit 1
    fi
    ssh root@$targetAddr "$BIN_PATH/secstore rm $KEY"
    CheckRet
}


##################################
##  Main Script Body  ############
##################################
echo "******** Secure Storage Test Starting ***********"

SetUp
RunSecStoreTest1
RunSecStoreTest2
RunSecStoreTestGlobal

echo "Secure Storage Test Passed!"
exit 0

