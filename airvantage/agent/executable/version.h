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

// Define the version of the Agent here:

#define MIHINI_AGENT__MAJOR_VERSION        "0"
#define MIHINI_AGENT__MINOR_VERSION        "9"



//
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

void push_sysversion(lua_State *L);

int luaopen_agent_versions(lua_State *L);
