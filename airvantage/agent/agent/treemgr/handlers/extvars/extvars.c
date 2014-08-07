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
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Fabien Fleutot     for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "lua.h"
#include "lauxlib.h"
#include "luasignal.h"
#include "lstate.h"
#include "extvars.h"

typedef struct
{
  char *name;
  rc_ReturnCode_t (*set)(int nvars, ExtVars_id_t *vars, void** values, ExtVars_type_t* types);
  rc_ReturnCode_t (*get) (ExtVars_id_t var, void **value, ExtVars_type_t *type);
  rc_ReturnCode_t (*register_var) (ExtVars_id_t var, int enable);
  rc_ReturnCode_t (*register_all) (int enable);
  rc_ReturnCode_t (*list)(int *nvars, ExtVars_id_t **vars);
  rc_ReturnCode_t (*get_release)(ExtVars_id_t var, void *value, ExtVars_type_t type);
  rc_ReturnCode_t (*list_release)(int nvars, ExtVars_id_t *vars);
} ExtVars_Mod_t;

static inline ExtVars_Mod_t *checkmod(lua_State *L)
{
    return (ExtVars_Mod_t *)luaL_checkudata(L, 1, "extvars_handler");
}

/* Static variables needed to handle and synchronize variable change notifications.
/ * This structure is a signleton. */
#define NOTIFY_SIGEMITTER "ExtVars"
#define NOTIFY_SIGEVENT   "CNotify"

static struct notify_buffer_t
{
    pthread_t            lua_thread;   // thread in which the Lua VM runs
    pthread_mutex_t      inprogress;   // ensure there's only one notification in progress
    sem_t                handled;      // released by Lua thread to signal handling completion
    lua_State           *L;            // lua state, in which notifications can be handled
    LuaSignalCtx        *luasigctx;    // context for luasignal emissions

    /* This content changes at each notification. */
    const char          *handler_name; // name of the handler whose notification is pending
    int                  nvars;        // # of variables whose notification is pending
    ExtVars_id_t        *vars;         // variable identifiers
    void               **values;       // variable values
    ExtVars_type_t      *types;        // variable types

} notify_buffer;

#define RETURN_ERROR_NUMBER(name, i) do {                       \
    lua_pushnil( L);                                            \
    const char* errstr = rc_ReturnCodeToString(i);              \
    if (!errstr) errstr = "UNSPECIFIED_ERROR";                  \
    lua_pushfstring(L, "%s:error %d in extvars.%s", errstr, i, name); \
    return 2;                                                   \
} while( 0)

#define RETURN_ERROR_STRING(msg, returnCodesMsg) do {    \
    lua_pushnil(L);                                      \
    lua_pushfstring(L, "%s:%s", returnCodesMsg, msg);	 \
    return 2;                                            \
} while(0)

#define RETURN_OK do{ lua_pushstring( L, "ok"); return 1; } while( 0)
#define RETURN_DA_NOT_FOUND do { lua_pushnil(L); lua_pushnil(L); return 2;} while(0)

/* Return whether the object at specified stack index is a nil token. */
static int isniltoken(lua_State *L, int index) {
    /* Ensure that index is positive: negative indexes will be messed up
     * when we'll push more stuff on the stack. */
    index = index < 0 ? lua_gettop(L) + index + 1 : index;

    /* push niltoken on the stack */
    lua_getglobal(     L, "require");  // require
    lua_pushstring(    L, "niltoken"); // require, "niltoken"
    lua_call(          L, 1, 1);       // niltoken
    if (lua_isnil(     L, -1)) return luaL_error( L, "module niltoken not found");
    int r = lua_equal( L, -1, index);  // niltoken
    lua_pop(           L, 1);          // -
    return r;
}

/* Called from `api_get`: add the list of every child id to children_set. */
static int get_all_var_names( lua_State *L) {
    // Initial stack state: ctx, hpath=="" (irrelevant)
    ExtVars_Mod_t *mod = checkmod(L);
    int nvars, *vars, i;
    rc_ReturnCode_t r;

    lua_newtable( L); // ctx, "", children_set

    if( ! mod->list) return 0; /* Var listing not implemented */

    r = mod->list(&nvars, &vars);

    if(r) RETURN_ERROR_NUMBER("get", r);

    for( i = 0; i<nvars; i++) {
        lua_pushnumber(  L, vars[i]); // ctx, "", children_set, var_number
        lua_tostring(    L, -1);      // ctx, "", children_set, var_string
        lua_pushboolean( L, 1);       // ctx, "", children_set, var_string, true
        lua_settable(    L, -3);      // ctx, "", children_set
    }

    lua_pushnil(   L);     // ctx, "", children_set, nil
    lua_pushvalue( L, -2); // ctx, "", children_set, nil, children_set
    if (mod->list_release)
        mod->list_release(nvars, vars);
    return 2;              // return nil, children_set
}

/* Called from `api_get`: push the appropriate value, retrieved from callback, on the Lua stack. */
static int get_leaf_value( lua_State *L) {
    ExtVars_Mod_t *mod = checkmod(L);
    int var = (int) lua_tonumber( L, 2);
    ExtVars_type_t  type;
    void       *value;
    rc_ReturnCode_t   r;

    if(!lua_isnumber(L, 2)) RETURN_DA_NOT_FOUND;

    r = mod->get(var, & value, & type);

    if(r) {
        if (r == RC_NOT_FOUND) {
            RETURN_DA_NOT_FOUND;
        }
        else {
            RETURN_ERROR_NUMBER("get", r);
	}
    }
    switch( type) {
        case EXTVARS_TYPE_STR:    lua_pushstring(  L, value ? (const char *) value : ""); break;
        case EXTVARS_TYPE_INT:    lua_pushinteger( L, *(int*)        value); break;
        case EXTVARS_TYPE_DOUBLE: lua_pushnumber(  L, *(double*)     value); break;
        case EXTVARS_TYPE_BOOL:   lua_pushboolean( L, *(int*)        value); break;
        case EXTVARS_TYPE_NIL:    lua_pushnil(L); break;
        default:                  RETURN_ERROR_STRING("Unknown ExtVars type", "UNSPECIFIED_ERROR"); // TODO get_release leak
    }
    if (mod->get_release)
        mod->get_release(var, value, type);
    return 1;
}

/* Takes an hpath and a children set;
 * Returns the value associated with `hpath` if applicable;
 * Add the list of all leaf nodes if `hpath` is "" the root path.
 * This implementation relies on the ExtVars design constraints:
 *
 * * the only non-leaf node is the root node, i.e. the tree is of depth 1 *
 * * each leaf name is a number. */
static int api_get(lua_State *L)
{
    // Initial stack state: ctx, hpath, children_set
    size_t len;
    luaL_checklstring( L, 2, & len); // We only want to know whether it's empty
    return (len == 0) ? get_all_var_names(L) : get_leaf_value(L);
}


/* Takes an hmap,
 * converts it into nvars / vars / values / types,
 * calls the corresponding `set` C callback. */
static int api_set( lua_State *L) {
    // Initial stack state: ctx, hmap
    ExtVars_Mod_t *mod = checkmod(L);
    int n_entries = 0, n_ints = 0, n_numbers = 0;
    int val_false = 0, val_true = 1;

    luaL_checktype( L, 2, LUA_TTABLE);

    /* First pass: count entries and numbers, to determine how much malloc space is needed. */
    lua_pushnil( L);           // ctx, hmap, key==nil
    while( lua_next( L, -2)) { // ctx, hmap, key, value
        n_entries ++;
        if( lua_type( L, -1) == LUA_TNUMBER) {
            lua_Number n = lua_tonumber( L, -1);
            if( (lua_Number) (int) n == n) n_ints ++; else n_numbers ++;
        }
        lua_pop( L, 1); // ctx, hmap, key
    }                   // ctx, hmap


    /* Allocate all tables as a single memory chunk.
     * By using Lua's allocator, we avoid an unnecessary dependency to malloc(). */
    int chunk_size =
            sizeof( int)            * n_entries + /* variable ids */
            sizeof( void*)          * n_entries + /* values       */
            sizeof( ExtVars_type_t) * n_entries + /* types        */
            sizeof( int)            * n_ints    + /* ints         */
            sizeof( lua_Number)     * n_numbers;  /* doubles      */
#ifdef NOMALLOC
    void *alloc_ctx;
    lua_Alloc allocf = lua_getallocf( L, & alloc_ctx);
    void *chunk = allocf( alloc_ctx, NULL, 0, chunk_size);
#else
    void *chunk = malloc( chunk_size);
#endif
    if( ! chunk) RETURN_ERROR_STRING("Not enough memory", "NO_MEMORY");

    int            *variables = (int*)            chunk;
    void          **values    = (void**)          (variables + n_entries);
    ExtVars_type_t *types     = (ExtVars_type_t*) (values    + n_entries);
    int            *ints      = (int*)            (types     + n_entries);
    lua_Number     *numbers   = (lua_Number*)     (ints      + n_ints);

    int i=0, i_int=0, i_number=0;

    /* 2nd pass: actually fill the tables for the set callback. */
    lua_pushnil( L); // ctx, hmap, key==nil
    while( lua_next( L, -2)) { // ctx, hmap, key, value

        /* Fill the variable id */
        int var = lua_tonumber( L, -2);
        if( ! var) RETURN_ERROR_STRING("Not a numeric variable name", "BAD_PARAMETER");
        variables[i] = var;

        /* Fill type and value, allocate number value if necessary */
        int ltype = lua_type( L, -1);
        if( isniltoken( L, -1)) {
            types[i] = EXTVARS_TYPE_NIL;
        } else if( ltype == LUA_TNUMBER) {
            lua_Number n = lua_tonumber( L, -1);
            if( (lua_Number)  (int) n == n) {
                ints[i_int] = (int) n;
                types[i] = EXTVARS_TYPE_INT;
                values[i] = ints + i_int;
                i_int++;
            } else {
                numbers[i_number] = (lua_Number) n;
                types[i] = EXTVARS_TYPE_DOUBLE;
                values[i] = numbers + i_number;
                i_number++;
            }
        } else if( ltype == LUA_TSTRING) {
            types[i]  = EXTVARS_TYPE_STR;
            values[i] = (void *) lua_tostring( L, -1);
        } else if( ltype == LUA_TBOOLEAN) {
            types[i] = EXTVARS_TYPE_BOOL;
            values[i] = lua_toboolean( L, -1) ? & val_true : & val_false;
        } else {
	  RETURN_ERROR_STRING("Unsupported Lua type", "BAD_PARAMETER");
        }
        lua_pop( L, 1); // ctx, hmap, key
        i++;
    } // ctx, hmap

    rc_ReturnCode_t r = mod->set(n_entries, variables, values, types);

    /* Lua-allocated chunks are freed by reallocating them to a 0 size. */
#ifdef NOMALLOC
    allocf( alloc_ctx, chunk, chunk_size, 0);
#else
    free( chunk);
#endif
    if(r) RETURN_ERROR_NUMBER("set", r); else RETURN_OK;
}

static int register_unregister( lua_State *L, int enable) {
    ExtVars_Mod_t *mod = checkmod(L);
    if( lua_isstring( L, 2) && lua_objlen( L, 2) == 0) {
        if(mod->register_all) {
	    rc_ReturnCode_t r = mod->register_all(enable);
            if( r) RETURN_ERROR_NUMBER( "register", r);
        }
    } else {
      if (! lua_isnumber(L, 2)) {
	    RETURN_OK;
        }
        else if(mod->register_var) {
            rc_ReturnCode_t r = mod->register_var(luaL_checkint( L, 2), enable);
            if( r) RETURN_ERROR_NUMBER( "register", r);
        }
    }
    RETURN_OK;
}

static int api_register( lua_State *L)   { return register_unregister( L, 1); }
static int api_unregister( lua_State *L) { return register_unregister( L, 0); }


/* Calls `require 'agent.treemgr' .notify (handler_name, hmap)`.
 * Can be either called directly by `trigger_notification`, if the later is called
 * in the Lua VM thread, or through a Lua signal.
 * Releases `notify_buffer->handled` to indicate that a notification handling
 * has been completed. */
static int handle_notification( lua_State *L) {
    int i, r;

    // TODO: register notify in a faster-to-reach place
    lua_getglobal(  L, "require");                   // require
    lua_pushvalue(  L, -1);                          // require, require
    lua_pushstring( L, "sched");                     // require, require, "sched"
    lua_call(       L, 1, 1);                        // require, sched
    lua_getfield(   L, -1, "run");                   // require, sched, run
    lua_pushvalue(  L, -3);                          // require, sched, run, require
    lua_pushstring( L, "agent.treemgr");             // require, sched, run, require, "agent.treemgr"
    lua_call(       L, 1, 1);                        // require, sched, run, treemgr
    lua_getfield(   L, -1, "notify");                // require, sched, run, treemgr, notify
    lua_remove(     L, -2);                          // require, sched, run, notify
    lua_pushstring( L, notify_buffer.handler_name);  // require, sched, run, notify, hname

    /* Create and fill the hmap of notified variables. */
    lua_newtable(   L);                              // require, sched, run, notify, hname, hmap
    for( i=0; i<notify_buffer.nvars; i++) {          // require, sched, run, notify, hname, hmap
        lua_pushnumber( L, notify_buffer.vars[i]);   // require, sched, run, notify, hname, hmap, id_num
        lua_tostring(    L, -1);                     // require, sched, run, notify, hname, hmap, id_string
        void *v = notify_buffer.values[i];
        switch( notify_buffer.types[i]) { /* Push the Lua-converted value on stack. */
        case EXTVARS_TYPE_INT:    lua_pushinteger( L, * (int*) v); break;
        case EXTVARS_TYPE_BOOL:   lua_pushboolean( L, * (int*) v); break;
        case EXTVARS_TYPE_DOUBLE: lua_pushnumber(  L, * (lua_Number*) v); break;
        case EXTVARS_TYPE_STR:    lua_pushstring(  L, (char*) v); break;
        default: lua_pushnil( L);  break; /* This is an error: just avoid a catastrophic failure */
        }                      // require, sched, run, notify, hname, hmap, id_string, value
        lua_settable( L, -3);  // require, sched, run, notify, hname, hmap[id_string=value]
    }                          // require, sched, run, notify, hname, hmap
    r = lua_pcall( L, 3, 0, 0);
    sem_post(&notify_buffer.handled); /* Allow next notification. */

    if( r) { // require, sched, error_msg
        printf("Error during notification: %s\n", lua_tostring( L, -1));
        lua_pop( L, 3); // -
    } else { // require, sched
        lua_pop( L, 2); // -
    }
    return 0;
}

/* This is the C function which allows to cause a variable change notification in the Lua VM.
 * Its works differently, depending on whether it's called from the thread in which the Lua
 * VM runs:
 *
 * * in all cases, it stores the notification description in `notify_buffer`.
 * * if it runs in the Lua VM thread, it calls the Lua handler directly;
 * * if called from another thread, it sends a Lua signal, which will trigger
 *   the notification handler, and waits until the handling is completed; completion
 *   is signaled by the release of a dedicated `notify_buffer->handled` mutex.
 */
static rc_ReturnCode_t trigger_notification(void *ctx, int nvars, ExtVars_id_t* vars, void** values, ExtVars_type_t* types) {

    ExtVars_Mod_t *mod = (ExtVars_Mod_t *)ctx;
    /* make sure that only one notification is in progress. */
    pthread_mutex_lock(&notify_buffer.inprogress);

    notify_buffer.handler_name = mod->name;
    notify_buffer.nvars        = nvars;
    notify_buffer.vars         = vars;
    notify_buffer.values       = values;
    notify_buffer.types        = types;

    if(pthread_self() == notify_buffer.lua_thread) { /* Direct nested call */
        handle_notification(notify_buffer.L);
    } else { /* Triggered through a pre-subscribed Lua signal */
        LUASIGNAL_SignalT( notify_buffer.luasigctx, NOTIFY_SIGEMITTER, NOTIFY_SIGEVENT, NULL);
    }

    /* If this function runs in the lua thread, the `handled` semaphore has already been released by the
     * Lua notification handler.
     *
     * If this thread is not the lua thread, the lua signal sent above will eventually cause the lua
     * handler to run, which will eventually release the `handled` sem.
     * Until this happens, the current thread pauses, and the notification ctx won't be altered. */
    sem_wait(&notify_buffer.handled);

    pthread_mutex_unlock(&notify_buffer.inprogress);

    return RC_OK;
}


static luaL_Reg hdlr[] = {
  {"get", api_get},
  {"set", api_set},
  {"register", api_register},
  {"unregister", api_unregister},
  {NULL, NULL}
};

static int l_load(lua_State *L)
{
  const char *name, *path;
  void *handler;
  ExtVars_Mod_t *mod;
 
  name = luaL_checkstring(L, 1);
  path = luaL_checkstring(L, 2);
  handler = dlopen(path, RTLD_LAZY | RTLD_LOCAL);

  if (handler == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, dlerror());
    return 2;
  }

  rc_ReturnCode_t (*init)(void) = dlsym(handler, "ExtVars_initialize");
  if (init) {
    rc_ReturnCode_t res = init();
    if (res) {
      lua_pushnil(L);
      lua_pushfstring(L, "ExtVars: Failed to initialize the treehandler %s [error code = %d]\n", path, res);
      return 2;
    }
  }

  mod = (ExtVars_Mod_t *)lua_newuserdata(L, sizeof(ExtVars_Mod_t));
  mod->name = strdup(name);
  mod->set = dlsym(handler, "ExtVars_set_variables");
  mod->get = dlsym(handler, "ExtVars_get_variable");
  mod->register_var = dlsym(handler, "ExtVars_register_variable");
  mod->register_all = dlsym(handler, "ExtVars_register_all");
  mod->list = dlsym(handler, "ExtVars_list");
  mod->get_release = dlsym(handler, "ExtVars_get_variable_release");
  mod->list_release = dlsym(handler, "ExtVars_list_release");

  if (mod->get == NULL) {
    lua_pushnil(L);
    lua_pushfstring(L, "ExtVars: %s: missing required operation \"get\"", path);
    return 2;
  }

  void (*set_notifier)(void *, ExtVars_notify_t *) = dlsym(handler, "ExtVars_set_notifier");
  if (set_notifier)
    set_notifier(mod, trigger_notification);

  notify_buffer.L = G( L)->mainthread;
  notify_buffer.lua_thread = pthread_self();
  pthread_mutex_init(&notify_buffer.inprogress, NULL);
  sem_init(&notify_buffer.handled, 0, 0);

  
  int r = LUASIGNAL_Init(&notify_buffer.luasigctx, 18888, NULL, NULL);
  if(r) 
    RETURN_ERROR_NUMBER( "newhandler/luasignal", r);

  /* Subscribe the notification handler to the signal, for notifications
   * triggered from outside the Lua thread. */
  lua_getglobal(L, "require");
  lua_pushstring(L, "sched");
  lua_call(L, 1, 1);
  lua_getfield(L, -1, "sighook");
  lua_pushstring(L, NOTIFY_SIGEMITTER);
  lua_pushstring(L, NOTIFY_SIGEVENT);
  lua_pushcfunction(L, handle_notification);
  lua_call(L, 3, 0);
  lua_pop(L, 1);

  luaL_getmetatable(L, "extvars_handler");
  lua_setmetatable(L, -2);
  return 1;
}

static const luaL_Reg R[] =
{
    { "load", l_load },
    { NULL, NULL }
};

int luaopen_agent_treemgr_handlers_extvars(lua_State* L)
{
    luaL_newmetatable(L, "extvars_handler");

    /* mt.__index = mt */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_register(L, NULL, hdlr);
    luaL_register(L, "extvars", R);
    return 1;
}
