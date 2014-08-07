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

IF(NOT TARGET messaging)

PROJECT(MIHINI_MESSAGING)

ADD_LUA_LIBRARY(messaging messaging.lua messaginghal.lua)
ADD_LUA_LIBRARY(smspdu smspdu.c)
ADD_DEPENDENCIES(messaging smspdu)

INSTALL(TARGETS smspdu LIBRARY DESTINATION lua)
INSTALL(FILES messaging.lua messaginghal.lua DESTINATION lua)

ENDIF()
