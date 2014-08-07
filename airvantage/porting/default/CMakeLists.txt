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

IF(NOT TARGET socket_platform)
ADD_LUA_LIBRARY(socket_platform =socket/platform.lua)
ENDIF()

IF(NOT TARGET agent_platform)
ADD_LUA_LIBRARY(agent_platform =agent/defaultconfig.lua =agent/platform.lua)
ADD_DEPENDENCIES(agent_platform
    # Agent modules
    agent_modem agent_time agent_mediation agent_appcon agent_rest agent_autoexec
    agent_networkmanager agent_system agent_update agent_monitoring
    m3da_session_security
    # Device manager commands
    agent_devman_executescript agent_devman_reboot agent_devman_reset
    ntp web web_auth shell_telnet rpc_sched
    # Treemgr handlers
    agent_treemgr_constant agent_treemgr_table agent_treemgr_functable agent_treemgr_cellular
    # Racon
    racon_system racon_sms
)
ENDIF()

SET(SYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/agent/system)

ADD_CUSTOM_COMMAND(TARGET agent_platform POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYS_DIR}/start.sh ${CMAKE_INSTALL_PREFIX}/start.sh
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYS_DIR}/GPRS ${CMAKE_INSTALL_PREFIX}/bin/GPRS
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYS_DIR}/ETH ${CMAKE_INSTALL_PREFIX}/bin/ETH
)


INSTALL(PROGRAMS ${SYS_DIR}/GPRS DESTINATION bin)
INSTALL(PROGRAMS ${SYS_DIR}/ETH DESTINATION bin)

INSTALL(FILES socket/platform.lua DESTINATION lua/socket)
INSTALL(FILES agent/platform.lua agent/defaultconfig.lua DESTINATION lua/agent)
