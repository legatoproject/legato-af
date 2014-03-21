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

#define leLogBinder(level)						                 \
static int l_LE_##level(lua_State *L)                                                    \
{                                                                                        \
    const char *module = luaL_checkstring(L, 1);                                         \
    const char *msg = luaL_checkstring(L, 2);                                            \
    lua_Debug ar;                                                                        \
    /* Get the stack frame context at the given level */                                 \
    lua_getstack(L, 5, &ar);                                                             \
    /* Get the filename and the currentline */                                           \
    lua_getinfo(L, "Sl", &ar);                                                           \
    char source[32];                                                                     \
    snprintf(source, sizeof(source), "%s(%s)", module, basename((char*)ar.source));      \
    _le_log_Send(LE_LOG_##level, NULL, LE_LOG_SESSION, source, "", ar.currentline, msg); \
    return 0;                                                                            \
}

leLogBinder(DEBUG)
leLogBinder(INFO)
leLogBinder(WARN)
leLogBinder(ERR)

static const luaL_reg R[] =
{
    { "LE_DEBUG", l_LE_DEBUG},
    { "LE_INFO", l_LE_INFO },
    { "LE_WARN", l_LE_WARN },
    { "LE_ERROR", l_LE_ERR},
    { NULL, NULL },
};

DECL_IMPORT void agent_platform_core_log_init(lua_State *L)
{
    luaL_register(L, "agent.platform.core", R);
}
