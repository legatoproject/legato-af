#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
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
            set(STUB_PATH         ${LEGATO_ROOT}/apps/stub/modem/stub_gnss_nmea.pl)
            set(TEST_XML          ${CMAKE_CURRENT_SOURCE_DIR}/modem_gnss.xml)
            set(TEST_GNSS         ${CMAKE_CURRENT_SOURCE_DIR}/gnss_nmea.txt)
            set(TEST_GNSS_BLOCK   5)
            set(TEST_SOCKET       "/tmp/modem_gnss")

            add_test(${APP_TARGET}
                ${SCRIPT_PATH} ${STUB_PATH} ${TEST_APP_PATH} ${TEST_XML} ${TEST_GNSS} ${TEST_GNSS_BLOCK} ${TEST_SOCKET}
            )
            set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 120)

            set(PA_FILES    ${LEGATO_GNSS_PA}/gnss_at.c
                            ${LEGATO_GNSS_PA}/Component.cdef
                        )
        endif()
    endif()
endif()

if(LEGATO_COMPONENTS_GNSS MATCHES "QMI")
    set(INCLUDE_DIRECTORIES -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
                            -i ${LEGATO_ROOT}/proprietary/qct/inc/services
                            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/qmi/le_pa
        )

    set (LINK_LIBRARIES
                            --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
        )

    set (FLAGS
                            --cflags="-DSIERRA"
        )

elseif(LEGATO_COMPONENTS_GNSS MATCHES "simu")

    set(INCLUDE_DIRECTORIES     -i ${LEGATO_ROOT}/components/
                                -i ${LEGATO_ROOT}/components/modemServices/modemDaemon/
        )
endif()

mkexe(  ${APP_TARGET}
            .
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/positioning
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/positioning/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components
            ${INCLUDE_DIRECTORIES}
            ${LINK_LIBRARIES}
            ${FLAGS}
        DEPENDS
            legato
            le_posTest.c
            main.c
            Component.cdef
            ${PA_FILES}
    )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

