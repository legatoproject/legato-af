#*******************************************************************************
# Copyright (c) 2012 Sierra Wireless and others.
# All rights reserved. This building script is made available under the terms
# of the Eclipse Public License v1.0 which accompanies this distribution, and
# is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

PROJECT(MIHINI_LUASOCKET)

INCLUDE_DIRECTORIES(${MIHINI_LUASOCKET_SOURCE_DIR})

# TODO check where it lands
ADD_LUA_LIBRARY(mime mime/mime.c)

ADD_LUA_LIBRARY(socket ltn12.lua mime.lua socket.lua
    =socket/http.lua =socket/url.lua =socket/ftp.lua =socket/tp.lua =socket/smtp.lua)

SET(SOCKET_CORE_SRC
  auxiliar.c
  buffer.c
  except.c
  inet.c
  io.c
  luasocket.c
  options.c
  select.c
  tcp.c
  timeout.c
  udp.c
  usocket.c
)

ADD_LUA_LIBRARY(socket_core DESTINATION socket ${SOCKET_CORE_SRC})
SET_TARGET_PROPERTIES(socket_core PROPERTIES OUTPUT_NAME core)
ADD_DEPENDENCIES(socket socket_core socket_platform)

INSTALL(TARGETS mime LIBRARY DESTINATION lua)
INSTALL(TARGETS socket_core LIBRARY DESTINATION lua/socket)
install(FILES socket/http.lua socket/url.lua socket/ftp.lua socket/tp.lua socket/smtp.lua DESTINATION lua/socket)
INSTALL(FILES ltn12.lua mime.lua socket.lua DESTINATION lua)
