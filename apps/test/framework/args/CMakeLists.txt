#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT argsTest)
set(APP_TARGET testFwArgs)
set(APP_SOURCES
    argTest.c
)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})

add_test(
    NAME ${APP_TARGET}
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/${APP_TARGET} param1 "param 2" param 3
)
