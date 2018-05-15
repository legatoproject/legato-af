#---------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#---------------------------------------------------------------------------------------------------

### Copy test setup script to test program output directory.
file( COPY testFwMessaging-Setup DESTINATION ${EXECUTABLE_OUTPUT_PATH} )


### TEST 1

set(TEST_NAME testFwMessaging-Test1)

mkexe(  ${TEST_NAME} -v
            messagingTest1.c
            burgerServer.c
        )

add_test(${TEST_NAME} ${EXECUTABLE_OUTPUT_PATH}/${TEST_NAME})

# This is a C test
add_dependencies(tests_c ${TEST_NAME})


### TEST 2

set(TEST_NAME testFwMessaging-Test2)

mkexe(  ${TEST_NAME}
            messagingTest2.c
            burgerServer.c
        )

add_test(${TEST_NAME} ${EXECUTABLE_OUTPUT_PATH}/${TEST_NAME})

# This is a C test
add_dependencies(tests_c ${TEST_NAME})


### TEST 3

set(TEST_NAME testFwMessaging-Test3)

mkexe(  ${TEST_NAME}-client
            messagingTest3-client.c
        )

mkexe(  ${TEST_NAME}-server
            messagingTest3-server.c
        )

mkexe(  ${TEST_NAME}
            messagingTest3.c
        )

add_test(${TEST_NAME} ${EXECUTABLE_OUTPUT_PATH}/${TEST_NAME})

# This is a C test
add_dependencies(tests_c ${TEST_NAME}
                         ${TEST_NAME}-client
                         ${TEST_NAME}-server)
