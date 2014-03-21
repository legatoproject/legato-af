#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved. 
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

macro(get_git_revision GIT_REVISION)
    set(GIT_REVISION "Unknown")
    
    if(EXISTS "${PROJECT_SOURCE_DIR}/.git/")
        find_package(Git)
        if(GIT_FOUND)
            set(ENV{GIT_DIR} ${PROJECT_SOURCE_DIR}/.git)
            execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_REVISION_TMP)
            string(REPLACE "\n" "" ${GIT_REVISION} ${GIT_REVISION_TMP})
            unset(ENV{GIT_DIR})
        endif(GIT_FOUND)
    endif()
    
endmacro()