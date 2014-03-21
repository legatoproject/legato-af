#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

#
# Build the Modem Daemon
#

set(MODEM_DAEMON modemDaemon)
set_legato_component(${MODEM_DAEMON})

# Set the path to the ifgen tool
set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/bin/ifgen )

# Set path for .api files and associated .h files
set(API_FILE_DIR ${LEGATO_SOURCE_DIR}/interfaces/modemServices/api)
set(API_HDR_DIR  ${LEGATO_SOURCE_DIR}/interfaces/modemServices/c)

# todo: These add_custom_command() definitions could probably be made a little
#       nicer by adding a CMake function.  Consider this for later.

add_custom_command (
    OUTPUT ${API_HDR_DIR}/info_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/info_service.api
                          --gen-interface
                          --user-include userInclude.h
                          --name-prefix le_info_
                          --file-prefix info_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/info_service.api
)

add_custom_command (
    OUTPUT info_client.c info_server.c info_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/info_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include userInclude.h
                          --name-prefix le_info_
                          --file-prefix info_
    DEPENDS ${API_HDR_DIR}/info_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/sms_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/sms_service.api
                          --gen-interface
                          --user-include userInclude.h
                          --name-prefix le_sms_msg_
                          --file-prefix sms_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/sms_service.api
)

add_custom_command (
    OUTPUT sms_client.c sms_server.c sms_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/sms_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include userInclude.h
                          --name-prefix le_sms_msg_
                          --file-prefix sms_
    DEPENDS ${API_HDR_DIR}/sms_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/mrc_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mrc_service.api
                          --gen-interface
                          --user-include userInclude.h
                          --name-prefix le_mrc_
                          --file-prefix mrc_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/mrc_service.api
)

add_custom_command (
    OUTPUT mrc_client.c mrc_server.c mrc_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mrc_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include userInclude.h
                          --name-prefix le_mrc_
                          --file-prefix mrc_
    DEPENDS ${API_HDR_DIR}/mrc_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/sim_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/sim_service.api
                          --gen-interface
                          --user-include userInclude.h
                          --name-prefix le_sim_
                          --file-prefix sim_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/sim_service.api
)

add_custom_command (
    OUTPUT sim_client.c sim_server.c sim_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/sim_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include userInclude.h
                          --name-prefix le_sim_
                          --file-prefix sim_
    DEPENDS ${API_HDR_DIR}/sim_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/mdc_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mdc_service.api
                          --gen-interface
                          --user-include userInclude.h
                          --name-prefix le_mdc_
                          --file-prefix mdc_
                          --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/mdc_service.api
)

add_custom_command (
    OUTPUT mdc_client.c mdc_server.c mdc_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mdc_service.api
                          --gen-client
                          --gen-server
                          --gen-server-interface
                          --gen-local
                          --user-include userInclude.h
                          --name-prefix le_mdc_
                          --file-prefix mdc_
    DEPENDS ${API_HDR_DIR}/mdc_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/mcc_profile_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mcc_profile_service.api
    --gen-interface
    --user-include userInclude.h
    --name-prefix le_mcc_profile_
    --file-prefix mcc_profile_
    --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/mcc_profile_service.api
)

add_custom_command (
    OUTPUT mcc_profile_client.c mcc_profile_server.c mcc_profile_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mcc_profile_service.api
    --gen-client
    --gen-server
    --gen-server-interface
    --gen-local
    --user-include userInclude.h
    --name-prefix le_mcc_profile_
    --file-prefix mcc_profile_
    DEPENDS ${API_HDR_DIR}/mcc_profile_interface.h
)

add_custom_command (
    OUTPUT ${API_HDR_DIR}/mcc_call_interface.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mcc_call_service.api
    --gen-interface
    --user-include userInclude.h
    --name-prefix le_mcc_call_
    --file-prefix mcc_call_
    --output-dir ${API_HDR_DIR}
    DEPENDS ${API_FILE_DIR}/mcc_call_service.api
)

add_custom_command (
    OUTPUT mcc_call_client.c mcc_call_server.c mcc_call_local.h
    COMMAND ${IFGEN_TOOL} ${API_FILE_DIR}/mcc_call_service.api
    --gen-client
    --gen-server
    --gen-server-interface
    --gen-local
    --user-include userInclude.h
    --name-prefix le_mcc_call_
    --file-prefix mcc_call_
    DEPENDS ${API_HDR_DIR}/mcc_call_interface.h
)

# Since the generated header files go into the BINARY_DIR, need to add this to the
# include path for the compiler.  Also need to add the original source directory to
# the include path, since the userInclude.h file is still located there.
add_definitions(-I${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR})

add_legato_internal_executable(${MODEM_DAEMON}
                               info_server.c
                               sms_server.c
                               mrc_server.c
                               sim_server.c
                               mdc_server.c
                               mcc_profile_server.c
                               mcc_call_server.c
                               serverMain.c)

target_link_libraries( ${MODEM_DAEMON} ${LEGATO_SERVICES_MODEM_TARGET} )


#
# Build the modem daemon client library
#

add_library(${LEGATO_SERVICES_MODEM_CLIENT_TARGET} SHARED
            info_client.c
            sms_client.c
            mrc_client.c
            sim_client.c
            mdc_client.c
            mcc_profile_client.c
            mcc_call_client.c
            clientMain.c
)

target_link_libraries(${LEGATO_SERVICES_MODEM_CLIENT_TARGET}
                      ${LEGATO_FRAMEWORK_TARGET}
)


