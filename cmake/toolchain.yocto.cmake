#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(COMPILER_FOUND FALSE)

if(NOT DEFINED TOOLCHAIN_PREFIX)
    if(DEFINED ENV{TOOLCHAIN_PREFIX})
        set(TOOLCHAIN_PREFIX "$ENV{TOOLCHAIN_PREFIX}")
    else()
        set(TOOLCHAIN_PREFIX "arm-poky-linux-gnueabi-")
    endif()
endif()

# Look for Cross-Compiler in CC
if(DEFINED ENV{CC})

    if($ENV{CC} MATCHES "${TOOLCHAIN_PREFIX}gcc")
        include(CMakeForceCompiler)
        set(CMAKE_FORCE_C_COMPILER $ENV{CC})
        set(CMAKE_C_COMPILER_FORCED TRUE)

        set(COMPILER_FOUND TRUE)
    endif()
endif()

# Look for Cross-Compiler in PATH
if(NOT COMPILER_FOUND)
    find_program(CROSS_COMPILER
        NAMES "${TOOLCHAIN_PREFIX}gcc")

    if(CROSS_COMPILER)
        set(CMAKE_C_COMPILER ${CROSS_COMPILER})
        set(COMPILER_FOUND TRUE)
    endif()
endif()

if(NOT COMPILER_FOUND)
    message(FATAL_ERROR "Unable to find C compiler")
endif()