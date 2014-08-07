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
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "lauxlib.h"
#include "legato.h"
#include "le_data_interface.h"
#include <assert.h>

#define USERDATA "nmud"

static le_data_ConnectionStateHandlerRef_t airvantage_handlerRef;
extern lua_State *L_eventloop;


//structure used to put le_data_RequestObjRef_t in a Lua userdata
typedef struct
{
  le_data_RequestObjRef_t data_ref;
} nmctx;


/*
 * l_connectionRequest and l_connectionRelease both assume that
 * ConnectionStateHandler will be called:
 *  - for each request (mount or unmount)
 *  - whatever is the current network state
 */


/* Asynchronous connectionRequest Lua API.
 * Final result will be received by ConnectionStateHandler.
 *
 * @returns on success: a Lua userdata containing the Request id
 *          to be given back to le_data_Release.
 * @returns on error: nil+error_string
 */
static int l_connectionRequest(lua_State *L)
{
    le_data_RequestObjRef_t ref = le_data_Request();
    if ( ref != NULL){
      nmctx* nmud = lua_newuserdata(L, sizeof(*nmud));
      nmud->data_ref = ref;
      luaL_getmetatable(L, USERDATA);
      lua_setmetatable(L, -2);
      return 1;
    }
    else{
      lua_pushnil(L);
      lua_pushstring(L, "le_data_Request failed (NULL returned)");
      return 2;
    }
}

/* Asynchronous connectionRelease Lua API.
 * Final result will be received by ConnectionStateHandler.
 *
 * @param Lua userdata containing the Request id as returned by l_connectionRequest
 *
 * @returns on success: "ok" string.
 *          to be given back to le_data_Release.
 * @returns on error: nil+error_string
 */
static int l_release(lua_State *L)
{
    nmctx* nmud = (nmctx*) luaL_checkudata(L, 1, USERDATA);
    if (!nmud || !nmud->data_ref) {
      //data_ref is set to NULL after le_data_Release is done
      //nmud NULL would be internal error (luaL_checkudata should make Lua error)
      lua_pushnil(L);
      lua_pushstring(L, "connection already released");
      return 2;
    }
    le_data_Release(nmud->data_ref); //le_data_Release is void
    nmud->data_ref = NULL;
    lua_pushstring(L, "ok");
    return 1;
}

static void ConnectionStateHandler(const char* intfName, bool isConnected, void* contextPtr)
{
    LE_INFO("Network interface '%s' %s connected", intfName, isConnected ? "is" : "is not");

    assert(L_eventloop != NULL);
    lua_getglobal(L_eventloop, "require");                                   // require
    lua_pushstring(L_eventloop, "sched");                                    // require, "sched"
    lua_call(L_eventloop, 1, 1);                                             // sched
    lua_getfield(L_eventloop, -1, "signal");                                 // sched, signal
    lua_pushstring(L_eventloop, "netman-legato");                            // sched, signal, "netman-legato"
    const char *state = isConnected ? "connected": "disconnected";
    lua_pushstring(L_eventloop, state);                                      // sched, signal, "netman-legato", state
    lua_call(L_eventloop, 2, 0);
}

static int l_connectionRegisterHandler(lua_State *L)
{
    airvantage_handlerRef = le_data_AddConnectionStateHandler(ConnectionStateHandler, NULL);
    lua_pushstring(L, "ok");
    return 1;
}


static const luaL_reg R[] =
{
    { "connectionRequest", l_connectionRequest},
    { "connectionRegisterHandler", l_connectionRegisterHandler},
    { NULL, NULL },
};


static const luaL_Reg module_functions[] =
{
 { "release", l_release },
 { NULL, NULL }
};

DECL_IMPORT void agent_platform_core_nm_init(lua_State *L)
{
    luaL_newmetatable(L, USERDATA);
    lua_pushvalue(L, -1); // push copy of the metatable
    lua_setfield(L, -2, "__index"); // set index file of the metatable
    luaL_register(L, NULL, module_functions); //Register functions in the metatable
    luaL_register(L, "agent.platform.core", R);
}
