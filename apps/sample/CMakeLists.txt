#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Output
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples/bin)
set(APP_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples)
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

# lwm2m airvantage control applcation
add_subdirectory(lwm2mAirVantageControl)

# Simple GPIO Cf3 application using GPIO Service
if(LEGATO_TARGET MATCHES "wp85")
    add_subdirectory(gpioCf3Demo)
endif()

add_subdirectory(textLoc)

# Two functioning tools that can be used with AirVantage to gather information about the system
# and apps in the field.
add_subdirectory(avAppsInfo)
add_subdirectory(avSystemInfo)
