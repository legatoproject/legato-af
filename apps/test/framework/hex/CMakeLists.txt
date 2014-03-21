#*******************************************************************************
#
# Copyright (C) 2013, Sierra Wireless, Inc., all rights reserved.
#
#*******************************************************************************

find_package(Legato REQUIRED)
find_package(CUnit REQUIRED)

set(APP_COMPONENT hexTest)
set(APP_TARGET testFwHex)
set(APP_SOURCES
    test_hex.c
)

include_directories(${CUNIT_INCLUDE_DIRS})

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

target_link_libraries(${APP_TARGET} ${CUNIT_LIBRARIES})

add_test(${APP_TARGET} ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET})
