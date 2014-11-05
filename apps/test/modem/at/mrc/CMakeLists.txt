#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

if(${LEGATO_TARGET} STREQUAL "localhost")

    if(NOT DEFINED DISABLE_SIMUL)

        message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

        set(APP_TARGET testPaAtMrc)

        mkexe(${APP_TARGET}
                    .
                    ${CUNIT_LIBRARIES}
                    -i ${CUNIT_INSTALL}/include
                    -i ${CUNIT_INSTALL}/include/CUnit
                    -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
                    -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/at/le_pa
                    -i ${LEGATO_ROOT}/components/atManager/inc
                    -i ${LEGATO_ROOT}/components/atManager/devices/adapter_layer/inc
                    -i ${LEGATO_ROOT}/components/atManager/devices/uart/inc
                    -i ${LEGATO_ROOT}/components/atManager/src
                    -i ${LEGATO_ROOT}/components
                DEPENDS
                    legato
                    Component.cdef
                    test_at_mrc.c
                 )

        set(SCRIPT_PATH       ${LEGATO_ROOT}/apps/test/modem/at/testLeAT.sh)
        set(STUB_PATH         ${LEGATO_ROOT}/apps/stub/modem/stub_modem.pl)
        set(TEST_APP_PATH     ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
        set(TEST_XML          ${CMAKE_CURRENT_SOURCE_DIR}/modem_mrc.xml)
        set(TEST_SOCKET       "/tmp/modem_mrc")

        add_test(${APP_TARGET}
                 ${SCRIPT_PATH} ${STUB_PATH} ${TEST_APP_PATH} ${TEST_XML} ${TEST_SOCKET}
        )

        set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 30)

    endif()

endif()
