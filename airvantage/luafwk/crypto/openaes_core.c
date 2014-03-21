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

#include <stddef.h>
#include <malloc.h>
#include "oaes_lib.h"
#include "lauxlib.h"
#include "keystore.h"


/* Check that value at Lua stack index `idx` is a userdata containing an aes context. */
static OAES_CTX *checkctx( lua_State *L, int idx) {
    return * (OAES_CTX **) luaL_checkudata( L, idx, "OAES_CTX");
}

/* M.new(nonce_string, key_index, initial_vector) returns aes_ctx */
static int api_new( lua_State *L) {
    size_t size_nonce;
    const uint8_t *nonce = (const uint8_t *) luaL_checklstring( L, 1, & size_nonce);
    uint8_t key_CK [128/8];
    int idx_K = luaL_checkinteger( L, 2)-1;
    size_t initial_vector_length;
    const char *initial_vector = luaL_optlstring( L, 3, NULL, & initial_vector_length);
    if( initial_vector && initial_vector_length != 128/8) {
        luaL_error( L, "Initial vector must be 16 bytes long");
    }
    OAES_CTX *ctx = oaes_alloc();
    if( ! ctx) return 0;
    get_cipher_key( nonce, size_nonce, idx_K, key_CK, 128/8);
    oaes_key_import_data( ctx, key_CK, 128/8);
    if( initial_vector) oaes_set_option( ctx, OAES_OPTION_CBC, initial_vector);
    * (OAES_CTX **) lua_newuserdata( L, sizeof( ctx)) = ctx;
    luaL_newmetatable( L, "OAES_CTX");
    lua_setmetatable( L, -2);
    return 1;
}

/* M.encrypt(aes_ctx, message_string) returns ciphered_string or nil+error_num. */
static int api_encrypt( lua_State *L) {
    OAES_CTX *ctx = checkctx( L, 1);
    size_t srclen, dstlen;
    const uint8_t *src = (const uint8_t *) luaL_checklstring( L, 2, & srclen);
    uint8_t *dst;
    oaes_encrypt( ctx, src, srclen, NULL, & dstlen); // retrieve dstlen
    dst = malloc( dstlen);
    if( ! dst) { lua_pushnil( L); lua_pushinteger( L, OAES_RET_MEM); return 2; }
    int r = oaes_encrypt( ctx, src, srclen, dst, & dstlen);
    if( r) { free( dst); lua_pushnil( L); lua_pushinteger( L, r); return 2; }
    lua_pushlstring( L, (const char *) dst, dstlen);
    free( dst);
    return 1;
}

/* M.decrypt(aes_ctx, ciphered_string) returns message_string or nil+error_num. */
static int api_decrypt( lua_State *L) {
    OAES_CTX *ctx = checkctx( L, 1);
    size_t srclen, dstlen;
    const uint8_t *src = (const uint8_t *) luaL_checklstring( L, 2, & srclen);
    uint8_t *dst;
    oaes_decrypt( ctx, src, srclen, NULL, & dstlen);
    dst = malloc( dstlen);
    if( ! dst) { lua_pushnil( L); lua_pushinteger( L, OAES_RET_MEM); return 2; }
    int r = oaes_decrypt( ctx, src, srclen, dst, & dstlen);
    if( r) { free( dst); lua_pushnil( L); lua_pushinteger( L, r); return 2; }
    lua_pushlstring( L, (const char *) dst, dstlen);
    free( dst);
    return 1;
}

/* M.close(aes_ctx) releases resources associated with AES context. */
static int api_close( lua_State *L) {
    OAES_CTX *ctx = checkctx( L, 1);
    oaes_free( & ctx);
    return 0;
}


int luaopen_crypto_openaes_core( lua_State *L) {
    lua_newtable( L);
#define REG( name) lua_pushcfunction( L, api_##name); lua_setfield( L, -2, #name)
    REG( new);
    REG( encrypt);
    REG( decrypt);
    REG( close);
#undef REG
    return 1;
}
