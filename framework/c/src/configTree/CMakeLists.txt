
#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

include_directories("${PROJECT_SOURCE_DIR}/interfaces/config/api"
                    "${PROJECT_SOURCE_DIR}/interfaces/configAdmin/api"
                    ${CMAKE_CURRENT_BINARY_DIR})


set(IFGEN_TOOL ${LEGATO_SOURCE_DIR}/framework/tools/ifgen/ifgen)

set(CONFIG_API_DIR "${PROJECT_SOURCE_DIR}/interfaces/config/api")
set(CONFIG_C_DIR   "${PROJECT_SOURCE_DIR}/interfaces/config/c")

set(CONFIG_ADMIN_API_DIR "${PROJECT_SOURCE_DIR}/interfaces/configAdmin/api")
set(CONFIG_ADMIN_C_DIR   "${PROJECT_SOURCE_DIR}/interfaces/configAdmin/c")


add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/le_cfg_server.h"
           "${CMAKE_CURRENT_BINARY_DIR}/le_cfg_server.c"
    DEPENDS "${CONFIG_API_DIR}/config.api"
            "${CONFIG_API_DIR}/configTypes.h"
    COMMENT "Generating configuration API async server side IPC code."
    COMMAND ${IFGEN_TOOL} --gen-server
                          --gen-server-interface
                          --async-server
                          --gen-local
                          --user-include configTypes.h
                          --output-dir ${CMAKE_CURRENT_BINARY_DIR}
                          --name-prefix le_cfg_
                          --file-prefix le_cfg_
                          "${CONFIG_API_DIR}/config.api")


add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/le_cfgAdmin_server.h"
           "${CMAKE_CURRENT_BINARY_DIR}/le_cfgAdmin_server.c"
    DEPENDS "${CONFIG_ADMIN_API_DIR}/configAdmin.api"
            "${CONFIG_ADMIN_API_DIR}/configAdminTypes.h"
    COMMENT "Generating configuration administration API async server side IPC code."
    COMMAND ${IFGEN_TOOL} --gen-server
                          --gen-server-interface
                          --async-server
                          --gen-local
                          --user-include configAdminTypes.h
                          --output-dir ${CMAKE_CURRENT_BINARY_DIR}
                          --name-prefix le_cfgAdmin_
                          --file-prefix le_cfgAdmin_
                          "${CONFIG_ADMIN_API_DIR}/configAdmin.api")

add_definitions(-std=c99)

set_legato_component(configTree)
add_legato_internal_executable(configTree
                               ${CMAKE_CURRENT_BINARY_DIR}/le_cfg_server.c
                               ${CMAKE_CURRENT_BINARY_DIR}/le_cfgAdmin_server.c
                               configTree.c
                               configTreeApi.c
                               configTreeAdminApi.c
                               stringBuffer.c
                               dynamicString.c
                               requestQueue.c
                               nodeIterator.c
                               treePath.c
                               treeUser.c
                               treeDb.c)
