/*******************************************************************************
 * Copyright (c) 2013 Sierra Wireless and others.
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
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <libgen.h>

#include "lauxlib.h"
/* Public Legato API */
#include "legato.h"
/* Internal Legato API, required for initialization */
#include "log.h"
#include "args.h"
#include "eventLoop.h"
/* init functions implemented in other files */
#include "agent_platform_core.h"

static char *argv[] = { "AirVantage" };

static int l_syncWithSupervisor(lua_State *L)
{
    if (freopen("/dev/null", "r", stdin) == NULL) 
        return luaL_error(L, "Redirecting stdin failed");
    return 0;
}

static const luaL_reg R[] =
{
    { "syncWithSupervisor", l_syncWithSupervisor },
    { NULL, NULL }
};

int luaopen_agent_platform_core(lua_State *L)
{
    /* We don't use the COMPONENT_INIT as main entry point (unlike main components in legato),
     * so we need to do initialization explicitly.
     */
    arg_SetArgs(1, argv);
    LE_LOG_SESSION = log_RegComponent(STRINGIZE(LE_COMPONENT_NAME), &LE_LOG_LEVEL_FILTER_PTR);
    log_ConnectToControlDaemon();

    agent_platform_core_log_init(L);
    agent_platform_core_event_init(L);
    agent_platform_core_nm_init(L);
    luaL_register(L, "agent.platform.core", R);
    return 1;
}
