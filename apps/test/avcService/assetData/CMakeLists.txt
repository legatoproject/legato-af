#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(TEST_EXE testAssetData)
set(TEST_SCRIPT testAssetData.sh)


# build the test executable
mkexe(${TEST_EXE}
      assetDataTest
      -i ${LEGATO_ROOT}/interfaces
      -i ${LEGATO_ROOT}/components/airVantage/avcDaemon/
      -i ${LEGATO_ROOT}/framework/c/src
      DEPENDS legato
              ${LEGATO_ROOT}/interfaces/le_cfg.api
              ${LEGATO_ROOT}/components/airVantage/avcDaemon/assetData.c
              assetDataTest/assetDataTest.c)


# This goes into the "tests" directory, with all the other executables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
               ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})



