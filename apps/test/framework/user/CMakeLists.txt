#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_COMPONENT userTest)
set(APP_TARGET testFwUsers)
set(APP_SOURCES
    userTest.c
)

include_directories(${PROJECT_SOURCE_DIR}/framework/c/src)

set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET} ${APP_SOURCES})
