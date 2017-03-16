#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

find_package(CUnit REQUIRED)

set(APP_TARGET testFwClock)

mkexe(  ${APP_TARGET}
            main.c
            -i ${CUNIT_INSTALL}/include
            ${CUNIT_LIBRARIES}
        )

add_dependencies(${APP_TARGET} cunit)
add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
