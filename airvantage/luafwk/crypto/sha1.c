/*******************************************************************************
 *
 * Copyright (c) 2013 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 *
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *
 *    Fabien Fleutot for Sierra Wireless - initial API and implementation
 *
 ******************************************************************************/

/* Streaming implementation of SHA1 for Lua.
 *
 * Usage:
 *
 *    sha1 = require 'crypto.sha1'
 *    sha1_text1_text2  = sha1() :update(text1) :update(text2) :digest()
 */

#include <strings.h>
#include <stddef.h>
#include <malloc.h>
#include "sha1.h"
#include "lauxlib.h"

#define DIGEST_LEN     16
#define HEX_DIGEST_LEN (2*DIGEST_LEN)


static const char figures[]="0123456789abcdef";

static void bin2hex( char hex[HEX_DIGEST_LEN], unsigned char bin[DIGEST_LEN]) {
    int i;
    for( i=0; i<DIGEST_LEN; i++) {
        int byte = bin[i];
        hex[2*i]   = figures[byte>>4];
        hex[2*i+1] = figures[byte&0xf];
    }
}

/* Check that value at Lua stack index `idx` is a userdata containing sha1 context. */
static SHA1Context *checksha1 ( lua_State *L, int idx) {
    return (SHA1Context *) luaL_checkudata( L, idx, "SHA1_CTX");
}

/* sha1() returns an MD5 ctx as userdata. */
static int api_sha1( lua_State *L) {
    SHA1Context *ctx = lua_newuserdata( L, sizeof( SHA1Context));
    luaL_newmetatable( L, "SHA1_CTX");
    lua_setmetatable( L, -2);
    SHA1Reset( ctx);
    return 1;
}


/* sha1_ctx:update(data) returns sha1_ctx, processes data. */
static int api_sha1_update( lua_State *L) {
    SHA1Context *ctx = checksha1 ( L, 1);
    size_t datalen;
    unsigned char *data = (unsigned char *) luaL_checklstring( L, 2, & datalen);
    SHA1Input( ctx, data, datalen);
    lua_pushvalue( L, 1);
    return 1;
}

/* sha1_ctx:digest() returns the MD5 of all data passed to update().
 * Digest is returns as a binary string if `b` is true,
 * as an hexadecimal string if it's false. */
static int api_sha1_digest( lua_State *L) {
    SHA1Context *ctx = checksha1 ( L, 1);
    unsigned char digest[DIGEST_LEN];
    SHA1Result(ctx, digest);
    if( lua_toboolean( L, 2)) {
        lua_pushlstring( L, (const char *) digest, DIGEST_LEN);
    } else {
        char hex[HEX_DIGEST_LEN];
        bin2hex( hex, digest);
        lua_pushlstring( L, (const char *) hex, HEX_DIGEST_LEN);
    }
    return 1;
}

/* Helper for ltn12 filter. */
static int sha1_filter_closure( lua_State *L) {
    SHA1Context *ctx = checksha1 ( L, lua_upvalueindex( 1));
    if( ! lua_isnil( L, 1)) {
        size_t size;
        const char *data = luaL_checklstring( L, 1, & size);
        SHA1Input( ctx, (unsigned char *) data, size);
        lua_pushvalue( L, 1);
        return 1;
    }
    /* return the original argument, unmodified. */
    lua_settop( L, 1);
    return 1;
}

/* Returns an ltn2 filter, which lets data go through it unmodified but updates the sha1 context. */
static int api_sha1_filter( lua_State *L) {
    checksha1 ( L, 1);
    lua_settop( L, 1);
    lua_pushcclosure( L, sha1_filter_closure, 1);
    return 1;
}

int luaopen_crypto_sha1( lua_State *L) {
#define REG(c_name, lua_name) lua_pushcfunction( L, c_name); lua_setfield( L, -2, lua_name)

    /* Build metatable. */
    luaL_newmetatable( L, "SHA1_CTX"); // sha1_mt
    lua_newtable( L);                  // sha1_mt, hmac_index
    lua_pushvalue( L, -1);             // sha1_mt, hmac_index, hmac_index
    lua_setfield( L, -3, "__index");   // sha1_mt[__index=hmac_index], hmac_index
    REG( api_sha1_update, "update");
    REG( api_sha1_digest, "digest");
    REG( api_sha1_filter, "filter");
    lua_pop( L, 2);// -

    /* Return constructor. */
    lua_pushcfunction( L, api_sha1);
    return 1;
#undef REG
}


