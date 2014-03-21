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

PROJECT(CFWK_SMS)

ADD_LIBRARY(Swi_Sms SHARED swi_sms.c)
ADD_PUBLIC_HEADER(Swi_Sms swi_sms.h)
TARGET_LINK_LIBRARIES(Swi_Sms Emp lib_yajl)

ADD_UNIT_TEST(sms_test sms_test.c TEST_DEPENDENCY messaging_stubs RUNTIME_DEPENDENCIES Swi_Sms lib_swi_log lualib)
INSTALL(TARGETS Swi_Sms LIBRARY DESTINATION lib)
INSTALL(FILES swi_sms.h DESTINATION itf)
