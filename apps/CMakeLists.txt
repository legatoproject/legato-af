#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Samples
add_subdirectory(sample EXCLUDE_FROM_ALL)

# Tools
add_subdirectory(tools)

# Tests
enable_testing()

# UTs
add_subdirectory(test)
