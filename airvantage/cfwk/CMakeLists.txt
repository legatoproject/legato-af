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

PROJECT(CFWK)

INCLUDE_DIRECTORIES (${LUA_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_YAJL_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_YAJL_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${LIB_YAJL_SOURCE_DIR}/api)
INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CFWK_SOURCE_DIR})

ADD_SUBDIRECTORY(racon)
INCLUDE_DIRECTORIES(${CFWK_RACON_SOURCE_DIR}) # Other modules need racon headers
ADD_SUBDIRECTORY(airvantage)
ADD_SUBDIRECTORY(sms)
ADD_SUBDIRECTORY(system)
ADD_SUBDIRECTORY(update)
ADD_SUBDIRECTORY(devicetree)

ADD_CUSTOM_TARGET(cfwk)
ADD_DEPENDENCIES(cfwk
    Emp Swi_DSet Swi_AirVantage Swi_Sms Swi_System Swi_Update Swi_DeviceTree)