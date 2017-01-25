#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

macro(get_compiler_target GCC_COMPILER TARGET_HOST)

    # Launch compiler without file
    execute_process(
        COMMAND ${GCC_COMPILER} --verbose
        ERROR_VARIABLE GCC_VERSION
        )

    # Select target part
    string(REGEX MATCH "Target: ([^\n]+)" TARGET_LINE ${GCC_VERSION})
    string(REGEX REPLACE "Target: (.+)" "\\1" TARGET_HOST ${TARGET_LINE})
endmacro()

macro(get_compiler_target_sysroot GCC_COMPILER TARGET_SYSROOT)

    # Launch compiler
    execute_process(
        COMMAND ${GCC_COMPILER} --print-sysroot
        OUTPUT_VARIABLE SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if(SYSROOT STREQUAL "")
        set(TARGET_SYSROOT "/")
    else()
        set(TARGET_SYSROOT "${SYSROOT}")
    endif()

    message(STATUS "Sysroot: ${SYSROOT}")

endmacro()
