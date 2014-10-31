#*******************************************************************************
# Copyright (C) 2013-2014, Sierra Wireless Inc.
#
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testModemInfo)

mkexe(${APP_TARGET}
            infoTest
            -i ${LEGATO_ROOT}/interfaces/modemServices
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            infoTest/Component.cdef
            infoTest/infoTest.c
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
