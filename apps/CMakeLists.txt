#************************************************************************************
# Copyright (C) Sierra Wireless Inc.
#************************************************************************************

# Samples
add_subdirectory(sample)

# UTs
add_subdirectory(test)

# Build language samples when building tests
add_dependencies(tests_c samples_c)

add_dependencies(tests_java samples_java)
