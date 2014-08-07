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
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "bysant_core.h"
#include "lua.h"
#include "lauxlib.h"
#include "bysant.h"

#include <stdlib.h>
#include <string.h>

static const char * const context_ids[] = {
    "global", "unsignedstring", "number", "int32", "float", "double", "listmap", NULL
};

bs_ctxid_t lua_bs_checkctxid( lua_State *L, int narg, bs_ctxid_t def) {
  return (bs_ctxid_t) luaL_checkoption( L, narg, context_ids[def], context_ids);
}

// Compute the alignement on the size of a pointer
#define ALIGN(x)    ((((ptrdiff_t)x)+sizeof(void*)-1) & ~(sizeof(void*)-1))

bs_class_t* lua_bs_toclassdef( lua_State *L, int narg) {
  size_t blocksize, size;
  void *endofblock;
  int named, nfields, i;
  bs_class_t *classdef;
  bs_classid_t classid;

  luaL_checkstack( L, 3, "class decoding");
  luaL_checktype( L, narg, LUA_TTABLE); // narg=>classdef
  nfields = lua_objlen( L, narg);

  // 1st pass: checks, size calculation
  blocksize = sizeof( bs_class_t) + nfields * sizeof( struct bs_field_t);

  // class id
  lua_getfield( L, narg, "id");
  classid = luaL_checkinteger( L, -1); // FIXME: make cryptic error messages
  lua_pop( L, 1);

  // classname
  lua_getfield( L, narg, "name"); // narg=>classdef, ..., -1=>name
  named = lua_isstring( L, -1);
  if( named) {
    lua_tolstring( L, -1, &size); size ++; // add one for the null terminator
    blocksize += ALIGN(size);
  }
  lua_pop( L, 1); // narg=>classdef

  for( i=1; i<=nfields; i++) {
    lua_pushinteger( L, i); lua_gettable( L, narg); // narg=>classdef, ..., -1=>fielddef
    if( !named && lua_isstring( L, -1)) { // short form
      lua_bs_checkctxid( L, -1, BS_CTXID_LAST);
    } else {
      // FIXME: make cryptic error messages
      lua_getfield( L, -1, "context"); lua_bs_checkctxid( L, -1, BS_CTXID_LAST); lua_pop( L, 1);
      if( named) {
        lua_getfield( L, -1, "name");
        luaL_checklstring( L, -1, &size); size ++; // count the null char at the end of the string.
        blocksize += size;
        lua_pop( L, 1);
      }
    }
    lua_pop( L, 1); // narg=>classdef
  }

  // 2nd pass: allocation, memory copy
  classdef = endofblock = malloc( blocksize);
  endofblock += sizeof( bs_class_t);
  classdef->classid = classid;
  classdef->mode = BS_CLASS_MANAGED;
  classdef->nfields = nfields;
  classdef->classname = NULL;
  if( named) {
    const char *name;
    lua_getfield( L, narg, "name"); // narg=>classdef, ..., -1=>name
    name = luaL_checklstring( L, -1, &size); size ++;  // add one for the null terminator. luaL_checklstring ensure that the string contains a null at the end
    memcpy( endofblock, name, size);
    classdef->classname = endofblock;
    endofblock += size;
  }
  classdef->fields = endofblock = (void*)ALIGN(endofblock);
  endofblock += nfields * sizeof( struct bs_field_t);

  for( i=1; i<=nfields; i++) {
    lua_pushinteger( L, i); lua_gettable( L, narg); // narg=>classdef, ..., -1=>fielddef

    if( !named && lua_isstring( L, -1)) { // short form
      classdef->fields[i-1].ctxid = lua_bs_checkctxid( L, -1, BS_CTXID_LAST);
    } else {
      lua_getfield( L, -1, "context"); // narg=>classdef, ..., -2=>fielddef, -1=>ctxid
      classdef->fields[i-1].ctxid = lua_bs_checkctxid( L, -1, BS_CTXID_LAST);
      lua_pop( L, 1); // narg=>classdef, ..., -1=>fielddef

      if( named) {
        const char *name;
        lua_getfield( L, -1, "name"); // narg=>classdef, ..., -2=>fielddef, -1=>name
        name = luaL_checklstring( L, -1, &size); size ++; // add one for the null terminator. luaL_checklstring ensure that the string contains a null at the end
        memcpy( endofblock, name, size);
        classdef->fields[i-1].name = endofblock;
        endofblock += size;
        lua_pop( L, 1); // narg=>classdef, ..., -1=>fielddef
      }
    }
    lua_pop( L, 1); // narg=>classdef
  }

  return classdef;
}
