#************************************************************************************
# Copyright (C) Sierra Wireless Inc.
#************************************************************************************

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/bin/fwupdateTest)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# Samples
add_subdirectory(sample)

# UTs
add_subdirectory(test)

# UTs for AirVantageConnector
if(EXISTS ${LEGATO_ROOT}/apps/platformServices/airVantageConnector/apps/test)
    add_subdirectory(platformServices/airVantageConnector/apps/test)

    # Build avc tests when building tests
    add_dependencies(tests_c avc_tests_c)
endif()

# UTs for fwupdateDualsys
if(EXISTS ${LEGATO_ROOT}/platformAdaptor/fwupdate/apps/test)
    add_subdirectory(${LEGATO_ROOT}/platformAdaptor/fwupdate/apps/test ${EXECUTABLE_OUTPUT_PATH})
endif()

# Build language samples when building tests
add_dependencies(tests_c samples_c)

add_dependencies(tests_java samples_java)
