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
 *
 *    Fabien Fleutot for Sierra Wireless - initial API and implementation
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tomcrypt.h>
#include <lauxlib.h>

#define ENTROPY 128

#define MYNAME      "ecdh"
#define MYVERSION   MYNAME " library for " LUA_VERSION " / May 2011"

/* TODO: determine appropriate size */
#define BUFF_SIZE 1000

/** Generates and returns a new (privkey, pubkey) ECDH key pair.
 * Keys are represented as Lua strings, the private one under a libtomcrypt
 * proprietary format, the public one under X9.63 format. */
static int lnew( lua_State *L) {
    prng_state prng;
    int prng_initialized = 0;
    ecc_key key;

    int idx = find_prng("fortuna");
    if( -1 == idx) goto failure;
    if( CRYPT_OK != rng_make_prng( ENTROPY, idx, & prng, NULL)) goto failure;
    prng_initialized=1;

    /* Generate the 512 bits ECC key in privkey. */
    if( CRYPT_OK != ecc_make_key( & prng, idx, 64, & key)) goto failure;

    /* Buffer will hold both the private and public key transiently,
     * until each of them is transformed into a Lua string. */
    unsigned char buff [BUFF_SIZE];
    unsigned long buff_len = BUFF_SIZE;

    /* Push the string representation of privkey (tomcrypt's proprietary format). */
    if( CRYPT_OK != ecc_export( buff, & buff_len, PK_PRIVATE, & key)) goto failure;
    lua_pushlstring( L, (const char*) buff, buff_len);

    /* Push the string representation of pubkey (ANSI X9.63 format, which only
     * supports public keys. This is the format expected by the server). */
    if( CRYPT_OK != ecc_ansi_x963_export( & key, buff, & buff_len)) goto failure;
    lua_pushlstring( L, (const char *) buff, buff_len);

    fortuna_done( & prng);
    return 2;

    failure:
    /* TODO: release resources */
    if( prng_initialized) fortuna_done( & prng);
    lua_pushnil( L);
    lua_pushstring( L, "error");
    return 2;
}

/** M.getsecret(my_privkey, peer_pubkey)
 * Keys are represented as Lua strings, the private one under a libtomcrypt
 * proprietary format, the public one under X9.63 format.
 * The public one normally comes from the distant computer with which we want
 * to establish a shared secret. */
static int lgetsecret( lua_State *L) {

    /* Retrieve private key */
    ecc_key my_privkey;
    size_t my_privkey_len;
    const char *my_privkey_str = luaL_checklstring( L, 1, & my_privkey_len);
    if( CRYPT_OK != ecc_import( (unsigned char *) my_privkey_str, my_privkey_len, & my_privkey)) goto failure;

    /* Retrieve public key */
    ecc_key peer_pubkey;
    size_t peer_pubkey_len;
    const char *peer_pubkey_str = luaL_checklstring( L, 2, & peer_pubkey_len);
    if( CRYPT_OK != ecc_ansi_x963_import( (unsigned char *) peer_pubkey_str, peer_pubkey_len,& peer_pubkey)) goto failure;

    /* Retrieve secret */
    unsigned char buff [BUFF_SIZE];
    unsigned long buff_len = -1;
    if( CRYPT_OK != ecc_shared_secret( & my_privkey, & peer_pubkey, buff, & buff_len)) goto failure;
    lua_pushlstring( L, (const char *) buff, buff_len);
    return 1;

    failure:
    lua_pushnil( L);
    lua_pushstring( L, "error");
    return 2;
}


static const luaL_Reg R[] = {
        { "new", lnew },
        { "getsecret", lgetsecret },
        { NULL, NULL } };

int luaopen_crypto_ecdh( lua_State* L) {

#if 0 //probably useless, to be delegated to lrng
    /* Register random number generator */
    if( -1 == register_prng( & fortuna_desc))
        luaL_error(L, "'fortuna' registration has failed\n");
#endif

    /* Register big math lib for tomcrypt */
    ltc_mp = ltm_desc;

    luaL_register(L, MYNAME, R);
    lua_pushliteral(L, "version");
    lua_pushliteral(L, MYVERSION);
    lua_settable(L, -3);
    return 1;
}
