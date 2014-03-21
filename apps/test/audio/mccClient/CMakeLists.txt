#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT AudioMccClientTest)
set(APP_TARGET testAudioMccClient)

set(APP_SOURCES
    audioMccClientTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

target_link_libraries(${APP_TARGET}
    ${LEGATO_SERVICES_MODEM_CLIENT_TARGET}
    ${LEGATO_SERVICES_AUDIO_CLIENT_TARGET}
)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

# AR7 Component Implementation
if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")
    add_definitions(-DENABLE_CODEC=1)
endif()