/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "lauxlib.h"
#include <time.h>
#include <errno.h>
#include <string.h>

static int l_time(lua_State *L)
{
    struct timespec tp;
    int ret;

    ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    if (ret == -1)
    {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
	return 2;
    }
    lua_pushnumber(L, tp.tv_sec + ((double)tp.tv_nsec / 1000000000));
    return 1;
}

static const luaL_Reg R[] =
{
    { "time", l_time },
    { NULL, NULL }
};

int luaopen_sched_timer_core(lua_State* L)
{
    luaL_register(L, "timer.core", R);
    return 1;
}
