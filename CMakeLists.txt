#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

cmake_minimum_required(VERSION 2.8)

# Variables changing internal behavior:
# - LEGATO_TARGET: Target to build (default to 'localhost')
#
# Advanced:
# - TEST_COVERAGE: Provide support for GCov

# Target Platform
if(NOT DEFINED LEGATO_TARGET)
    if(NOT "$ENV{LEGATO_TARGET}" STREQUAL "")
        message("Legato target found in environement: $ENV{LEGATO_TARGET}")
        set(LEGATO_TARGET $ENV{LEGATO_TARGET})
    else()
        message("Legato Target is not selected, default to localhost")
        set(LEGATO_TARGET "localhost" CACHE STRING "Defines global target of the Legato library")
    endif()
endif()

message("Building Legato for target ${LEGATO_TARGET}")

# Import platform adapter environment variables.
set(LEGATO_AUDIO_PA $ENV{LEGATO_AUDIO_PA})
set(LEGATO_MODEM_PA $ENV{LEGATO_MODEM_PA})
set(LEGATO_GNSS_PA $ENV{LEGATO_GNSS_PA})

# Default Target configuration
set(LEGATO_COMPONENTS_MODEM "AT" CACHE STRING "Legato Modem Platform-Adapter to build")
set(LEGATO_COMPONENTS_GNSS "AT" CACHE STRING "Legato Gnss Platform-Adapter to build")
set(LEGATO_COMPONENTS_AUDIO "STUB" CACHE STRING "Legato Audio Platform-Adapter to build")

# Target: ar6
if(LEGATO_TARGET MATCHES "ar6")
    set(LEGATO_EMBEDDED "true")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.yocto.cmake")
    add_definitions(-DLEGATO_EMBEDDED)
    add_definitions(-DLEGATO_DEBUG)

# Target: ar7
elseif(LEGATO_TARGET MATCHES "ar7")
    set(LEGATO_EMBEDDED "true")
    set(AUTOMOTIVE_TARGET "true")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.yocto.cmake")
    set(LEGATO_COMPONENTS_MODEM "QMI")
    set(LEGATO_COMPONENTS_GNSS "QMI")
    set(LEGATO_COMPONENTS_AUDIO "ar7")
    add_definitions(-DLEGATO_EMBEDDED)
    add_definitions(-DLEGATO_DEBUG)
    add_definitions(-DAUTOMOTIVE_TARGET)

# Target: wp7
elseif(LEGATO_TARGET MATCHES "wp7")
    set(LEGATO_EMBEDDED "true")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.yocto.cmake")
    set(LEGATO_COMPONENTS_MODEM "QMI")
    set(LEGATO_COMPONENTS_GNSS "QMI")
    set(LEGATO_COMPONENTS_AUDIO "wp7")
    add_definitions(-DLEGATO_EMBEDDED)
    add_definitions(-DLEGATO_DEBUG)

# Target: raspi
elseif(LEGATO_TARGET MATCHES "raspi")
    set(LEGATO_EMBEDDED "true")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.yocto.cmake")

# Target: localhost
elseif(LEGATO_TARGET MATCHES "localhost")
    add_definitions(-DLEGATO_DEBUG)

# Target: Unknown
else()
    message(FATAL_ERROR "Invalid Target")
endif()

project(LEGATO C CXX)

# CMake Extension
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include("${CMAKE_MODULE_PATH}/Helpers.cmake")
set(Legato_DIR ${PROJECT_SOURCE_DIR}/cmake)

## Output
set(EXECUTABLE_OUTPUT_PATH      ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH         ${PROJECT_BINARY_DIR}/bin/lib)
set(APP_OUTPUT_PATH             ${PROJECT_BINARY_DIR}/bin/apps)
set(DOCUMENTATION_OUTPUT_PATH   ${PROJECT_BINARY_DIR}/bin/doc)
set(READYAGENT_OUTPUT_PATH      ${PROJECT_BINARY_DIR}/bin/readyagent)

## 3rd Party
set(CUnit_DIR ${PROJECT_SOURCE_DIR}/3rdParty/CUnit/)

## Configuration

# Global definitions
find_package(Legato REQUIRED)

# Version
file(READ version LEGATO_VERSION)
string(REPLACE "\n" "" LEGATO_VERSION ${LEGATO_VERSION})
message(STATUS "Version: ${LEGATO_VERSION}")
add_definitions(-DLEGATO_VERSION=\"${LEGATO_VERSION}\")

# Debug/Release
if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-O3)
else()
    add_definitions(-g -O0)
endif()

# All warnings and treat warnings as errors.
add_definitions(-Wall -Werror)

# Coverage
if(TEST_COVERAGE)
    message(STATUS "Coverage reporting (TEST_COVERAGE): Enabled")
    message(STATUS "=> use make coverage_report to produce report")

    # Building part
    add_definitions(--coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage")

    # Generation part
    add_custom_target(
        coverage_report
        COMMAND lcov --capture --output-file coverage.info -d .
        COMMAND mkdir -p coverage_out
        COMMAND genhtml -o coverage_out coverage.info
        COMMAND echo
        COMMAND echo \"Report in ${LEGATO_BINARY_DIR}/coverage_out/\"
        COMMENT "Coverage report will be available in ${LEGATO_BINARY_DIR}/coverage_out/"
        WORKING_DIRECTORY ${LEGATO_BINARY_DIR}
    )

else()
    message(STATUS "Coverage reporting (TEST_COVERAGE): Disabled")
endif()

set(LEGATO_COMPONENTS_MODEM_TARGET          le_pa)
set(LEGATO_COMPONENTS_GNSS_TARGET           le_pa_gnss)

## Pre-compiled targets
if(${LEGATO_COMPONENTS_MODEM} MATCHES "QMI" AND NOT EXISTS ${LEGATO_COMPONENTS_MODEM_DIR}/qmi/src)
    add_library(${LEGATO_COMPONENTS_MODEM_TARGET} SHARED IMPORTED GLOBAL)
endif()
if(${LEGATO_COMPONENTS_GNSS} MATCHES "QMI" AND NOT EXISTS ${LEGATO_COMPONENTS_GNSS_DIR}/qmi/src)
    add_library(${LEGATO_COMPONENTS_GNSS_TARGET} SHARED IMPORTED GLOBAL)
endif()

## Paths to AT Manager include files needed by various platform adapters that use it.
### TODO: Move the Platform Adapters together under their own subdirectory.
set (LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS
        modemServices/platformAdaptor/at/atManager/inc
        modemServices/platformAdaptor/at/atManager/devices/adapter_layer/inc
        modemServices/platformAdaptor/at/atManager/devices/uart/inc
        )

## Building

# Build Documentation
add_subdirectory(interfaces)
add_subdirectory(framework/doc)

# Build 3rd party libs.
add_subdirectory(3rdParty/jansson-2.6)

# Build C Framework
add_subdirectory(framework/c)
add_subdirectory(framework/tools)

# Build Components
add_subdirectory(components)

# Build Platform Services (modem, positioning, audio, etc. services).
add_subdirectory(platformServices)

# Build Applications
add_subdirectory(apps)

## Testing

# Tests with ctest
enable_testing()
