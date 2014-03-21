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

PROJECT(MIHINI_NTP)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR} .)
ADD_LUA_LIBRARY(ntp ntp.c ntp_time.c)


INSTALL(TARGETS ntp LIBRARY DESTINATION lua)
