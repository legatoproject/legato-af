/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef LOADER_PATH_BASE

//CPATH_PATTERN:
//${PREVIOUS_CPATH};${PRELOAD_PATH_BASE}/${APPNAME}/?.so
#define CPATH_PATTERN ";%s/%s/?.so"
//LUAPATH_PATTERN:
//${PREVIOUS_LPATH};${PRELOAD_PATH_BASE}/${APPNAME}/?.lua;${PRELOAD_PATH_BASE}/${APPNAME}/?/init.lua;
#define LUAPATH_PATTERN ";%s/%s/?.lua;%s/%s/?/init.lua"

/* --- preload.addpath(subpath1, subpath2, ..., subpathn)
 * -- Add a/several new path(s) to be searched when loading Lua code using require.
 * -- This function patches global values: package.path and package.cpath
 * -- The new path is concatenated with the previous value of the according global value.
 * -- @param subpath string to be concatenated with LOADER_PATH_BASE to form the new path.
 * -- @return "ok" on success, nil+error message otherwise
 */
int l_addpath(lua_State *L)
{

  int nb_args = lua_gettop(L);
  if (nb_args < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, "Not enough argument");
    return 2;
  }

  //Patch Lua patch
  luaL_Buffer b;
  luaL_buffinit(L, &b);

  lua_getglobal(L, "package"); //-1: package
  lua_getfield(L, -1, "path"); //-1: path -2: package
  lua_remove(L, -2); //-1: path
  luaL_addvalue(&b); // void

  int i = 0;
  for (; i < nb_args; i++)
  {

    const char* appname = luaL_checkstring(L, i+1);
    lua_pushfstring(L, LUAPATH_PATTERN, LOADER_PATH_BASE, appname, LOADER_PATH_BASE,
        appname); //-1:new path
    luaL_addvalue(&b); //void
  }
  luaL_pushresult(&b); //-1 bufres
  lua_getglobal(L, "package"); //-1 package -2 bufres
  lua_insert(L, -2); //-1 bufres -2 pachage
  lua_setfield(L, -2, "path"); //-: package
  lua_pop(L, 1); //void

  //Patch C PATH
  luaL_buffinit(L, &b);
  lua_getglobal(L, "package"); //-1: package
  lua_getfield(L, -1, "cpath"); //-1: path -2: package
  lua_remove(L, -2); //-1: path
  luaL_addvalue(&b); // void

  i = 0;
  for (; i < nb_args; i++)
  {
    const char* appname = luaL_checkstring(L, i+1);
    lua_pushfstring(L, CPATH_PATTERN, LOADER_PATH_BASE, appname); //-1:new cpath
    luaL_addvalue(&b); //void
  }
  luaL_pushresult(&b);
  lua_getglobal(L, "package");
  lua_insert(L, -2); //-1 bufres -2 pachage
  lua_setfield(L, -2, "cpath"); //-1: path -2: package
  lua_pop(L, 1); //void

  lua_pushstring(L, "ok");
  return 1;
}
static const luaL_reg R[] = { { "addpath", l_addpath }, { NULL, NULL } };

int luaopen_loader(lua_State *L)
{
  luaL_register(L, "loader", R);
  return 1;

}
#else
int luaopen_loader(lua_State *L)
{
  lua_pushstring(L, "no define for LOADER_PATH_BASE, loader module can't be loaded");
  lua_error(L);
  return 0;
}
#endif

