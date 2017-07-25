#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Output
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples/bin)
set(APP_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(samples_c
    COMMENT "Generated C & C++ samples in ${EXECUTABLE_OUTPUT_PATH}"
)

add_custom_target(samples_java
    COMMENT "Generated Java samples in ${EXECUTABLE_OUTPUT_PATH}"
)

add_custom_target(samples
    DEPENDS samples_c samples_java
    COMMENT "Generated all samples in ${EXECUTABLE_OUTPUT_PATH}"
)

# Simple client/server application
add_subdirectory(helloWorld)

# Simple client/server java application
add_subdirectory(javaHelloWorld)

# Simple client application + server application
add_subdirectory(helloIpc)

# Legacy porting example
add_subdirectory(legacy)

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

# lwm2m airvantage asset data sample application
add_subdirectory(assetData)

# lwm2m airvantage control applcation
add_subdirectory(avcControl)

# Simple GPIO Cf3 application using GPIO Service
if((LEGATO_TARGET MATCHES "wp85") OR (LEGATO_TARGET MATCHES "wp750x"))
    add_subdirectory(gpioCf3Demo)
endif()

# Application showing ultra low power mode entry/exit
if (NOT (${LEGATO_TARGET} MATCHES "ar759x" OR ${LEGATO_TARGET} MATCHES "ar758x"))
    add_subdirectory(textLoc)
endif()

# Watchdog
add_subdirectory(watchdog)

