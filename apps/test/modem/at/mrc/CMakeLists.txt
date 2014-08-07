#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

if(${LEGATO_TARGET} STREQUAL "localhost")

    if(NOT DEFINED DISABLE_SIMUL)

        include_directories(
            ${CUNIT_INCLUDE_DIRS}
            ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
            ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/src
            ${LEGATO_COMPONENTS_MODEM_AT_SRC_DIR}
        )

        set(APP_COMPONENT atmrctest)
        set(APP_TARGET testPaAtMrc)
        set(APP_SOURCES
        test_at_mrc.c
        )

        set_legato_component(${APP_COMPONENT})
        add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

        target_link_libraries(${APP_TARGET}
        ${LEGATO_COMPONENTS_MODEM_TARGET}
        ${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
        pthread
        ${CUNIT_LIBRARIES}
        )

        set(SCRIPT_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/../testLeAT.sh)
        set(STUB_PATH         ${LEGATO_SOURCE_DIR}/apps/stub/modem/stub_modem.pl)
        set(TEST_APP_PATH     ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
        set(TEST_XML          ${CMAKE_CURRENT_SOURCE_DIR}/modem_mrc.xml)
        set(TEST_SOCKET       "/tmp/modem_mrc")

# TODO: verify simulator since this test often fails with a timeout
#        add_test(${APP_TARGET}
#                 ${SCRIPT_PATH} ${STUB_PATH} ${TEST_APP_PATH} ${TEST_XML} ${TEST_SOCKET}
#        )
#
#        set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 10)

    endif()

endif()
