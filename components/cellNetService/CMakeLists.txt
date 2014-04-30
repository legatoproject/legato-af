#*******************************************************************************
# Copyright (C) Sierra Wireless, Inc. 2014.  All rights reserved.
# Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

#
# Build the Cellular Network Service Daemon
#

set(CNET_DAEMON cnetDaemon)
set_legato_component(${CNET_DAEMON})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# Set path for .api files and associated .h files
set(API_FILE_DIR ${LEGATO_SOURCE_DIR}/interfaces/cellNetService/api)
set(API_HDR_DIR  ${LEGATO_SOURCE_DIR}/interfaces/cellNetService/c)

# todo: These add_custom_command() definitions could probably be made a little
#       nicer by adding a CMake function.  Consider this for later.

add_custom_command (
    OUTPUT ${API_HDR_DIR}/le_cellnet_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/le_cellnet.api
                          --gen-interface
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/le_cellnet.api
)

add_custom_command (
    OUTPUT le_cellnet_client.c le_cellnet_server.c le_cellnet_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/le_cellnet.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
    DEPENDS ${API_HDR_DIR}/le_cellnet_interface.h
)


# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the userInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})

add_legato_internal_executable(${CNET_DAEMON}
                               le_cellnet_server.c
                               cellNetServiceServer.c)

target_link_libraries( ${CNET_DAEMON}
                       ${LEGATO_SERVICES_MODEM_CLIENT_TARGET}
                       ${LEGATO_CONFIGTREE_API_TARGET}
)


#
# Build the data connection service client library
#

add_library(${LEGATO_SERVICES_CNET_CLIENT_TARGET} SHARED
            le_cellnet_client.c
            cellNetServiceClient.c
)

target_link_libraries(${LEGATO_SERVICES_CNET_CLIENT_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
)


