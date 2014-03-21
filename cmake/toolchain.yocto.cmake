#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(COMPILER_FOUND FALSE)

# Look for Cross-Compiler in CC
if(DEFINED ENV{CC})
    
    if($ENV{CC} MATCHES "arm-poky-linux-gnueabi-gcc")
        include(CMakeForceCompiler)
        set(CMAKE_FORCE_C_COMPILER $ENV{CC})
        set(CMAKE_C_COMPILER_FORCED TRUE)
        
        set(COMPILER_FOUND TRUE)
    endif()
endif()

# Look for Cross-Compiler in PATH
if(NOT COMPILER_FOUND)
    find_program(ARM_COMPILER
        NAMES arm-poky-linux-gnueabi-gcc arm-none-linux-gnueabi-gcc)

    if(ARM_COMPILER)
        set(CMAKE_C_COMPILER ${ARM_COMPILER})
        set(COMPILER_FOUND TRUE)
    endif()
endif()

if(NOT COMPILER_FOUND)
    message(FATAL_ERROR "Unable to find ARM compiler")
endif()