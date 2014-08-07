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

INCLUDE_DIRECTORIES(
    ${LIB_MIHINI_COMMON_SOURCE_DIR}
    # ${LIB_ALEOSVARS_SOURCE_DIR}
    ${LIB_EXTVARS_SOURCE_DIR}
    ${LIB_MIHINI_LUASIGNAL_SOURCE_DIR}
)

ADD_LUA_LIBRARY(treehdlsample EXCLUDE_FROM_ALL DESTINATION agent/treemgr/handlers/extvars treehdlsample.c)
TARGET_LINK_LIBRARIES(treehdlsample lib_swi_log)
#INSTALL(TARGETS treehdlsample LIBRARY DESTINATION lua)
