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
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "version.h"

int luaopen_agent_versions(lua_State *L)
{
#ifdef GIT_REV
    lua_pushstring(L, MIHINI_AGENT__MAJOR_VERSION "." MIHINI_AGENT__MINOR_VERSION " - Build: " GIT_REV);
#else
    lua_pushstring(L, MIHINI_AGENT__MAJOR_VERSION "." MIHINI_AGENT__MINOR_VERSION);
#endif
    lua_setglobal(L, "_MIHINI_AGENT_RELEASE");
    lua_pushstring(L, LUA_RELEASE);
    lua_setglobal(L, "_LUARELEASE");
    push_sysversion(L);
    lua_setglobal(L, "_OSVERSION");

    return 0;
}

