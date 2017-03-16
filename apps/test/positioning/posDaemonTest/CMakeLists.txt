#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

find_package(CUnit REQUIRED)

set(APP_TARGET testPosDaemon)

if(DEFINED ENABLE_SIMUL)
    add_definitions(-DENABLE_SIMUL=1)
endif()

if(${LEGATO_TARGET} STREQUAL "localhost")
    if(LEGATO_COMPONENTS_GNSS MATCHES "AT")
        set(TEST_APP_PATH     ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
        if(DEFINED ENABLE_SIMUL)
            message("Positioning test uses simulator.")

            set(SCRIPT_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/gnssSimulator.sh)
            set(STUB_PATH         ${CMAKE_CURRENT_SOURCE_DIR}/stub_gnss_nmea.pl)
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
endif()

mkexe(  ${APP_TARGET}
            .
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/positioning
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/positioning/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components
    )

add_dependencies(${APP_TARGET} cunit)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
