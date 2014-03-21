#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LOGTOOL C)

find_package(Legato REQUIRED)

# Set the output file name
set(APP_TARGET "log")
set(APP_COMPONENT "log")

# Compile
set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET} logTool.c)
