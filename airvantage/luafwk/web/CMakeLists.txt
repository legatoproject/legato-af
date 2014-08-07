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

PROJECT(MIHINI_WEB)

ADD_LUA_LIBRARY(web EXCLUDE_FROM_ALL DESTINATION web
    server.lua compile.lua session.lua template.lua rest.lua)
ADD_LUA_LIBRARY(web_auth EXCLUDE_FROM_ALL DESTINATION web/auth
    auth/basic.lua auth/digest.lua)
ADD_DEPENDENCIES(web sched socket_sched)
INSTALL(FILES server.lua compile.lua session.lua template.lua rest.lua DESTINATION lua/web)
INSTALL(FILES auth/basic.lua auth/digest.lua DESTINATION lua/web/auth)
