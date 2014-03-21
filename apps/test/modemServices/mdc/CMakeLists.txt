#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT modemServicesTestMdc)
set(APP_TARGET testModemMdc)
set(APP_SOURCES
    mdcTest.c
)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")
    include_directories(${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS})
endif()

set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET}
    ${LEGATO_SERVICES_MODEM_TARGET}
)

if(LEGATO_COMPONENTS_MODEM MATCHES "AT")
    target_link_libraries(${APP_TARGET} ${LEGATO_COMPONENTS_AT_MANAGER_TARGET})
endif()

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
endif()

# Extra settings for AR7
if(${LEGATO_TARGET} STREQUAL "ar7")

    # Define some test specific variables
    set(TEST_DIR /mnt/userrw/testMDC)
    set(TEST_SCRIPT testMDC.sh)
    set(TEST_PROGRAM ${APP_TARGET})
    set(CONFIG_FILE configMDC.txt)

    # The rest is generic
    set(TEST_EXEC_LIST 
        ${TEST_SCRIPT}
        ${TEST_PROGRAM})

    # These go into the "tests" directory, with all the other executables
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT}.in
                   ${EXECUTABLE_OUTPUT_PATH}/${TEST_SCRIPT})

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_FILE}.in
               ${EXECUTABLE_OUTPUT_PATH}/${CONFIG_FILE})

endif()

