#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC testFwClock)

find_package(CUnit REQUIRED)

include_directories(${CUNIT_INCLUDE_DIRS})

add_executable(${TEST_EXEC} main.c)

add_dependencies(${TEST_EXEC} cunit)
target_link_libraries(${TEST_EXEC} legato ${CUNIT_LIBRARIES})

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})
