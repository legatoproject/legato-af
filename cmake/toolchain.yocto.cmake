#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# This script requires TARGET_CC to be set in the environment.
# It must contain the path to the C cross compiler.

# Optionally, the target sysroot can be set using the LEGATO_SYSROOT
# environment variable.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

if(NOT DEFINED ENV{TARGET_CC})
    message(FATAL_ERROR "TARGET_CC not set")
endif()

set(TARGET_CC "$ENV{TARGET_CC}")

if(NOT DEFINED ENV{TARGET_CXX})
    message(FATAL_ERROR "TARGET_CXX not set")
endif()

set(TARGET_CXX "$ENV{TARGET_CXX}")

if(NOT DEFINED ENV{TARGET_CPP})
    message(FATAL_ERROR "TARGET_CPP not set")
endif()

set(TARGET_CPP "$ENV{TARGET_CPP}")

if(NOT DEFINED USE_CLANG)
    set(USE_CLANG $ENV{USE_CLANG})
endif()

message("Target compilers: ${TARGET_CC} ${TARGET_CXX}")

# Check for sysroot override via LEGATO_SYSROOT environment variable.
if (DEFINED ENV{LEGATO_SYSROOT})
    set(LEGATO_SYSROOT "$ENV{LEGATO_SYSROOT}")
else()
    # Ask the compiler for its sysroot.
    if(NOT USE_CLANG EQUAL 1)
        execute_process(
            COMMAND ${TARGET_CC} --print-sysroot
            OUTPUT_VARIABLE LEGATO_SYSROOT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

message("Target sysroot: ${LEGATO_SYSROOT}")

set(CMAKE_C_COMPILER "${TARGET_CC}")
set(CMAKE_CXX_COMPILER "${TARGET_CXX}")
set(CMAKE_CPP_COMPILER "${TARGET_CPP}")

if (DEFINED ENV{CCACHE})
    set(CMAKE_C_COMPILER_LAUNCHER "$ENV{CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "$ENV{CCACHE}")
endif()

SET(CMAKE_REQUIRED_FLAGS "--sysroot=${LEGATO_SYSROOT}")
add_definitions(--sysroot=${LEGATO_SYSROOT})
SET(CMAKE_EXE_LINKER_FLAGS "--sysroot=${LEGATO_SYSROOT}" CACHE STRING "" FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS "--sysroot=${LEGATO_SYSROOT}" CACHE STRING "" FORCE)
SET(CMAKE_MODULE_LINKER_FLAGS "--sysroot=${LEGATO_SYSROOT}" CACHE STRING "" FORCE)

set(CMAKE_FIND_ROOT_PATH "${LEGATO_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

