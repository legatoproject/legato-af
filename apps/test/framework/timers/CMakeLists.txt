#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_COMPONENT timersTest)
set(APP_TARGET testFwTimers)
set(APP_SOURCES
    timerTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c testFwTimers)

#
# Build test for timer expiry fixes.  This is not run as part of the standard
# tests, at least for now.
#

set(TEST_SCRIPT testExpiryFix.sh)
set(TEST_EXE testExpiryFix)

add_legato_internal_executable(${TEST_EXE} testExpiryFix.c)

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

# This is a C test
add_dependencies(tests_c ${TEST_EXE})
