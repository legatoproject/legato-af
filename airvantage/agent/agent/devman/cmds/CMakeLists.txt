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

# TODO: separate commands into separate target, to avoid embedding unnecessary ones

ADD_LUA_LIBRARY(agent_devman_cmds DESTINATION agent/devman/cmds
    ReadNode.lua)

ADD_DEPENDENCIES(agent_devman_cmds
    agent_config
    agent_treemgr
    agent_srvcon
    agent_update
    socket
    log_store
    racon
    niltoken
    persist)

ADD_LUA_LIBRARY(agent_devman_appcon DESTINATION agent/devman/cmds
    appcon.lua)

ADD_LUA_LIBRARY(agent_devman_update DESTINATION agent/devman/cmds
    SoftwareUpdate.lua)

ADD_LUA_LIBRARY(agent_devman_reboot DESTINATION agent/devman/cmds
    Reboot.lua)

ADD_LUA_LIBRARY(agent_devman_reset DESTINATION agent/devman/cmds
    ResetToFactoryDefault.lua)

ADD_LUA_LIBRARY(agent_devman_tcp DESTINATION agent/devman/cmds
    TCPRemoteConnect.lua)

ADD_LUA_LIBRARY(agent_devman_executescript DESTINATION agent/devman/cmds
    ExecuteScript.lua)

ADD_LUA_LIBRARY(agent_devman_logupload DESTINATION agent/devman/cmds
    LogUpload.lua)

ADD_DEPENDENCIES(agent_appcon agent_devman_appcon)
ADD_DEPENDENCIES(agent_update agent_devman_update)

INSTALL(FILES appcon.lua ExecuteScript.lua LogUpload.lua ReadNode.lua Reboot.lua ResetToFactoryDefault.lua SoftwareUpdate.lua TCPRemoteConnect.lua DESTINATION lua/agent/devman/cmds)
