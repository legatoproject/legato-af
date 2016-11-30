#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

find_package(CUnit REQUIRED)

set(APP_TARGET testFwClock)

mkexe(  ${APP_TARGET}
            main.c
            -i ${CUNIT_INSTALL}/include
            ${CUNIT_LIBRARIES}
        )

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
