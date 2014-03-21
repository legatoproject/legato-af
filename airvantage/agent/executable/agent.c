/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot   for Sierra Wireless - initial API and implementation
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "version.h"

// LUA_AF_RO_PATH: This is the location used for read only components like binaries, libraries, resources
// LUA_AF_RF_PATH: This is the location used for persisted data or components which need to be saved for future usage like:
// persisted settings, appcon, update, treemgr
static void envsetup(lua_State *L)
{
  char *ro_path, *rw_path;

  ro_path = getenv("LUA_AF_RO_PATH");
  rw_path = getenv("LUA_AF_RW_PATH");

  // the 'strict' module is loaded when the agent is started,
  // so defining nil global variables here won't work
  // because the VM won't use the special table defined in strict.
  // We push a boolean value instead
  if (ro_path)
    lua_pushstring(L, ro_path);
  else
    lua_pushboolean(L, 0);
  lua_setglobal(L, "LUA_AF_RO_PATH");

  if (rw_path)
    lua_pushstring(L, rw_path);
  else
    lua_pushboolean(L, 0);
  lua_setglobal(L, "LUA_AF_RW_PATH");
}

int main(void)
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

#ifdef AWT_USE_PRELOADED_LIBS
#include "preload.h"
  luapreload_preload(L);
#endif

  envsetup(L);

  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  lua_getglobal(L, "require");
  lua_pushstring(L, "agent.boot");

  int s = lua_pcall (L, 1, 0, -3);
  const char* err = lua_tostring(L, -1);
  if (s == LUA_ERRRUN)
    printf("Runtime error: %s\n", err);
  else if (s == LUA_ERRMEM)
    printf("Memory error: %s\n", err);
  else if (s != 0)
    printf("Debug Handler error: %s\n", err);
  else
    printf("Application finished normally.\n");

  lua_close(L);
  return s;
}
