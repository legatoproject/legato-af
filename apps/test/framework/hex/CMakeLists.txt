#*******************************************************************************
#
# Copyright (C) 2013, Sierra Wireless, Inc., all rights reserved.
#
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testFwHex)

mkexe(  ${APP_TARGET}
            test_hex.c
            -i ${CUNIT_INSTALL}/include
            ${CUNIT_LIBRARIES}
        DEPENDS
            test_hex.c
        )

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
