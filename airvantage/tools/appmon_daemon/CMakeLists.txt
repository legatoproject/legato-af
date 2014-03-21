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

IF(MSVC)
  # Prevent some MSVC-specific warnings
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS "-DLPACK_API=__declspec(dllexport)")
ENDIF(MSVC)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})


ADD_EXECUTABLE(appmon_daemon appmon_daemon.c)
TARGET_LINK_LIBRARIES(appmon_daemon lib_pointerlist lib_swi_log)


ADD_DEFINITIONS(-Wall)

INSTALL(TARGETS appmon_daemon RUNTIME DESTINATION bin)
