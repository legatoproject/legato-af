
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

ADD_LUA_LIBRARY(platformserver EXCLUDE_FROM_ALL
    =platform/backend.lua
    =platform/favicon.lua
    =platform/init.lua
    =platform/pages.lua
    =platform/response_templates.lua
)

ADD_DEPENDENCIES(platformserver m3da_transport_servers lua web)
INSTALL(FILES platform/backend.lua platform/favicon.lua platform/init.lua platform/pages.lua platform/response_templates.lua DESTINATION lua/platform)
