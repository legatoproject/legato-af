#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT positioningTest)
set(APP_TARGET testPositioning)

file(GLOB APP_SOURCES "${LEGATO_SOURCE_DIR}/apps/positioning/src/*.c")

set(APP_SOURCES
    ${APP_SOURCES}
    le_posTest.c
    main.c
)

include_directories(
    ${CUNIT_INCLUDE_DIRS}
)

if(LEGATO_COMPONENTS_GNSS MATCHES "AT")
    include_directories(
        ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
        ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/src
    )
endif()

if(DEFINED ENABLE_SIMUL)
    add_definitions(-DENABLE_SIMUL=1)
endif()

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${CUNIT_LIBRARIES}
    ${LEGATO_SERVICES_POSITIONING_TARGET}
    ${LEGATO_SERVICES_POSSER_CLIENT_TARGET}
)

if(LEGATO_COMPONENTS_GNSS MATCHES "AT")
    target_link_libraries(${APP_TARGET}
        ${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
    )
endif()

if(${LEGATO_TARGET} STREQUAL "localhost")
    set(TEST_APP_PATH     ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    if(DEFINED ENABLE_SIMUL)
        message("Positioning test uses simulator.")

        set(SCRIPT_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/gnssSimulator.sh)
        set(STUB_PATH         ${LEGATO_SOURCE_DIR}/apps/stub/modem/stub_gnss_nmea.pl)
        set(TEST_XML          ${CMAKE_CURRENT_SOURCE_DIR}/modem_gnss.xml)
        set(TEST_GNSS         ${CMAKE_CURRENT_SOURCE_DIR}/gnss_nmea.txt)
        set(TEST_GNSS_BLOCK   5)
        set(TEST_SOCKET       "/tmp/modem_gnss")

        add_test(${APP_TARGET}
        ${SCRIPT_PATH} ${STUB_PATH} ${TEST_APP_PATH} ${TEST_XML} ${TEST_GNSS} ${TEST_GNSS_BLOCK} ${TEST_SOCKET}
        )
        set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 120)
    endif()
endif()


set(APP_TARGET testPosService)

mkexe(  ${APP_TARGET}
            posServiceTest
            -i ${LEGATO_ROOT}/interfaces/posService
        DEPENDS
            legato
            posServiceTest/Component.cdef
            posServiceTest/posServiceTest.c
    )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
