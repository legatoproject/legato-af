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

PROJECT(CFWK_RACON)

ADD_LIBRARY(Swi_DSet SHARED swi_dset.c)
ADD_PUBLIC_HEADER(Swi_DSet swi_dset.h)
TARGET_LINK_LIBRARIES(Swi_DSet lib_pointerlist)

ADD_LIBRARY(Emp SHARED emp.c)
TARGET_LINK_LIBRARIES(Emp Swi_DSet pthread lib_swi_log)

ADD_UNIT_TEST(dset_test dset_test.c RUNTIME_DEPENDENCIES lib_swi_log Swi_DSet)
ADD_UNIT_TEST(emp_test emp_test.c TEST_DEPENDENCY emp_server TEST_OPT 1234 RUNTIME_DEPENDENCIES lib_swi_log Emp pthread lib_yajl)
INSTALL(TARGETS Swi_DSet LIBRARY DESTINATION lib)
INSTALL(FILES swi_dset.h DESTINATION itf)
INSTALL(TARGETS Emp LIBRARY DESTINATION lib)
