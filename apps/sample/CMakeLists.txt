#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Output
set(EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}/samples)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(samples
    COMMENT "Generated samples in ${EXECUTABLE_OUTPUT_PATH}"
)

# Simple client/server application
add_subdirectory(helloWorld)

# Application using Modem Services
add_subdirectory(modemDemo)

# Application using AirVantage services
if(INCLUDE_AIRVANTAGE)
    add_subdirectory(airvantageDemo)
endif()
