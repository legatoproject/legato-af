#*******************************************************************************
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_TARGET testFwSemaphore)

mkexe(  ${APP_TARGET}
            test_le_semaphore.c
            -i ${CUNIT_INSTALL}/include
            ${CUNIT_LIBRARIES}
        DEPENDS
            test_le_semaphore.c
        )

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
