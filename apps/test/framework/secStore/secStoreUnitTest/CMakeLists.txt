#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(LEGATO_SECSTORE "${LEGATO_ROOT}/components/secStore/")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

function(add_secstore_test TEST_NAME)
    set(TEST_EXEC "secStoreUnitTest_${TEST_NAME}")

    mkexe(${TEST_EXEC}
        ${PA_DIR}/simu/components/le_pa_secStore
        secStoreComp
        ${TEST_NAME}
        -i secStoreComp/
        -i ${LEGATO_ROOT}/interfaces/secureStorage/
        -i ${LEGATO_SECSTORE}/platformAdaptor/inc
        -i ${LEGATO_ROOT}/framework/liblegato
        -i ${LEGATO_ROOT}/framework/liblegato/linux
        -i ${LEGATO_ROOT}/components/appCfg
        -i ${PA_DIR}/simu/components/le_pa_secStore
        -i ${PA_DIR}/simu/components/simuConfig
        -C "-fvisibility=default -g"
        --cflags="-DWITHOUT_SIMUCONFIG"
        ${CFLAGS}
        ${LFLAGS}
    )

    add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC} ${ARGN})

   # This is a C test
   add_dependencies(tests_c ${TEST_EXEC})
endfunction()

add_secstore_test(secStoreTest1a -l 8192)
add_secstore_test(secStoreTest1b)
add_secstore_test(secStoreTest2)
add_secstore_test(secStoreTestGlobal)
