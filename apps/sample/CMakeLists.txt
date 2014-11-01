#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
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

# Application using eCall
if(INCLUDE_ECALL EQUAL 1 AND LEGATO_TARGET MATCHES "ar7")
    add_subdirectory(eCall)
endif()


# Application using AirVantage services
if(INCLUDE_AIRVANTAGE)
    add_subdirectory(airvantageDemo)
endif()

# SMS Sample application using Modem Services
add_subdirectory(sms)
