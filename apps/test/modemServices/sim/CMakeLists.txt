#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT simTest)
set(APP_TARGET testModemSim)

set(APP_SOURCES
    ${APP_SOURCES}
    le_simTest.c
    main.c
)

include_directories(${CUNIT_INCLUDE_DIRS})

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${CUNIT_LIBRARIES}
    ${LEGATO_SERVICES_MODEM_TARGET}
    pthread
)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
