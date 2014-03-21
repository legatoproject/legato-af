
#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************



set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/framework/tools/ifgen/ifgen)

set(INTERFACE_DIR "${PROJECT_SOURCE_DIR}/interfaces/config")

set(CONFIG_API_DIR   "${INTERFACE_DIR}/api")
set(CONFIG_API_C_DIR "${INTERFACE_DIR}/c")


include_directories("${CONFIG_API_C_DIR}" ${CMAKE_CURRENT_BINARY_DIR})



add_custom_command(
    OUTPUT "${CONFIG_API_C_DIR}/configTypes.h"
    COMMENT "Sym-Linking configTypes.h"
    COMMAND ln -sf "${CONFIG_API_DIR}/configTypes.h" "${CONFIG_API_C_DIR}/configTypes.h")



add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/le_cfg_client.c" "${CMAKE_CURRENT_BINARY_DIR}le_cfg_local.h"
    DEPENDS "${CONFIG_API_DIR}/config.api" "${CONFIG_API_C_DIR}/configTypes.h"
    COMMENT "Generating configuration API client side IPC code."
    COMMAND ${IFGEN_TOOL} --gen-client
                          --gen-local
                          --output-dir ${CMAKE_CURRENT_BINARY_DIR}
                          --user-include configTypes.h
                          --name-prefix le_cfg_
                          --file-prefix le_cfg_
                          "${CONFIG_API_DIR}/config.api")

add_custom_command(
    OUTPUT "${CONFIG_API_C_DIR}/le_cfg_interface.h"
    DEPENDS "${CONFIG_API_DIR}/config.api" "${CONFIG_API_C_DIR}/configTypes.h"
    COMMENT "Generating configuration API client interface header."
    COMMAND ${IFGEN_TOOL} --gen-interface
                          --output-dir ${CONFIG_API_C_DIR}
                          --user-include configTypes.h
                          --name-prefix le_cfg_
                          --file-prefix le_cfg_
                          "${CONFIG_API_DIR}/config.api")



set_property(SOURCE configInit.c
             APPEND PROPERTY OBJECT_DEPENDS "${CONFIG_API_C_DIR}/configTypes.h")

set_property(SOURCE configInit.c
             APPEND PROPERTY OBJECT_DEPENDS "${CONFIG_API_C_DIR}/le_cfg_interface.h")


add_library(le_cfg_api SHARED
            configInit.c
            "${CMAKE_CURRENT_BINARY_DIR}/le_cfg_client.c")

target_link_libraries(le_cfg_api ${LEGATO_FRAMEWORK_TARGET})
