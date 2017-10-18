#************************************************************************************
# Copyright (C) Sierra Wireless Inc.
#************************************************************************************

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

# Build language samples when building tests
add_dependencies(tests_c samples_c)

add_dependencies(tests_java samples_java)
