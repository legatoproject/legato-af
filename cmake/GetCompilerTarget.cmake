#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
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