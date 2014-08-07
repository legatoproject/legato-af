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

# Porting for Legato use case, for AR/WP devices

# Specific Legato porting files
IF(NOT TARGET agent_platform)
ADD_LUA_LIBRARY(agent_platform =agent/defaultconfig.lua =agent/platform.lua)
ADD_LUA_LIBRARY(agent_platform_event agent/platform_event.lua DESTINATION =agent/platform/event.lua)
ADD_LUA_LIBRARY(agent_platform_log agent/platform_log.lua DESTINATION =agent/platform/log.lua)
ADD_LUA_LIBRARY(agent_platform_nm agent/platform_nm.lua DESTINATION =agent/platform/nm.lua)
ADD_DEPENDENCIES(agent_platform agent_platform_event agent_platform_log agent_platform_nm)
ENDIF()

SET(LEGATO_SOURCE_DIR ../../..)
INCLUDE(${LEGATO_SOURCE_DIR}/cmake/legato-config.cmake)
SET_LEGATO_COMPONENT(AirVantage)

INCLUDE_DIRECTORIES(
    ${LUA_SOURCE_DIR}
    ${LIB_MIHINI_LUASIGNAL_SOURCE_DIR}
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
)
INCLUDE_DIRECTORIES(
    ${LEGATO_SOURCE_DIR}/framework/c/inc
    ${LEGATO_SOURCE_DIR}/framework/c/src
    ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/c
    ${LEGATO_SOURCE_DIR}/interfaces/modemServices/c
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/../bin/lib/)

ADD_DEFINITIONS(-DDECL_IMPORT=__attribute__\(\(visibility\(\"hidden\"\)\)\))
ADD_LUA_LIBRARY(agent_platform_core
    agent/agent_platform_core.c
    agent/agent_platform_core_event.c
    agent/agent_platform_core_nm.c
    agent/agent_platform_core_log.c
    DESTINATION agent/platform)
SET_TARGET_PROPERTIES (agent_platform_core PROPERTIES OUTPUT_NAME core)
TARGET_LINK_LIBRARIES(agent_platform_core legato le_data_client lib_luasignal)
ADD_DEPENDENCIES(agent_platform agent_platform_core)

ADD_CUSTOM_COMMAND(TARGET agent_platform POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/envVars ${CMAKE_INSTALL_PREFIX}/envVars
)


SET(MAP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/agent/treemgr/maps)
SET(MAP_DST ${CMAKE_INSTALL_PREFIX}/resources)
SET(HDL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/agent/treemgr/handlers)
SET(HDL_DST ${CMAKE_INSTALL_PREFIX}/lua/agent/treemgr/handlers)


ADD_CUSTOM_TARGET(legato_airvantage)
# Additional cfwk components
ADD_CUSTOM_TARGET(legato_cfwk)

ADD_LUA_LIBRARY(agent_legato_networkmanager =agent/netman.lua)
ADD_LUA_LIBRARY(agent_legato_appcon         =agent/appcon.lua)

ADD_DEPENDENCIES(legato_cfwk Swi_AirVantage Swi_DeviceTree)
# Additional agent components
ADD_CUSTOM_TARGET(legato_agent)
ADD_DEPENDENCIES(legato_agent
    # Agent modules
    agent_system agent_update agent_legato_networkmanager agent_legato_appcon
    shell_telnet
    # Device manager commands
    agent_devman_reboot agent_devman_reset agent_devman_appcon
)
# Add dependencies for the custom target defining the whole legato porting
# racon_aliases (Lua aliases) are here to help debugging.
ADD_DEPENDENCIES(legato_airvantage agent legato_agent legato_cfwk racon_aliases)

# Other porting targets are reused from default porting
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../default ${CMAKE_CURRENT_BINARY_DIR}/default)
