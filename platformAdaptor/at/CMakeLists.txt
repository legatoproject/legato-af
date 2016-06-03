#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

# Tests
enable_testing()

# Output
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/lib)
set(APP_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests/apps)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${APP_OUTPUT_PATH})

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")
#    add_subdirectory(components/ATManager)
#    add_subdirectory(modem/at/mrc)
#    add_subdirectory(modem/at/sim)
#    add_subdirectory(modem/at/sms)
endif()

