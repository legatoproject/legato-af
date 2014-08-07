#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

cmake_minimum_required(VERSION 2.8)

project(LEGATO_BUILD_TOOLS C CXX)

# Version
file(READ ${LEGATO_BUILD_TOOLS_SOURCE_DIR}/../version LEGATO_VERSION)
string(REPLACE "\n" "" LEGATO_VERSION ${LEGATO_VERSION})
message(STATUS "Version: ${LEGATO_VERSION}")
add_definitions(-DLEGATO_VERSION=\"${LEGATO_VERSION}\")

add_definitions(-Wall -Werror)
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x -g")

include_directories(${LEGATO_BUILD_TOOLS_SOURCE_DIR}/../framework/c/src)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(Parser)
add_subdirectory(ComponentModel)
add_subdirectory(mk)

