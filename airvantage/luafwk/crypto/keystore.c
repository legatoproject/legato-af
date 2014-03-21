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

#include "lauxlib.h"
#include "keystore.h"
#include <malloc.h>
#include <string.h>

/** Lwrite(first_idx, keys) */
static int api_write( lua_State *L) {
    int first_idx = luaL_checkint( L, 1), n_keys;
    char *data;
    char *allocated = NULL;
    if( lua_isstring( L, 2)) { // first_key, key_str
        data = (char *) luaL_checkstring( L, 2);
        n_keys = 1;
    } else {
        if( ! lua_istable( L, 2)) {
            luaL_error( L, "2nd arg must be a string or a table of strings");
        }
        n_keys = lua_objlen( L, 2);        // n_keys, key_table
        data = allocated = malloc( n_keys * 16);
        if( ! data) {
            lua_pushnil( L);
            lua_pushstring( L, "not enough memory");
            return 2;
        }
        int i; for( i=0; i<n_keys; i++) {
            lua_pushinteger( L, i+1); // n_keys, key_table, i+1
            lua_gettable( L, -2); // n_keys, key_table, key_table[i+1]
            size_t len;
            const char *str = luaL_checklstring( L, -1, & len); // n_keys, key_table, key_table[i+1]
            if( len != 16) {
                free( (void *) data);
                luaL_error( L, "keys must be 16 characters long");
            }
            lua_pop( L, 1); // n_keys, key_table, key_table[i+1]
            memcpy( data + 16 * i, str, 16);
        }
    }
    /* first_idx+1 because Lua counts from 1 whereas C counts from 0. */
    int status = set_plain_bin_keys( first_idx-1, n_keys, (const unsigned char *) data);
    if( allocated) free( allocated);
    if( status) { lua_pushnil( L); lua_pushstring( L, "Crypto error"); return 2; }
    else { lua_pushboolean( L, 1); return 1; }
}

int luaopen_crypto_keystore( lua_State *L) {
    lua_pushcfunction( L, api_write);
    return 1;
}
