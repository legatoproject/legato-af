#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(APP_COMPONENT threadTest)
set(APP_TARGET testFwThread)
set(APP_SOURCES
    main.c
    forkJoinMutex.c
    externalThreadApi.c
    priority.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

add_dependencies(tests_c ${APP_TARGET})

set(APP_TARGET testFwThreadLimit)
set(APP_SOURCES
    limit.c
)

add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(${APP_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/limit.sh ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})

# This is a C test
add_dependencies(tests_c ${APP_TARGET})
