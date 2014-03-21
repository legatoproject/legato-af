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

PROJECT(MIHINI_SHELL)

ADD_SUBDIRECTORY(telnet)

ADD_LUA_LIBRARY(shell_telnet DESTINATION shell shell/telnet.lua)
ADD_DEPENDENCIES(shell_telnet sched socket_sched telnet)
INSTALL(FILES shell/telnet.lua DESTINATION lua/shell)
