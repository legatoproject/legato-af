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

# Extend the timeout as this test can take some time to collect enough samples
# In most PCs this should run quite fast, but if there is a lack of entropy
# (VM for instance), install haveged to improve the performance.
set_tests_properties(${APP_TARGET} PROPERTIES TIMEOUT 600)

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
