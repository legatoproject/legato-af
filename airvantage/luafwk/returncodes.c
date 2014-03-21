/*******************************************************************************
 * Copyright (c) 2013 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *     Romain Perier  for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/* Wraps returncodes library, which converts between string and numeric
 * representations of status codes. */

#include <returncodes.h>
#include <lauxlib.h>

/* Converts a name into a numeric status, or `0` if unknown.
 * Warning: `0` also happens to be the status code corresponding to `"OK"`. */
static int api_tonumber( lua_State *L) {
    const char *name = luaL_checkstring( L, 1);
    lua_pushinteger( L, rc_StringToReturnCode( name));
    return 1;
}

/* Converts a numeric status into a name, or `nil` if not found. */
static int api_tostring( lua_State *L) {
    int num = luaL_checkinteger( L, 1);
    lua_pushstring( L, rc_ReturnCodeToString( num));
    return 1;
}

/* Loads the library. */
int luaopen_returncodes( lua_State *L) {
    lua_newtable( L);
    lua_pushcfunction( L, api_tonumber);
    lua_setfield( L, -2, "tonumber");
    lua_pushcfunction( L, api_tostring);
    lua_setfield( L, -2, "tostring");
    return 1;
}
