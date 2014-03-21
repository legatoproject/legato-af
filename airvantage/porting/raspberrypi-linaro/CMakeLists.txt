#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

IF(NOT TARGET socket_platform)
ADD_LUA_LIBRARY(socket_platform =socket/platform.lua)
ENDIF()

IF(NOT TARGET agent_platform)
ADD_LUA_LIBRARY(agent_platform =agent/defaultconfig.lua =agent/platform.lua)
ENDIF()

SET(SYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/agent/system)

ADD_CUSTOM_COMMAND(TARGET agent_platform POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYS_DIR}/GPRS ${CMAKE_INSTALL_PREFIX}/bin/GPRS
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYS_DIR}/ETH ${CMAKE_INSTALL_PREFIX}/bin/ETH
)


INSTALL(PROGRAMS ${SYS_DIR}/GPRS DESTINATION bin)
INSTALL(PROGRAMS ${SYS_DIR}/ETH DESTINATION bin)

INSTALL(FILES socket/platform.lua DESTINATION lua/socket)
INSTALL(FILES agent/platform.lua agent/defaultconfig.lua DESTINATION lua/agent)
