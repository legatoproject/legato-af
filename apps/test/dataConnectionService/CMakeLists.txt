#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET testDcs)

mkexe(${APP_TARGET}
            dcsTest
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            dcsTest/Component.cdef
            dcsTest/dcsTest.c
            )

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")
    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()

