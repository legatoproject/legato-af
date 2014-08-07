#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#   http://www.eclipse.org/legal/epl-v10.html
# The Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

include(CMakeParseArguments)

# Build a Lua library either from C files or Lua files.
# When Lua files are given they are actually only copied
# add_lua_library(libaryname [DESTINATION destdir] [EXCLUDE_FROM_ALL] filelist...)
# note: for Lua files only: you can prepend the filename with char '=' to force
#       the tree structure. example:
#           add_lua_library(totolib somedir/file.lua) => will copy file.lua into lua/file.lua (lua is the output directory)
#           add_lua_library(totolib =somedir/file.lua) => will copy file.lua into lua/somedir/file.lua

function(add_lua_library name)

    set(options EXCLUDE_FROM_ALL)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs)
    cmake_parse_arguments(LUALIBRARY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(filelist ${LUALIBRARY_UNPARSED_ARGUMENTS})

    if (LUALIBRARY_EXCLUDE_FROM_ALL)
        unset(all)
        set(exclude EXCLUDE_FROM_ALL)
    else ()
        unset(exclude)
        set(all "ALL")
    endif ()

    # Regroup C files versus Lua files
    foreach(f ${filelist})

        if((${f} MATCHES ".c$") OR (${f} MATCHES ".h$") OR (${f} MATCHES ".cpp$"))
            # C file, populate the list
            set(cfilelist ${cfilelist} ${f})

        else()
            # Lua file, they should only be copied
            # Compute the right output path
            if (${f} MATCHES "^=") # file path starting with '=' will be copied with the given added path
                string(LENGTH ${f} l)
                math(EXPR  l ${l}-1)
                string(SUBSTRING ${f} 1 ${l} f)
                set(of ${f})
            else() # otherwise the file is striped from its path
                get_filename_component(of ${f} NAME)
            endif()

            # Full qualified input file name
            set(if "${CMAKE_CURRENT_SOURCE_DIR}/${f}")

            # Additional DESTINATION subdirectory
            if (LUALIBRARY_DESTINATION)
                if(${LUALIBRARY_DESTINATION} MATCHES "^=")
                    string(LENGTH ${LUALIBRARY_DESTINATION} l)
                    math(EXPR  l ${l}-1)
                    string(SUBSTRING ${LUALIBRARY_DESTINATION} 1 ${l} LUALIBRARY_DESTINATION)
                    set(of "${LUALIBRARY_DESTINATION}")
                else()
                    set(of "${LUALIBRARY_DESTINATION}/${of}")
                endif()
            endif()

            # Full qualified output file name
            set(of "${CMAKE_LUA_LIBRARY_OUTPUT_DIRECTORY}/${of}")

            add_custom_command(
                OUTPUT     ${of}
                COMMAND    ${CMAKE_COMMAND} -E copy_if_different ${if} ${of}
                DEPENDS    ${if}
            )
            set(luaouputfilelist ${luaouputfilelist} ${of})
            set(luafilelist ${luafilelist} ${f})

        endif()

    endforeach(f)

    if (NOT cfilelist)
        add_custom_target(
            ${name} ${all}
            DEPENDS ${luaouputfilelist}
        )

    else()
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LUA_LIBRARY_OUTPUT_DIRECTORY}/${LUALIBRARY_DESTINATION}/")
        set(CMAKE_SHARED_MODULE_PREFIX "")
        add_library(
            ${name} ${exclude}
            MODULE ${cfilelist}
        )

        if (luafilelist)
            add_custom_target(
                "${name}_lua" ${all}
                DEPENDS ${luaouputfilelist}
                SOURCES ${luafilelist}
            )
            add_dependencies(${name} "${name}_lua")
        endif(luafilelist)


    endif()


endfunction(add_lua_library)
