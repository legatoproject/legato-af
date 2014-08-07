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

PROJECT(CFWK_DEVICETREE)

ADD_LIBRARY(Swi_DeviceTree SHARED swi_devicetree.c)
ADD_PUBLIC_HEADER(Swi_DeviceTree swi_devicetree.h)
TARGET_LINK_LIBRARIES(Swi_DeviceTree Emp lib_yajl lib_swi_log)

ADD_UNIT_TEST(dt_test dt_test.c RUNTIME_DEPENDENCIES Swi_DeviceTree lib_swi_log)
INSTALL(TARGETS Swi_DeviceTree LIBRARY DESTINATION lib)
INSTALL(FILES swi_devicetree.h DESTINATION itf)
