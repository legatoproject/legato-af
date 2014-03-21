
#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************



set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/framework/tools/ifgen/ifgen)

set(INTERFACE_DIR "${PROJECT_SOURCE_DIR}/interfaces/configAdmin")

set(CONFIG_API_DIR   "${INTERFACE_DIR}/api")
set(CONFIG_API_C_DIR "${INTERFACE_DIR}/c")


include_directories("${PROJECT_SOURCE_DIR}/interfaces/config/c"
                    "${CONFIG_API_C_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}")



add_custom_command(
    OUTPUT "${CONFIG_API_C_DIR}/configAdminTypes.h"
    COMMENT "Sym-Linking configAdminTypes.h"
    COMMAND ln -sf "${CONFIG_API_DIR}/configAdminTypes.h" "${CONFIG_API_C_DIR}/configAdminTypes.h")




add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/le_cfgAdmin_client.c"
           "${CMAKE_CURRENT_BINARY_DIR}le_cfgAdmin_local.h"
    DEPENDS "${CONFIG_API_DIR}/configAdmin.api" "${CONFIG_API_C_DIR}/configAdminTypes.h"
    COMMENT "Generating configuration administration API client side IPC code."
    COMMAND ${IFGEN_TOOL} --gen-client
                          --gen-local
                          --output-dir ${CMAKE_CURRENT_BINARY_DIR}
                          --user-include configAdminTypes.h
                          --name-prefix le_cfgAdmin_
                          --file-prefix le_cfgAdmin_
                          "${CONFIG_API_DIR}/configAdmin.api")

add_custom_command(
    OUTPUT "${CONFIG_API_C_DIR}/le_cfgAdmin_interface.h"
    DEPENDS "${CONFIG_API_DIR}/configAdmin.api" "${CONFIG_API_C_DIR}/configAdminTypes.h"
    COMMENT "Generating configuration API client interface header."
    COMMAND ${IFGEN_TOOL} --gen-interface
                          --output-dir ${CONFIG_API_C_DIR}
                          --user-include configAdminTypes.h
                          --name-prefix le_cfgAdmin_
                          --file-prefix le_cfgAdmin_
                          "${CONFIG_API_DIR}/configAdmin.api")



set_property(SOURCE configAdminInit.c
             APPEND PROPERTY OBJECT_DEPENDS "${CONFIG_API_C_DIR}/configAdminTypes.h")

set_property(SOURCE configAdminInit.c
             APPEND PROPERTY OBJECT_DEPENDS "${CONFIG_API_C_DIR}/le_cfgAdmin_interface.h")


add_library(le_cfgAdmin_api SHARED
            configAdminInit.c
            "${CMAKE_CURRENT_BINARY_DIR}/le_cfgAdmin_client.c")

target_link_libraries(le_cfgAdmin_api ${LEGATO_FRAMEWORK_TARGET})
