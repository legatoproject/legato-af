#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

set(APP_COMPONENT leatgnsstest)
set(APP_TARGET testGnss)
set(APP_SOURCES
    test_gnss.c
)

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
    ${LEGATO_COMPONENTS_GNSS_TARGET}
    pthread
)

set(SCRIPT_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/testLeAT.sh)
set(STUB_PATH         ${LEGATO_ROOT}/apps/stub/modem/stub_gnss_nmea.pl)
set(TEST_APP_PATH     ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
set(TEST_XML          ${CMAKE_CURRENT_SOURCE_DIR}/modem_gnss.xml)
set(TEST_GNSS         ${CMAKE_CURRENT_SOURCE_DIR}/gnss_nmea.txt)
set(TEST_GNSS_BLOCK   4)
set(TEST_SOCKET       "/tmp/modem_gnss")

if(${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET}
    ${SCRIPT_PATH} ${STUB_PATH} ${TEST_APP_PATH} ${TEST_XML} ${TEST_GNSS} ${TEST_GNSS_BLOCK} ${TEST_SOCKET}
    )

    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 60)
endif()
