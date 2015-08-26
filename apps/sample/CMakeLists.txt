#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Output
set(EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}/samples)
set(APP_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(samples
    COMMENT "Generated samples in ${EXECUTABLE_OUTPUT_PATH}"
)

# Simple client/server application
add_subdirectory(helloWorld)

# Simple client application + server application
add_subdirectory(helloIpc)

# Application using Modem Services
add_subdirectory(modemDemo)

# Command-line application
add_subdirectory(commandLine)

# Application using eCall
if((INCLUDE_ECALL EQUAL 1) AND ((LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86")))
    add_subdirectory(eCallDemo)
endif()

# SMS Sample application using Modem Services
add_subdirectory(sms)

# Simple shell piping application
add_subdirectory(shellPiping)

