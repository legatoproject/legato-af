#*******************************************************************************
# Copyright (C) 2013-2014, Sierra Wireless Inc.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testModemMcc)

message("CUNIT_INCLUDE_DIRS: ${CUNIT_INCLUDE_DIRS}")

mkexe(${APP_TARGET}
            mccTest
            ${CUNIT_LIBRARIES}
            -i ${CUNIT_INSTALL}/include
            -i ${CUNIT_INSTALL}/include/CUnit
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            mccTest/Component.cdef
            mccTest/main.c
            mccTest/main.h
            mccTest/le_mccTest.c
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
