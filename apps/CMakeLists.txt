#************************************************************************************
# Copyright 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
#************************************************************************************

# Samples
add_subdirectory(sample EXCLUDE_FROM_ALL)

# Tools
add_subdirectory(tools)

# Tests
enable_testing()

# UTs
add_subdirectory(test)
