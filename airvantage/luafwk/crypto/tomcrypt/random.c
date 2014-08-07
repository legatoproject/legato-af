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
 *     Gilles Cannenterre for Sierra Wireless - initial API and implementation
 *******************************************************************************/
#include "tomcrypt.h"
#include "lua.h"
#include "lauxlib.h"

#define ENTROPY 128

static prng_state prng;
static int prng_initialized = 0;

/** new() */
static int prng_init() {
    int idx = find_prng("fortuna");
    if (idx == -1) { return -1; }
    if( CRYPT_OK != rng_make_prng(ENTROPY, idx, & prng, NULL)) { return -2; }
    return 0;
}

/** read(userdata, [size], [raw]) */
static int read( int size, char *buffer) {
	if( ! prng_initialized) {
		if( prng_init()) return -1;
		prng_initialized = 1;
	}
    int r = fortuna_read( (unsigned char *) buffer, (unsigned long) size, & prng);
    if( r == size) return 0; else return -2;
}

static int api_random( lua_State *L) {
	int size = luaL_checkint( L, 1);
	char *buffer  = malloc( size);
	if( ! buffer) { lua_pushnil( L); lua_pushstring( L, "memory"); return 2; }
	if( read( size, buffer)) { lua_pushnil( L); lua_pushstring( L, "fortuna"); return 2; }
	lua_pushlstring( L, buffer, size);
	return 1;
}
int luaopen_crypto_random(lua_State* L) {
    if (register_prng(&fortuna_desc) == -1)
        luaL_error(L, "'fortuna' registration has failed\n");
    lua_pushcfunction( L, api_random);
    return 1;
}
