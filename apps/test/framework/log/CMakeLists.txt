#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC logTest)

# Configure file

set(TESTLOG_LOGDAEMON_PATH              ${LEGATO_BINARY_DIR}/framework/bin/logCtrlDaemon)
set(TESTLOG_SERVICE_DIRECTORY_PATH      ${LEGATO_BINARY_DIR}/framework/bin/serviceDirectory)
set(TESTLOG_LOGTOOL_PATH                ${LEGATO_BINARY_DIR}/framework/bin/log)
set(TESTLOG_STDERR_FILE_PATH            ${CMAKE_CURRENT_BINARY_DIR}/logStdErr.txt)
set(TESTLOG_LOGTEST_PATH                ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

add_definitions(-DTESTLOG_LOGTOOL_PATH="${TESTLOG_LOGTOOL_PATH}"
                -DTESTLOG_STDERR_FILE_PATH="${TESTLOG_STDERR_FILE_PATH}"
                -I${LEGATO_ROOT}/framework/liblegato)

# Executable

set_legato_component(${TEST_EXEC})

add_legato_executable(${TEST_EXEC}
    main.c
    component1.c
    component1Helper.c
    component2.c
)

# Testing

# FIXME: log tool has evolved and tests have not been updated
#add_test(testFwLog ${CMAKE_CURRENT_SOURCE_DIR}/testFwLog.sh)
#set_tests_properties(testFwLog PROPERTIES
#    ENVIRONMENT "SERVICE_DIRECTORY_PATH=${TESTLOG_SERVICE_DIRECTORY_PATH};LOGDAEMON_PATH=${TESTLOG_LOGDAEMON_PATH};LOG_STDERR_PATH=${TESTLOG_STDERR_FILE_PATH};LOGTOOL_PATH=${TESTLOG_LOGTOOL_PATH};LOGTEST_PATH=${TESTLOG_LOGTEST_PATH}")

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
