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
  # Remove some anoying warnings while compiling with msvc...
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS "-DLPACK_API=__declspec(dllexport)")
ENDIF(MSVC)

INCLUDE_DIRECTORIES(${LIB_MIHINI_COMMON_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LIB_MIHINI_LUASIGNAL_SOURCE_DIR})


ADD_EXECUTABLE(luasignalcmd luasignalcmd.c)
TARGET_LINK_LIBRARIES(luasignalcmd lib_luasignal lib_swi_log pthread)


ADD_DEFINITIONS(-Wall)

INSTALL(TARGETS luasignalcmd RUNTIME DESTINATION bin)
