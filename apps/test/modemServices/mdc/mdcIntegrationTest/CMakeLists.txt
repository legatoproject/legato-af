#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemMdc)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

mkexe(${APP_TARGET}
            mdcTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            mdcTest/Component.cdef
            mdcTest/mdcTest.c
            )

# Extra settings for AR7
if(${LEGATO_TARGET} STREQUAL "ar7")

    # Define some test specific variables
    set(TEST_DIR /mnt/userrw/testMDC)
    set(TEST_SCRIPT testMDC.sh)
    set(TEST_PROGRAM mdcTest/${APP_TARGET})
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

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
