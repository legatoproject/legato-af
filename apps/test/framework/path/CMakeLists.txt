#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_COMPONENT pathTest)
set(APP_TARGET testFwPath)
set(APP_SOURCES
    pathTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
