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

PROJECT(sms)

ADD_LIBRARY(Swi_System SHARED swi_system.c)
ADD_PUBLIC_HEADER(Swi_System swi_system.h)
TARGET_LINK_LIBRARIES(Swi_System Emp lib_yajl)
ADD_UNIT_TEST(sys_test sys_test.c TEST_DEPENDENCY system_stubs RUNTIME_DEPENDENCIES Swi_System lib_swi_log)

INSTALL(TARGETS Swi_System LIBRARY DESTINATION lib)
INSTALL(FILES swi_system.h DESTINATION itf)
