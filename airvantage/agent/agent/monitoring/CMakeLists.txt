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

ADD_LUA_LIBRARY(agent_monitoring DESTINATION agent/monitoring
    init.lua)

ADD_DEPENDENCIES(agent_monitoring
    sched
    persist
    agent_srvcon
    agent_devman
    agent_config
    socket_sched)
INSTALL(FILES init.lua DESTINATION lua/agent/monitoring)
