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

#include "bysant_core.h"
#include "lua.h"
#include "lauxlib.h"
#include "bysantd.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MT_NAME "m3da.bysant.core.dctx"
#define CKSTACK(L, x) luaL_checkstack( L, x, "M3DA Bysant deserializer")

//store shortcut in reference system, containing niltoken, set at init
static int niltoken_reg = 0;
//push onto the stack the niltoken set at init
static void push_niltoken(lua_State *L){
    lua_rawgeti(L, LUA_REGISTRYINDEX, niltoken_reg);
}

/* Converts the deserialized data in `x` into a Lua value on the stack.
 * Pushes an error msg string on the stack in case of failure.
 * return 0 in case of success, non-zero in case of failure.
 *
 * If the value deserialized is part of a container, the container whose
 * deserialization is in progress is expected to be on top of the Lua stack
 * when `bysant2lua` is called.
 * */
static int bysant2lua( lua_State *L, const bsd_data_t *x) {
    /* This buffer will be used to efficiently concatenate chunked strings. Since objects are
     * guaranted to be only chunks (or errors), there is no stack manipulation problem.
     * Instead of allocating as global, it is allocated as userdata when needed. */
    //FIXME: this assume that the function is always called with the same lua_State.
    static luaL_Buffer *buf;

    CKSTACK( L, 1); /* at least one object will be pushed */

    /* step 1: the object is pushed on stack */
    switch( x->type) {
    /* pushnumber is used instead of pushinteger because of possible overflows:
     * lua_Integer is likely defined as ptrdiff_t which is a 32 bit integer. */
    case BSD_INT:     lua_pushnumber( L, x->content.i);     break;
    case BSD_NULL:    push_niltoken( L);                    break; //handle M3DA null coming from server
    case BSD_BOOL:    lua_pushboolean( L, x->content.bool); break;
    case BSD_DOUBLE:  lua_pushnumber( L, x->content.d);     break;
    case BSD_STRING:  lua_pushlstring( L, x->content.string.data, x->content.string.length); break;
    case BSD_CHUNK:   luaL_addlstring( buf, x->content.chunk.data, x->content.chunk.length); break;

    case BSD_MAP: case BSD_ZMAP: {
        int len = BSD_ZMAP==x->type ? 0 : x->content.length;
        lua_createtable( L, 0, len); // table
        break;
    }

    /* The index of the last object inserted in the Lua list is kept at
     * stack index -2. */
    case BSD_LIST: case BSD_ZLIST: {
        int len = BSD_ZLIST==x->type ? 0 : x->content.length;
        CKSTACK( L, 2);
        lua_pushinteger( L, 0);      // idx
        lua_createtable( L, len, 0); // idx, table
        break;
    }

    case BSD_OBJECT:
        /* If the class is named, fields will be named rather than numbered;
         * the class table will then be used as a list.
         * `idx` is the index of the last inserted field in tables used as lists.
         * Even for named classes, a dummy `idx` is inserted to save a test
         * when closing. */
        CKSTACK( L, 3);
        lua_pushinteger( L, 0);                                       // idx
        if( NULL == x->content.classdef->classname) {
            lua_createtable( L, x->content.classdef->nfields, 1);     // idx, table
            lua_pushinteger( L, x->content.classdef->classid);        // idx, table, classid
        } else { /* named class */
            lua_createtable( L, 0, x->content.classdef->nfields + 1); // idx, table
            lua_pushstring( L, x->content.classdef->classname);       // idx, table, classname
        }
        lua_setfield( L, -2, "__class");           // idx, table[__class=classid or classname]
        break;

    case BSD_CHUNKED_STRING:
        buf = lua_newuserdata( L, sizeof(luaL_Buffer)); // buffer
        luaL_buffinit( L, buf);
        break;

    case BSD_CLOSE:
        switch( x->content.cont_type) {
        case BSD_CHUNKED_STRING:   // buffer
            luaL_pushresult( buf); // buffer, result_string
            lua_replace( L, -2);   // result_string
            break;
        case BSD_OBJECT: case BSD_LIST: case BSD_ZLIST: // idx, table
            lua_replace( L, -2); // table
            break;
        default: break; // including MAP and ZMAP
        }
        break;

    case BSD_ERROR:
        switch( x->content.error) {
#        define CASE(x) case BSD_E##x: lua_pushstring( L, #x); break
        CASE( NOTIMPL);
        CASE( INVALID);
        CASE( BADCONTEXT);
        CASE( INVOPCODE);
        CASE( BADCLASSID);
        CASE( TOODEEP);
        CASE( INTERNAL);
#        undef CASE
        default: lua_pushinteger( L, x->content.error);
        }
        //printf("bysant: reinitializing ctx due to error\n");
        return -1;

    case BSD_CLASSDEF: return 0; // nothing to do

    default:
        lua_pushfstring( L, "Unhandled type: %d", x->type);
        return -1;
    }

    /* Step 2: the object is inserted in its container */
    switch( x->kind) {
    case BSD_KTOPLEVEL: break;
    case BSD_KOBJFIELD: // obj, value
        if( NULL != x->fieldname) { /* if field is named, set it with textual key */
            lua_setfield( L, -2, x->fieldname); // obj
            break;
        }
        /* otherwise, apply the same as list */
        /* fall through. */
    case BSD_KLISTITEM: // idx, table, value
        CKSTACK( L, 2);
        int new_idx = lua_tointeger( L, -3) + 1;
        lua_pushinteger( L, new_idx); // idx,   table, value, idx+1
        lua_pushvalue(   L, -1);      // idx,   table, value, idx+1, idx+1
        lua_replace(     L, -5);      // idx+1, table, value, idx+1
        lua_insert(      L, -2);      // idx+1, table, idx+1, value
        lua_settable(    L, -3);      // idx+1, table
        break;
    case BSD_KMAPVALUE:       // table, key, value
        lua_settable( L, -3); // table
        break;
    case BSD_KMAPKEY: // wait for associated value
    case BSD_KCHUNK:
    case BSD_KNEWCONTAINER: break;
    }
    return 0; // success
}

/* Save n elements on top of stack in the array-part of a new table,
 * save the offset value as the table's "offset" field". */
static void hibernate( lua_State *L, int depth, int offset) {
    int i;
    lua_createtable(     L, depth, 1);     // x[1]...x[depth], t
    lua_pushinteger(     L, offset);       // x[1]...x[depth], t, offset
    lua_setfield(        L, -2, "offset"); // x[1]...x[depth], t[offset]
    for(i=0; i<depth; i++) {               // x[1]...x[depth], t
        lua_pushinteger( L, i+1);          // x[1]...x[depth], t, i+1
        lua_pushvalue(   L, i-depth-2);    // x[1]...x[depth], t, i+1, x[i+1]
        lua_settable(    L, -3);           // x[1]...x[depth], t[i+1=x[i+1]]
    }
}

/* Unpack a deserialization state packed with hibernate():
 * put back every saved element on top of stack, return the saved offset. */
static int dehibernate( lua_State *L, int idx) {
    int depth = lua_objlen( L, idx), i;        // ...t...
    lua_getfield(           L, idx, "offset"); // ...t..., t[offset]
    int offset = luaL_checkinteger( L, -1);    // ...t..., t[offset]
    lua_pop( L, 1);                            // ...t...
    CKSTACK( L, depth);
    for( i=0; i<depth; i++) {                  // ...t..., t[1]...t[i]
        lua_pushinteger( L, i+1);              // ...t..., t[1]...t[i], i+1
        lua_gettable(    L, idx);              // ...t..., t[1]...t[i], t[i+1]
    }
    return offset;
}


// TODO: protect against invalid offsets
static int deserialize_or_skip( lua_State *L, int skip) {
    bsd_ctx_t *ctx = (bsd_ctx_t *) luaL_checkudata( L, 1, MT_NAME);
    size_t len, initial_depth;
    const uint8_t *buffer;

    /* Retrieve optional args: offset and partial deserialization */
    int offset, partial;
    if( LUA_TTABLE == lua_type( L, 3)) { // partial only (no offset)
        offset = 0;
        partial = 3;
    } else if( LUA_TNUMBER == lua_type( L, 3)) { // offset, maybe partial
        offset = luaL_checkinteger( L, 3) - 1;
        if( LUA_TTABLE == lua_type( L, 4)) partial = 4; // offset and partial
        else partial = 0; // offset only
    } else if( ! lua_isnoneornil( L, 3)) {
        luaL_error( L, "offset or partial deserialization expected");
        partial = offset = 0; // never reached, removes uninitialized var warning
    } else { // no offset nor partial
        partial = offset = 0;
    }
    if( lua_istable( L, 2)) { /* Concatenate a list of strings into a single string */
        int i, n = lua_objlen( L, 2);
        lua_checkstack( L, n);
        for(i=1; i<=n; i++) {
            lua_pushinteger( L, i); // ctx, tbl_buff, ?offset, ?partial, tbl_buff[1] ... tbl_buff[i-1], i
            lua_gettable( L, 2);    // ctx, tbl_buff, ?offset, ?partial, tbl_buff[1] ... tbl_buff[i]
        }
        // ctx, tbl_buff, ?offset, ?partial, tbl_buff[1] ... tbl_buff[n]
        lua_concat( L, n); // ctx, tbl_buff, ?offset, ?partial, str_buff
        buffer = (const uint8_t *) luaL_checklstring( L, -1, &len);
    } else { // ctx, str_buff, ?offset, ?partial
        buffer = (const uint8_t *) luaL_checklstring( L, 2, &len);
    }

    // TODO: not good enough. It must be impossible for users to mix up an hibernation
    // step with the wrong deserializer, as it can cause core dumps.
    if( ! partial && ctx->stacksize) {
        luaL_error( L, "Attempt to deserialize new data with an hibernated state");
    } else if( partial && ! ctx->stacksize) {
        luaL_error( L, "Attempt to resume a deserialization with an empty deserializer");
    }

    if( len <= offset) { /* Nothing to read */
        lua_pushnil( L);
        lua_pushnumber( L, offset);
        return 2;
    }

    bsd_data_t data;

    if( skip) {
        do {
            int r = bsd_read( ctx, & data, buffer + offset, len - offset);
            if( r < 0) {
                return 0;  // end-of-string reached
            } else {
                offset += r;
            }
        } while( ctx->stacksize > 0 || data.type == BSD_CLASSDEF);
        lua_pushinteger( L, offset+1);
        return 1;

    } else { /* Don't skip, actually build the object */

        initial_depth = lua_gettop( L);

        if( partial) { offset = dehibernate( L, partial); }

        //printf("Decoding [ ");
        //unsigned char *k; for(k=buffer+offset; k<buffer+len; k++) printf("%02x ", *k);
        //printf( "]\n");

        do {
            int r = bsd_read( ctx, & data, buffer + offset, len - offset);
            if( r < 0) {
                int depth = lua_gettop( L) - initial_depth; // # of pending containers to save
                CKSTACK( L, 4);
                hibernate(      L, depth, offset); // hibernated_stack
                lua_pushnil(    L); // hibernated_stack, nil
                lua_pushstring( L, "partial"); // hibernated_stack, nil, "partial"
                lua_pushvalue(  L, -3); // hibernated_stack, nil, "partial", hibernated_stack
                return 3;
            }
            if( bysant2lua( L, &data)) {
                bsd_init( ctx);
                lua_error( L);
            }
            offset += r;
        } while( ctx->stacksize > 0 || data.type == BSD_CLASSDEF);
        lua_pushinteger( L, offset+1);
        return 2;
    }

}


/* Deserialize Bysant string or list of strings into Lua data.
 * Returns a Lua value and then the next offset.
 * Throw errors if deserialization fails. */
static int api_deserialize( lua_State *L) {
    return deserialize_or_skip( L, 0);
}

/* Skip from current offset to the offset of the next object;
 * return `nil` if end-of-string is reached before end-of-object */
static int api_skip( lua_State *L) {
    return deserialize_or_skip( L, 1);
}

static int api_collect( lua_State *L) {
    bsd_reset( (bsd_ctx_t *) luaL_checkudata( L, 1, MT_NAME));
    return 0;
}

static int api_init( lua_State *L) {
    bsd_ctx_t *ctx = (bsd_ctx_t *) lua_newuserdata( L, sizeof( bsd_ctx_t)); // udata
    lua_getfield( L, LUA_REGISTRYINDEX, MT_NAME); // mt, udata
    lua_setmetatable( L, -2); // udata
    bsd_init( ctx);
    return 1;
}

static int api_dump( lua_State *L) {
    size_t len;
    bsd_ctx_t *ctx = (bsd_ctx_t *) luaL_checkudata( L, 1, MT_NAME);
    const uint8_t *buffer = (const uint8_t *) luaL_checklstring( L, 2, &len);
    bsd_dump( ctx, stdout, buffer, len);
    return 0;
}

static int api_addClass( lua_State *L) {
    bsd_ctx_t *ctx = (bsd_ctx_t *) luaL_checkudata( L, 1, MT_NAME); // 1=>udata
    bs_class_t *classdef = lua_bs_toclassdef( L, 2);
    int r = bsd_addClass( ctx, classdef);
    if( 0 != r) free(classdef);
    if( r) { lua_pushnil( L); lua_pushinteger( L, r); return 2; }
    else { lua_pushinteger( L, r); return 1; }
}

int luaopen_m3da_bysant_core_deserialize( lua_State *L) {
    luaL_findtable( L, LUA_GLOBALSINDEX, "m3da.bysant.core", 14); // m3da.bysant.core

    //TODO: check if this m3da.niltoken 'global' is still needed! -> likely to be removed
    //same remark apply for serializer lua_open function.
    /* set m3da.niltoken if absent */
    lua_getglobal( L, "m3da");        // m3da.bysant.core, m3da
    lua_getfield( L, -1, "niltoken");   // m3da.bysant.core, m3da, ?niltoken
    if( lua_isnil(L, -1)) {
        lua_pop( L, 1);                   // m3da.bysant.core, m3da
        lua_getglobal( L, "require");     // m3da.bysant.core, m3da, require
        lua_pushstring( L, "niltoken");   // m3da.bysant.core, m3da, require, "niltoken"
        lua_call( L, 1, 1);               // m3da.bysant.core, m3da, niltoken
        lua_setfield( L, -2, "niltoken"); // m3da.bysant.core, m3da[niltoken=niltoken]
    }
    else lua_pop( L, 1); // m3da.bysant.core, m3da
    lua_pop( L, 1);       // m3da.bysant.core

    /* set niltoken in registry as shortcut*/
    //Note: maybe to be moved into bysant.core.c
    lua_getglobal( L, "require");     // m3da.bysant.core, require
    lua_pushstring( L, "niltoken");   // m3da.bysant.core, require, "niltoken"
    int pcall_res = lua_pcall( L, 1, 1, 0);               // m3da.bysant.core, niltoken or errmsg
    if (pcall_res){ //require niltoken failed
        //pop error message
        lua_pop( L, 1); // m3da.bysant.core
        //push nil as m3da.bysant 'default' niltoken
        lua_pushnil(L); // m3da.bysant.core, nil
        //Note: at some point we may want to inform somebody, somehow that we couln't use
        //'real' niltoken as this implies some limitations: M3DA null coming from server is likely to be poorly handled in this case
    }
    //store niltoken as reference in reference system, to be used by push_niltoken later on.
    niltoken_reg = luaL_ref(L, LUA_REGISTRYINDEX); // m3da.bysant.core


    luaL_newmetatable( L, MT_NAME);        // m3da.bysant.core, mt
    lua_pushcfunction( L, api_collect); lua_setfield( L, -2, "__gc");
    lua_createtable( L, 0, 3);             // m3da.bysant.core, mt, __index


    lua_pushcfunction( L, api_deserialize);// m3da.bysant.core, mt, __index, deserialize
    lua_pushvalue( L, -1);                 // m3da.bysant.core, mt, __index, deserialize, deserialize
    lua_setfield( L, -3, "deserialize");   // m3da.bysant.core, mt, __index[deserialize], deserialize
    lua_setfield( L, -3, "__call");        // m3da.bysant.core, mt[__call=deserialize], __index

    lua_pushcfunction( L, api_dump);     lua_setfield( L, -2, "dump");
    lua_pushcfunction( L, api_addClass); lua_setfield( L, -2, "addClass");
    lua_pushcfunction( L, api_skip);     lua_setfield( L, -2, "skip");

    lua_setfield( L, -2, "__index");           // m3da.bysant.core, mt[__index]
    lua_pushstring( L, "bysant.deserializer"); // m3da.bysant.core, mt, "bysant.deserializer"
    lua_setfield( L, -2, "__type");            // m3da.bysant.core, mt[__type="bysant.deserializer"]
    lua_pop( L, 1);                            // m3da.bysant.core

    lua_pushcfunction( L, api_init);       // m3da.bysant.core, deserializer
    lua_pushvalue( L, -1);                 // m3da.bysant.core, deserializer, deserializer
    lua_setfield( L, -3, "deserializer");  // m3da.bysant.core[deserializer], deserializer

    return 1; // return deserializer
}
