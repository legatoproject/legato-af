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

ADD_LUA_LIBRARY(agent_update DESTINATION agent/update
    init.lua builtinupdaters.lua pkgcheck.lua updatemgr.lua common.lua downloader.lua status.lua
)

ADD_DEPENDENCIES(agent_update racon crypto_md5)

ADD_LUA_LIBRARY(agent_update_tools EXCLUDE_FROM_ALL DESTINATION agent/update/tools
    tools/createpkg.lua
)

INSTALL(FILES init.lua builtinupdaters.lua pkgcheck.lua updatemgr.lua common.lua downloader.lua status.lua DESTINATION lua/agent/update)
INSTALL(FILES tools/createpkg.lua DESTINATION lua/agent/update/tools)

# create an empty runtime/update/drop folder that can be used to put pre-configuration
# update package (before agent first boot).
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/update/drop)
