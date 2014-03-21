#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Build the Audio Daemon

set(AUDIO_DAEMON audioDaemon)
set_legato_component(${AUDIO_DAEMON})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# Set path for .api files and associated .h files
set(API_FILE_DIR ${LEGATO_SOURCE_DIR}/interfaces/audio/api)
set(API_HDR_DIR  ${LEGATO_SOURCE_DIR}/interfaces/audio/c)

# todo: These add_custom_command() definitions could probably be made a little
#       nicer by adding a CMake function.  Consider this for later.

add_custom_command (
    OUTPUT ${API_HDR_DIR}/audio_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/audio_service.api
                          --gen-interface
                          --user-include audioUserInclude.h
                          --name-prefix le_audio_
                          --file-prefix audio_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/audio_service.api
)

add_custom_command (
    OUTPUT audio_client.c audio_server.c audio_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/audio_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include audioUserInclude.h
                          --name-prefix le_audio_
                          --file-prefix audio_
    DEPENDS ${API_HDR_DIR}/audio_interface.h
)

# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the audioUserInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR})

add_legato_internal_executable(${AUDIO_DAEMON}
                               audio_server.c
                               serverMain.c)

target_link_libraries( ${AUDIO_DAEMON} ${LEGATO_SERVICES_AUDIO_TARGET} )


#
# Build the audio daemon client library
#

add_library(${LEGATO_SERVICES_AUDIO_CLIENT_TARGET} SHARED
            audio_client.c
            clientMain.c
)

target_link_libraries(${LEGATO_SERVICES_AUDIO_CLIENT_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
)


