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

PROJECT(MIHINI_GPIO)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})

ADD_LUA_LIBRARY(gpio gpio.lua gpio.c)
ADD_DEPENDENCIES(gpio gpio_core sched lib_swi_log lib_returncodes)
TARGET_LINK_LIBRARIES(gpio lib_swi_log lib_returncodes)

INSTALL(TARGETS gpio LIBRARY DESTINATION lua)
INSTALL(FILES gpio.lua DESTINATION lua)
