/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "version.h"
#include <sys/utsname.h>


void push_sysversion(lua_State *L)
{
    int err;
    struct utsname name;

    err = uname(&name);
    if (err<0)
    {
        lua_pushstring(L, "Unknown");
        return;
    }

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaL_addstring(&b, name.sysname);
    luaL_addchar(&b, ' ');
    luaL_addstring(&b, name.nodename);
    luaL_addchar(&b, ' ');
    luaL_addstring(&b, name.release);
    luaL_addchar(&b, ' ');
    luaL_addstring(&b, name.version);
    luaL_addchar(&b, ' ');
    luaL_addstring(&b, name.machine);
    luaL_pushresult(&b);
}
