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

PROJECT(LUA)

# liblua static library. This is the lua VM

SET(LUALIB_SRC
  lapi.c
  lcode.c
  ldo.c
  lgc.c
  llex.c
  loadlib.c
  loslib.c
  lstring.c
  ltablib.c
  lvm.c
  lauxlib.c
  ldblib.c
  ldump.c
  linit.c
  lmathlib.c
  lobject.c
  lparser.c
  lstrlib.c
  ltm.c
  lzio.c
  lbaselib.c
  ldebug.c
  lfunc.c
  liolib.c
  lmem.c
  lopcodes.c
  lstate.c
  ltable.c
  lundump.c)

ADD_DEFINITIONS(-DLUA_USE_DLOPEN)
ADD_DEFINITIONS(-DLUA_USE_POSIX)
ADD_DEFINITIONS("-DLUA_PATH_DEFAULT=\"./?.lua\;./?/init.lua\;./lua/?.lua\;./lua/?/init.lua\"")
ADD_DEFINITIONS("-DLUA_CPATH_DEFAULT=\"./?.so\;./lua/?.so\"")

### lua library

ADD_LIBRARY(lualib SHARED ${LUALIB_SRC})
TARGET_LINK_LIBRARIES(lualib dl m)

### lua executable

ADD_EXECUTABLE(lua EXCLUDE_FROM_ALL lua.c)
TARGET_LINK_LIBRARIES (lua lualib pthread)
# The link to pthread shouldn't be necessary,
# but it prevents a segfault when using uClibc. To be removed when NPTL is used.

# readline support for the executable;
# added only if the library exists for that system
find_library(READLINE_LIB readline)
find_path( READLINE_INC readline/readline.h)
IF(READLINE_LIB AND READLINE_INC)
    ADD_DEFINITIONS(-DLUA_USE_READLINE)
    TARGET_LINK_LIBRARIES (lua readline)
ENDIF(READLINE_LIB AND READLINE_INC)

### luac compiler

# The compiler uses non-exported symbols "lua[DMPSTU]_*", and therefore can't
# rely on the shared library. Given its size, and the fact that it's not
# necessary to run Mihini, it reincludes the Lua lib's sources.
ADD_EXECUTABLE(luac EXCLUDE_FROM_ALL luac.c print.c ${LUALIB_SRC})
TARGET_LINK_LIBRARIES(luac dl m)

### bit32 bit manipulation library, back-ported from Lua 5.2

ADD_LUA_LIBRARY(bit32 lbitlib.c)
INSTALL(TARGETS lualib LIBRARY DESTINATION lib)
INSTALL(TARGETS lua luac RUNTIME DESTINATION bin)
INSTALL(TARGETS bit32 LIBRARY DESTINATION lua)
