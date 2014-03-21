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

PROJECT(CFWK_AIRVANTAGE)

INCLUDE_DIRECTORIES (${CFWK_AIRVANTAGE_SOURCE_DIR})
ADD_LIBRARY(Swi_AirVantage SHARED swi_airvantage.c)
ADD_PUBLIC_HEADER(Swi_AirVantage swi_airvantage.h)
TARGET_LINK_LIBRARIES(Swi_AirVantage Emp lib_yajl lib_swi_log)

ADD_UNIT_TEST(av_test av_test.c RUNTIME_DEPENDENCIES Swi_AirVantage lib_swi_log lualib)

INSTALL(TARGETS Swi_AirVantage LIBRARY DESTINATION lib)
INSTALL(FILES swi_airvantage.h DESTINATION itf)
