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

PROJECT(update)

ADD_LIBRARY(Swi_Update SHARED swi_update.c)
ADD_PUBLIC_HEADER(Swi_Update swi_update.h)
TARGET_LINK_LIBRARIES(Swi_Update Emp lib_yajl lib_swi_log)

ADD_UNIT_TEST(updatetest update_test.c TEST_DEPENDENCY messaging_stubs RUNTIME_DEPENDENCIES Swi_Update lib_swi_log lualib)
INSTALL(TARGETS Swi_Update LIBRARY DESTINATION lib)
INSTALL(FILES swi_update.h DESTINATION itf)
