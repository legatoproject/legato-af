#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT MCCTest)
set(APP_TARGET testModemMcc)

file(GLOB APP_SOURCES "${LEGATO_SOURCE_DIR}/apps/modemServices/src/*.c")

set(APP_SOURCES
    ${APP_SOURCES}
    le_mccTest.c
    main.c
)

include_directories(${CUNIT_INCLUDE_DIRS})

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${CUNIT_LIBRARIES}
    ${LEGATO_SERVICES_MODEM_TARGET}
    ${LEGATO_AUDIO_TARGET}
)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

