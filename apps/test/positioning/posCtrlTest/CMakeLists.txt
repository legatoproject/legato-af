#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

if(NOT ${LEGATO_TARGET} STREQUAL "localhost")

    set(APP_TARGET testPosCtrl)

    mkexe(  ${APP_TARGET}
                .
                -i ${LEGATO_ROOT}/interfaces/positioning
            DEPENDS
                legato
                Component.cdef
                posCtrlTest.c
        )

    add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
    set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 0)
endif()
