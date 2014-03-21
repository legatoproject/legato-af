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

PROJECT(MIHINI_AGENT)

INCLUDE_DIRECTORIES (${LUA_SOURCE_DIR})
INCLUDE_DIRECTORIES (executable)
INCLUDE_DIRECTORIES (${LIB_MIHINI_COMMON_SOURCE_DIR})

ADD_EXECUTABLE(agent_exec executable/agent.c)
TARGET_LINK_LIBRARIES (agent_exec lualib pthread)
#link to pthread should not be necessary here but prevent a segfault when using uClibc (to be removed when NPTL is used)

SET_TARGET_PROPERTIES(agent_exec PROPERTIES OUTPUT_NAME agent)

ADD_LUA_LIBRARY(agent_versions DESTINATION agent executable/version.c executable/sysversion.c)
SET_TARGET_PROPERTIES(agent_versions PROPERTIES OUTPUT_NAME versions)

ADD_SUBDIRECTORY(agent)

ADD_CUSTOM_TARGET(agent ALL)
ADD_DEPENDENCIES(agent agent_exec agent_modules extvars)

ADD_CUSTOM_COMMAND(TARGET agent POST_BUILD
    COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/crypto
)

INSTALL(TARGETS agent_exec RUNTIME DESTINATION bin)
INSTALL(TARGETS agent_versions LIBRARY DESTINATION lua/agent)
