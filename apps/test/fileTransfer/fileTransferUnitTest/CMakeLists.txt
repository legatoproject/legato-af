#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXEC fileTransferUnitTest)

set(LEGATO_FILE_STREAM "${LEGATO_ROOT}/components/fileStream")
set(SIMU_CONFIG_TREE "${CMAKE_CURRENT_SOURCE_DIR}/simu/")

if(TEST_COVERAGE EQUAL 1)
    set(CFLAGS "--cflags=\"--coverage\"")
    set(LFLAGS "--ldflags=\"--coverage\"")
endif()

mkexe(${TEST_EXEC}
    .
    -i ${LEGATO_FILE_STREAM}/fileStreamClient
    -i ${LEGATO_FILE_STREAM}/fileStreamServer
    -i ${LEGATO_ROOT}/components/cfgEntries
    -i ${LEGATO_ROOT}/framework/liblegato
    -i ${PA_DIR}/simu/components/le_pa
    -i ${PA_DIR}/simu/components/simuConfig
    -i ${SIMU_CONFIG_TREE}
    -s ${PA_DIR}
    --cflags="-DWITHOUT_SIMUCONFIG"
    ${CFLAGS}
    ${LFLAGS}
)

add_test(${TEST_EXEC} ${EXECUTABLE_OUTPUT_PATH}/${TEST_EXEC})

# This is a C test
add_dependencies(tests_c ${TEST_EXEC})
