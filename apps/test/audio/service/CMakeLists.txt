#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT AudioServiceTest)
set(APP_TARGET testAudioService)

set(APP_SOURCES
    audioTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_SERVICES_MODEM_TARGET}
    ${LEGATO_SERVICES_AUDIO_TARGET}
)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

clear_legato_component(${APP_COMPONENT})


# Test for valgrind to check le_audio implementation
set(MEM_APP_COMPONENT AudioServiceMemoryTest)
set(MEM_APP_TARGET testAudioServiceMemory)

set(MEM_APP_SOURCES
    audioMemoryTest.c
)

set_legato_component(${MEM_APP_COMPONENT})
add_legato_internal_executable(${MEM_APP_TARGET} ${MEM_APP_SOURCES})

target_link_libraries(${MEM_APP_TARGET}
    ${LEGATO_SERVICES_AUDIO_TARGET}
)

add_test(${MEM_APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${MEM_APP_TARGET} 1)
set_tests_properties(${MEM_APP_TARGET} PROPERTIES TIMEOUT 0)
