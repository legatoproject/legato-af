#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

cmake_minimum_required(VERSION 2.8)

project(LEGATO_BUILD_TOOLS C CXX)


add_definitions(-Wall -Werror)
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(Parser)
add_subdirectory(ComponentModel)
add_subdirectory(mk)

