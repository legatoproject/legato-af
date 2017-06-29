#--------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
#--------------------------------------------------------------------------------------------------

set(APP_COMPONENT randTest)
set(APP_TARGET testFwRand)
set(APP_SOURCES
    randTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(
    NAME ${APP_TARGET}
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET}
)

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
