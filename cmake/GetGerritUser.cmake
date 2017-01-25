#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Found Gerrit user from existing repository
macro(get_gerrit_user GIT_SOURCE_DIR GERRIT_USER)
    unset(GERRIT_USER)

    find_package(Git)
     if(GIT_FOUND)
        set(ENV{GIT_DIR} ${GIT_SOURCE_DIR}/.git)
        execute_process(COMMAND git remote -v OUTPUT_VARIABLE GIT_REMOTES_TMP)
        string(REGEX MATCH "([a-z]*)@[^:]+:29418" MATCH_TMP ${GIT_REMOTES_TMP})
        string(REGEX REPLACE "([a-z]*)@[^:]+:29418" "\\1" USER_TMP  ${MATCH_TMP})
        if(DEFINED USER_TMP)
            set(GERRIT_USER ${USER_TMP})
            message(STATUS "Gerrit User: ${USER_TMP}")
        endif()

        unset(ENV{GIT_DIR})
    endif(GIT_FOUND)

endmacro()