#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemSim)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

mkexe(${APP_TARGET}
            simTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            simTest/Component.cdef
            simTest/main.c
            simTest/main.h
            simTest/le_simTest.c
            ${LEGATO_ROOT}/interfaces/modemServices/le_mdmDefs.api
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()