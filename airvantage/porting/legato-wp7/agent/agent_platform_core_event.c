/*******************************************************************************
 * Copyright (c) 2013 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "lauxlib.h"
#include "legato.h"

// Uncomment this block when you want to debug by bypassing the legato logging framework
//#undef LE_DEBUG
//#define LE_DEBUG(format, ...)  printf("%s: " format "\n", __func__, ##__VA_ARGS__)

DECL_IMPORT lua_State *L_eventloop;

static int l_getEventloopFd(lua_State *L)
{
    lua_pushinteger(L, le_event_GetFd());
    return 1;
}

static int l_processEvents(lua_State *L)
{
    le_result_t res = LE_WOULD_BLOCK;

    L_eventloop = L;
    do {
        LE_DEBUG("=> serviceLoop");
        res = le_event_ServiceLoop();
        LE_DEBUG("<= serviceLoop, res = %d", res);
    } while(res == LE_OK);

    lua_pushstring(L, "ok");
    return 1;
}

static const luaL_reg R[] =
{
    { "getEventloopFd", l_getEventloopFd},
    { "processEvents", l_processEvents},
    { NULL, NULL },
};

DECL_IMPORT void agent_platform_core_event_init(lua_State *L)
{
    luaL_register(L, "agent.platform.core", R);
}
