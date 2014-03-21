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

ADD_CUSTOM_TARGET(luaExecEnv
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/ldoc.sh -g \"\${GA_TRACKER}\"
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/*.zip
    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/Lua_User_API_doc
    COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/mihini-*.zip ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/Lua_User_API_doc ${CMAKE_CURRENT_BINARY_DIR}
)
