#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXEC testFwUtf8)

add_executable(${TEST_EXEC} main.c)

target_link_libraries(${TEST_EXEC} legato)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})
