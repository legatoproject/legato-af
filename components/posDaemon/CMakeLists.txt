#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Build the Positioning Daemon

set(POS_DAEMON posDaemon)
set_legato_component(${POS_DAEMON})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# Set path for .api files and associated .h files
set(API_FILE_DIR ${LEGATO_SOURCE_DIR}/interfaces/positioning/api)
set(API_HDR_DIR  ${LEGATO_SOURCE_DIR}/interfaces/positioning/c)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/pos_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/pos_service.api
                          --gen-interface
                          --user-include posUserInclude.h
                          --name-prefix le_pos_
                          --file-prefix pos_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/pos_service.api
)

add_custom_command (
    OUTPUT pos_client.c pos_server.c pos_interface.h pos_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/pos_service.api
                          --gen-all
                          --user-include posUserInclude.h
                          --name-prefix le_pos_
                          --file-prefix pos_
    DEPENDS ${API_HDR_DIR}/pos_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/pos_sample_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/pos_sample_service.api
                          --gen-interface
                          --user-include posUserInclude.h
                          --name-prefix le_pos_sample_
                          --file-prefix pos_sample_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/pos_sample_service.api
)

add_custom_command (
    OUTPUT pos_sample_client.c pos_sample_server.c pos_sample_interface.h pos_sample_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/pos_sample_service.api
                          --gen-all
                          --user-include posUserInclude.h
                          --name-prefix le_pos_sample_
                          --file-prefix pos_sample_
    DEPENDS ${API_HDR_DIR}/pos_sample_interface.h
)


# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the posUserInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR})

add_legato_internal_executable(${POS_DAEMON}
                               pos_server.c
                               pos_sample_server.c
                               serverMain.c)

target_link_libraries( ${POS_DAEMON} ${LEGATO_SERVICES_POSITIONING_TARGET} )

#
# Build the positioning daemon client library
#

add_library(${LEGATO_SERVICES_POSITIONING_CLIENT_TARGET} SHARED
            pos_client.c
            pos_sample_client.c
            clientMain.c
)

target_link_libraries(${LEGATO_SERVICES_POSITIONING_CLIENT_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
)




