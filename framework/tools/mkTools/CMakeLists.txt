#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

cmake_minimum_required(VERSION 2.8)

# Clang
if(USE_CLANG EQUAL 1)
    set(CMAKE_C_COMPILER      "clang")
    set(CMAKE_CXX_COMPILER    "clang++")
endif()

project(LEGATO_BUILD_TOOLS C CXX)

set(LEGATO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Version
file(READ ${LEGATO_ROOT}/version LEGATO_VERSION)
string(REPLACE "\n" "" LEGATO_VERSION ${LEGATO_VERSION})
message(STATUS "Version: ${LEGATO_VERSION}")
add_definitions(-DLEGATO_VERSION=\"${LEGATO_VERSION}\")

add_definitions(-Wall -Werror)
set(CMAKE_CXX_FLAGS -std=c++0x)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c/src)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Generate Precompiled header including parse tree and conceptual model headers to speed up compilation.
set(PRECOMPILED_HEADER ${CMAKE_BINARY_DIR}/mkTools.h.gch)
add_custom_command( OUTPUT ${PRECOMPILED_HEADER}
                    COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS}
                            -std=c++0x
                            -g
                            -o ${PRECOMPILED_HEADER}
                            ${CMAKE_CURRENT_SOURCE_DIR}/mkTools.h
                    DEPENDS parseTree/*.h
                            parser/*.h
                            conceptualModel/*.h
                            modeller/*.h
                            buildScriptGenerator/*.h
                            codeGenerator/*.h
                            configGenerator/*.h
                            avManifestGenerator/*.h
                            *.h
                   )
add_custom_target(  PrecompiledHeader
                    DEPENDS ${PRECOMPILED_HEADER}
                 )

# Make everything include the precompiled header when it is compiled.
add_definitions(-include ${CMAKE_BINARY_DIR}/mkTools.h)

# Create the mkTools library containing utility code that gets shared by multiple modules.
add_library(mkTools
                exception.cpp
                envVars.cpp
                path.cpp
                file.cpp
                buildParams.cpp
                md5.cpp
           )

add_dependencies(mkTools
                    PrecompiledHeader
                )

add_subdirectory(parser)
add_subdirectory(parseTree)
add_subdirectory(conceptualModel)
add_subdirectory(modeller)
add_subdirectory(buildScriptGenerator)
add_subdirectory(codeGenerator)
add_subdirectory(configGenerator)
add_subdirectory(avManifestGenerator)
add_subdirectory(commandLineInterpreter)
