/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/* Export the Bysant serialization library to Lua.
 * bss_ctx_t is exported as a userdata;
 *
 * Each serialization function is exported as a method
 * of this userdata;
 *
 * Each method returns either the userdata, allowing chained
 * invocations, or nil followed by an error msg.
 *
 * The writer must be provided as a parameter to init(). It must
 * be a function taking a string and returning the number of
 * accepted bytes.
 *
 * Sample:
 *
 * serialize the following data in table 'buffer',
 * and concatenate the result into a string:
 * { position = { latitude=43.5; longitude=1.5 };
 *   timestamp = os.time();
 *   data = { 'A', 'B', 'C' } }
 *
 * buffer = { }
 * function write(x) table.insert(buffer, x); return #x end
 * require 'bysant.core' .init (write)
 *   :map()
 *     :string 'position' :map()
 *       :string 'latitude'  :number (43.5)
 *       :string 'longitude' :number (1.5)
 *     :close()
 *     :string 'timestamp' :number (os.time())
 *     :string 'data' :list()
 *       :string 'A'
 *       :string 'B'
 *       :string 'C'
 *     :close()
 *   :close()
 * return table.concat (buffer)
 */

#include <stdlib.h>
#include "bysant_core.h"
#include "lua.h"
#include "lauxlib.h"
#include "bysants.h"

/* Userdata content. */
typedef struct luactx_t {
  bss_ctx_t ctx; /* Bysant serialization context. */
  lua_State *L; /* Lua context. */
  int idx; /* Registration index of the userdata in the registry. */

} luactx_t;

#define RETURN_SELF do { lua_pushvalue( L, 1); return 1; } while( 0)
#define MT_NAME "m3da.bysant.core.sctx"
#define CTX_REGISTRY "proc.bysantcoresctx"
#define CHECK( x) do {                                  \
    int _r = (x);                                       \
    if( BSS_EOK != _r) return push_bss_error( L, _r);   \
  } while( 0)

/* Convert a numeric error into nil + lua string. */
static int push_bss_error( lua_State *L, int n) {
  lua_pushnil( L);
  switch( n) {
#define CASE(x) case BSS_E##x: lua_pushstring( L, #x); break
  CASE( OK);
  CASE( AGAIN);
  CASE( TOODEEP);
  CASE( INVALID);
  CASE( MEM);
  CASE( BROKEN);
  CASE( NOCONTAINER);
  CASE( BADMAP);
  CASE( SIZE);
  CASE( BADCTXID);
  CASE( BADCONTEXT);
  CASE( BADCLASSID);
  CASE( INTERNAL);
#undef CASE
  default:
    lua_pushinteger( L, n);
  }
  return 2;
}

/* Check that an bysant userdata is at index idx of the Lua stack. */
bss_ctx_t *lua_bss_checkctx( lua_State *L, int idx) {
  luactx_t *luactx = (luactx_t *) luaL_checkudata( L, idx, MT_NAME);
  return &(luactx->ctx);
}

/* Serialize a simple Lua value (no container type).
 * No point yet in supporting tables, as the server wouldn't know
 * what to do with them.
 * It wouldn't be hard to add though, if we can easily discriminate
 * hasmaps from lists. */
int lua_bss_serialize( lua_State *L, int idx, bss_ctx_t *bss) {
  switch( lua_type( L, idx)) {
  case LUA_TNIL:
    return bss_null( bss);
  case LUA_TBOOLEAN:
    return bss_bool( bss, lua_toboolean( L, idx));
  case LUA_TSTRING: {
    size_t len;
    const char *data;
    data = luaL_checklstring( L, idx, &len);
    return bss_lstring( bss, data, len);
  }
  case LUA_TNUMBER: {
    lua_Number n = luaL_checknumber( L, idx);
    return bss_double( bss, n);
  }
  default:
    return BSS_EINVALID;
  }
}
#if 0

bs_classid_t lua_bss_checkclassid( lua_State *L, bss_ctx_t *ctx, int idx) {
    if( lua_isnumber( L, idx)) {
        return (bs_classid_t) luaL_checkinteger( L, 2);
    } else if( lua_isstring( L, idx)) {
        const char *name = luaL_checkstring( L, 2);
        bs_class_t *cls = bs_classcoll_byname( ctx->classcoll, name);
        if( cls) return cls->classid;
        else ;// ????
    } else ; // ???

}

#endif

/* Wrapper to change the Lua writer function into an bss_writer_t. */
static int luawriter( unsigned const char *data, int len, void *writerctx) {
  luactx_t *c = (luactx_t *) writerctx;
  lua_State *L = c->L;
  int r;
  const char *strdata = (const char *) data;

  /* retrieve Lua writer function or table. */
  luaL_findtable( L, LUA_GLOBALSINDEX, CTX_REGISTRY, 0); // reg
  lua_pushinteger( L, c->idx);    // reg, idx
  lua_gettable( L, -2);           // reg, reg[idx]==udata
  lua_getfenv( L, -1);            // reg, udata, env
  lua_getfield( L, -1, "writer"); // reg, udata, env, writer

  if( lua_isfunction( L, -1)) {

    /* run it with the appropriate argument. */
    lua_pushlstring( L, strdata, len); // reg, udata, env, writer_function, data
    r = lua_pcall( L, 1, 1, 0);        // reg, udata, env, result

    /* cleanup the stack and turn the Lua result into a C return. */
    if( r) {
      lua_pop( L, 4);
      return BSS_EINVALID;
    }
    if( !lua_isnumber( L, -1)) {
      lua_pop( L, 4);
      // -
      luaL_error( L, "bysant writer callback must return a number");
    }
    r = luaL_checkint( L, -1);
    lua_pop( L, 4);
    // -
    return r;
  } else { /* writer is a table */
    int n = lua_objlen( L, -1);        // reg, udata, env, writer_table
    lua_pushinteger( L, n + 1);        // reg, udata, env, writer_table, n+1
    lua_pushlstring( L, strdata, len); // reg, udata, env, writer_table, n+1, data
    lua_settable( L, -3);              // reg, udata, env, writer_table
    lua_pop( L, 4);
    // -
    return len;
  }
}

/* export bss_init().
 * Return the userdata embedding the new bss_ctx_t, as well as
 * the writer as a convenience; it allows the idiom:
 * local ctx, buffer = bysant.core.init{ } */
static int api_init( lua_State *L) {
  luactx_t *c = lua_newuserdata( L, sizeof(*c)); // writer, udata
  int i = 1;

  if( !lua_isfunction( L, 1) && !lua_istable( L, 1))
    luaL_error( L, "writer function or table expected");

  c->L = L;

  /* Find a free index in the registry table. */
  luaL_findtable( L, LUA_GLOBALSINDEX, CTX_REGISTRY, 0); // writer, udata, reg
  while( 1) {               // writer, udata,  reg
    lua_pushinteger( L, i); // writer, udata, reg, i
    lua_gettable( L, -2);   // writer, udata, reg, reg[i]
    if( lua_isnil( L, -1)) break;
    lua_pop( L, 1);
    // writer, udata, reg
    i++;
  }                          // writer, udata, reg, nil
  lua_pop( L, 1);
  // writer, udata, reg

  /* Put the new userdata in the registry. */
  c->idx = i;
  lua_pushinteger( L, i); // writer, udata, reg, i
  lua_pushvalue( L, -3);  // writer, udata, reg, i, udata
  lua_settable( L, -3);   // writer, udata, reg[i=udata]
  lua_pop( L, 1);
  // writer, udata

  /* Put the writer function and the lua_State in the userdata's environment,
   * the later to protect it from garbage collection. */
  lua_newtable( L);
  // writer, udata, env
  lua_pushvalue( L, 1);           // writer, udata, env, writer
  lua_setfield( L, -2, "writer"); // writer, udata, env[writer=writer]
  lua_pushthread( L);             // writer, udata, env, L
  lua_setfield( L, -2, "L");      // writer, udata, env[L=L]
  lua_setfenv( L, -2);            // writer, udata[fenv=env]

  bss_init( &c->ctx, luawriter, c);

  /* Set the userdata's metatable. */
  lua_getfield( L, LUA_REGISTRYINDEX, MT_NAME); // writer, udata, mt
  lua_setmetatable( L, -2); // writer, udata
  lua_pushvalue( L, -2);    // writer, udata, writer
  return 2;
}

/* export bss_close(). */
static int api_close( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  CHECK( bss_close( ctx));
  RETURN_SELF;
}

/* export bss_list(). */
static int api_list( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  int len = luaL_optint( L, 2, -1);
  bs_ctxid_t ctxid = lua_bs_checkctxid( L, 3, BS_CTXID_GLOBAL);
  CHECK( bss_list( ctx, len, ctxid));
  RETURN_SELF;
}

/* export bss_map(). */
static int api_map( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  int len = luaL_optint( L, 2, -1);
  bs_ctxid_t ctxid = lua_bs_checkctxid( L, 3, BS_CTXID_GLOBAL);
  CHECK( bss_map( ctx, len, ctxid));
  RETURN_SELF;
}

/* export bss_object(). Class identifier can be passed by name or by numeric id. */
static int api_object( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);

  bs_classid_t classid;
  if( lua_isnumber( L, 2)) classid = luaL_checkinteger( L, 2);
  else {
      const char *classname = luaL_checkstring( L, 2);
      const bs_class_t *cls = bs_classcoll_byname( & ctx->classcoll, classname);
      if( ! cls) return push_bss_error( L, BSS_EBADCLASSID);
      classid = cls->classid;
  }
  CHECK( bss_object( ctx, classid));
  RETURN_SELF;
}

static int api_class( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  // check this before to not lose reference to classdef in case of error
  int internal = lua_toboolean( L, 3);
  bs_class_t *classdef = lua_bs_toclassdef( L, 2);
  int r = (bss_class( ctx, classdef, internal));
  if( BSS_EOK != r) {
    free(classdef);
    return push_bss_error( L, r);
  }
  RETURN_SELF;
}

/* export bss_chunkedString(). */
static int api_chunked( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  CHECK( bss_chunked( ctx));
  RETURN_SELF;
}

/* export bss_chunk(). */
static int api_chunk( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  size_t len;
  void *data = (void*) luaL_checklstring( L, 2, &len);
  CHECK( bss_chunk( ctx, data, len));
  RETURN_SELF;
}

/* export bss_int() and bss_double(). */
static int api_number( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  lua_Number n = luaL_checknumber( L, 2);
  CHECK( bss_double( ctx, n));
  RETURN_SELF;
}

/* FIXME: let these functions (they are useless as bss_double serializes integers too) */
/* export bss_int(). */
static int api_int( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  int i = luaL_checkinteger( L, 2);
  CHECK( bss_int( ctx, i));
  RETURN_SELF;
}

/* export bss_double(). */
static int api_double( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  double d = luaL_checknumber( L, 2);
  CHECK( bss_double( ctx, d));
  RETURN_SELF;
}

/* export bss_bool(). */
static int api_boolean( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  int b = lua_toboolean( L, 2);
  CHECK( bss_bool( ctx, b));
  RETURN_SELF;
}

/* export bss_lstring(). */
static int api_string( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  size_t len;
  const char *str = luaL_checklstring( L, 2, &len);
  CHECK( bss_lstring( ctx, str, len));
  RETURN_SELF;
}

/* export bss_null(). */
static int api_null( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  CHECK( bss_null( ctx));
  RETURN_SELF;
}

/* current nesting level, and max level. */
static int api_depth( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  lua_pushinteger( L, ctx->stacksize);
  lua_pushinteger( L, BSS_STACK_SIZE);
  return 2;
}

/* whether an error caused a corrupted output. */
static int api_broken( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  lua_pushboolean( L, ctx->broken);
  return 1;
}

static int api_setwriter( lua_State *L) {
  lua_bss_checkctx( L, 1); // udata, writer
  if( !lua_istable( L, 2) && !lua_isfunction( L, 2))
    luaL_error( L, "Invalid writer");

  lua_getfenv( L, 1);              // udata, writer, env
  lua_pushvalue( L, 2);            // udata, writer, env, writer
  lua_setfield( L, -2, "writer");  // udata, writer, env[writer=writer]
  RETURN_SELF;
}

/* reset a context (cheaper than garbaging it and creating a new one). */
static int api_reset( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  luactx_t *luactx = (luactx_t *) luaL_checkudata( L, 1, MT_NAME);
  bss_reset( ctx); // free previous memory before initializing with the new content
  bss_init( ctx, luawriter, luactx);
  if( !lua_isnoneornil( L, 2))
    return api_setwriter( L);
  else RETURN_SELF;
}

static int api_collect( lua_State *L) {
  bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
  bss_reset( ctx);
  return 0;
}

static int api_dumpclass( lua_State *L) {
    bss_ctx_t *ctx = lua_bss_checkctx( L, 1);
    const char *name = luaL_checkstring( L, 2);
    const bs_class_t *cls = bs_classcoll_byname( & ctx->classcoll, name);
    if( ! cls) { lua_pushnil( L); lua_pushstring( L, "class not found"); return 2; }
    lua_newtable( L); // ctx, name, desc
    lua_pushinteger( L, cls->classid);  // ctx, name, desc, id
    lua_setfield( L, -2, "id"); // ctx, name, desc[id]
    int i;
    for(i=0; i<cls->nfields; i++) {
        lua_pushinteger( L, i+1);  // ctx, name, desc, i+1
        lua_pushstring( L, cls->fields[i].name);  // ctx, name, desc, i+1, field.name
        lua_settable( L, -3); // ctx, name, desc[i+1=field.name]
    }
    return 1;
}

static int api_call( lua_State *L) {
    lua_bss_checkctx( L, 1);       // ctx, x
    luaL_findtable( L, LUA_GLOBALSINDEX, "m3da.bysant.core", 0); // ctx, x, m3da.bysant.core
    lua_getfield( L, -3, "value"); // ctx, x, m3da.bysant.core, value
    lua_pushvalue( L, -4);         // ctx, x, m3da.bysant.core, value, ctx
    lua_pushvalue( L, -4);         // ctx, x, m3da.bysant.core, value, ctx, x

    lua_call( L, 2, 2);            // ctx, x, m3da.bysant.core, statue, ?errmsg
    if(lua_isnil( L, -2)) {        // ctx, x, m3da.bysant.core, nil, errmsg
        return 2;
    } else {                       // ctx, x, m3da.bysant.core, status_ok, nil
        lua_pop( L, 1);             // ctx, x, m3da.bysant.core, status_ok
        return 1;
    }

    return 0;
}

/* Open the module; to be stored in package.preload['m3da.bysant.core']*/
int luaopen_m3da_bysant_core_serialize( lua_State *L) {
  /* Create context registry weak table. */
  luaL_findtable( L, LUA_GLOBALSINDEX, CTX_REGISTRY, 3); // reg
  lua_newtable( L);
  // reg, regmt
  lua_pushstring( L, "v");        // reg, regmt, "v"
  lua_setfield( L, -2, "__mode"); // reg, regmt[__mode="v"]
  lua_setmetatable( L, -2);       // reg
  lua_pop( L, 1);
  // -

  /* Register C functions. */
  luaL_findtable( L, LUA_GLOBALSINDEX, "m3da.bysant.core", 14); // m3da.bysant.core
#define REG(x) lua_pushcfunction( L, api_##x); lua_setfield( L, -2, #x)
  REG( boolean);
  REG( broken);
  REG( list);
  REG( map);
  REG( class);
  REG( object);
  REG( chunk);
  REG( chunked);
  REG( close);
  REG( depth);
  REG( double);
  REG( init);
  REG( int);
  REG( null);
  REG( number);
  REG( reset);
  REG( setwriter);
  REG( string);
  REG( dumpclass);
#undef REG

  /* set m3da.niltoken if absent */
  lua_getglobal( L, "m3da");        // m3da.bysant.core, m3da
  lua_getfield( L, -1, "niltoken");   // m3da.bysant.core, m3da, ?niltoken

  if( lua_isnil(L, -1)) {
    lua_pop( L, 1);                   // m3da.bysant.core, m3da
    lua_getglobal( L, "require");     // m3da.bysant.core, m3da, require
    lua_pushstring( L, "niltoken");   // m3da.bysant.core, m3da, require, "niltoken"
    lua_call( L, 1, 1);               // m3da.bysant.core, m3da, niltoken
    lua_setfield( L, -2, "niltoken"); // m3da.bysant.core, m3dat[niltoken=niltoken]
  }
  else lua_pop( L, 1); // m3da.bysant.core, m3da
  lua_pop( L, 1);       // m3da.bysant.core

  /* Set m3da.bysant.core as the methods table for bysant contexts. */
  luaL_newmetatable( L, MT_NAME);  // m3da.bysant.core, mt
  // set cleanup method
  lua_pushcfunction( L, api_collect); lua_setfield( L, -2, "__gc");
  lua_pushvalue( L, -2);           // m3da.bysant.core, mt, m3da.bysant.core
  lua_setfield( L, -2, "__index"); // m3da.bysant.core, mt[__index=m3da.bysant.core]
  lua_pushcfunction( L, api_call); // m3da.bysant.core, mt, __call
  lua_setfield( L, -2, "__call");  // m3da.bysant.core, mt[__call]
  lua_pushstring( L, "bysant.serializer"); // m3da.bysant.core, mt, "bysant.serializer"
  lua_setfield( L, -2, "__type");    // m3da.bysant.core, mt[__type="bysant.serializer"]

  lua_pop( L, 1);                  // m3da.bysant.core

  return 1; // m3da.bysant.core
}
