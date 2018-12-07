#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Output
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples/lib)
set(APP_OUTPUT_PATH ${CMAKE_BINARY_DIR}/samples/apps)
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${APP_OUTPUT_PATH})

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
if(INCLUDE_ECALL EQUAL 1)
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
if((LEGATO_TARGET STREQUAL "wp76xx") OR (LEGATO_TARGET STREQUAL "wp85") OR (LEGATO_TARGET STREQUAL "wp750x"))
    add_subdirectory(gpioCf3Demo)
    add_subdirectory(gpioCf3SSDemo)
endif()

# Application showing ultra low power mode entry/exit
if (NOT (LEGATO_TARGET MATCHES "ar7"))
    add_subdirectory(textLoc)
endif()

# Watchdog
add_subdirectory(watchdog)

# Application adding the streamMedia API which allows audio streaming through RTP
add_subdirectory(streamMedia)

# Application that gets online and fetches an URL with curl
if (NOT (LEGATO_TARGET STREQUAL "ar7" OR LEGATO_TARGET STREQUAL "ar86"))
    add_subdirectory(httpGet)
endif()

# Application that exposes a webserver (lighttpd) and few sample pages
if (NOT (LEGATO_TARGET STREQUAL "ar7" OR LEGATO_TARGET STREQUAL "ar86" OR LEGATO_TARGET STREQUAL "localhost"))
    add_subdirectory(httpServer)
endif()

# DataHub sample app
add_subdirectory(datahubGps)

# MQTT sample app
add_subdirectory(mqtt)
