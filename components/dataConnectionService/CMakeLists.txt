#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

#
# Build the Data Connection Service Daemon
#

set(DCS_DAEMON dcsDaemon)
set_legato_component(${DCS_DAEMON})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# Set path for .api files and associated .h files
set(API_FILE_DIR ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/api)
set(API_HDR_DIR  ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/c)

# todo: These add_custom_command() definitions could probably be made a little
#       nicer by adding a CMake function.  Consider this for later.

add_custom_command (
    OUTPUT ${API_HDR_DIR}/le_data_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/le_data.api
                          --gen-interface
                          --user-include le_data_defs.h
                          --name-prefix le_data_
                          --file-prefix le_data_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/le_data.api
)

add_custom_command (
    OUTPUT le_data_client.c le_data_server.c le_data_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/le_data.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include le_data_defs.h
                          --name-prefix le_data_
                          --file-prefix le_data_
    DEPENDS ${API_HDR_DIR}/le_data_interface.h
)


# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the userInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR})

add_legato_internal_executable(${DCS_DAEMON}
                               le_data_server.c
                               serverMain.c)

target_link_libraries( ${DCS_DAEMON} ${LEGATO_SERVICES_MODEM_CLIENT_TARGET} )


#
# Build the data connection service client library
#

add_library(${LEGATO_SERVICES_DCS_CLIENT_TARGET} SHARED
            le_data_client.c
            clientMain.c
)

target_link_libraries(${LEGATO_SERVICES_DCS_CLIENT_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
)


