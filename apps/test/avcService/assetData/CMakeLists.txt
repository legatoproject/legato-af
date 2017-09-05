#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(TEST_EXE testAssetData)
set(TEST_SCRIPT testAssetData.sh)


# build the test executable
mkexe(${TEST_EXE}
      assetDataTest
      -i ${LEGATO_ROOT}/interfaces
      -i ${LEGATO_ROOT}/components/airVantage/avcDaemon/
      -i ${LEGATO_ROOT}/framework/liblegato
      -i ${LEGATO_ROOT}/components/airVantage/platformAdaptor/inc
)

# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

# This is a C test
add_dependencies(tests_c ${TEST_EXE})
