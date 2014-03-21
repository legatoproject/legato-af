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

/* Lua wrapper around the ISAAC random number generator embedded in OpenAES. */

#include <isaac/rand.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include "lauxlib.h"

/* Build a random seed for pseudo-random generator. */
static void get_seed( char buf[RANDSIZ + 1] )
{
    struct timeb timer;
    struct tm *gmTimer;
    char * _test = NULL;

    ftime (&timer);
    gmTimer = gmtime( &timer.time );
    _test = (char *) calloc( sizeof( char ), timer.millitm );
    sprintf( buf, "%04d%02d%02d%02d%02d%02d%03d%p%d",
        gmTimer->tm_year + 1900, gmTimer->tm_mon + 1, gmTimer->tm_mday,
        gmTimer->tm_hour, gmTimer->tm_min, gmTimer->tm_sec, timer.millitm,
        _test + timer.millitm, getpid() );

    if( _test )
        free( _test );
}

/* Initialize ISAAC pseudo-random generator. */
static void rand_init( randctx *rctx) {
    char _seed[RANDSIZ + 1];
    get_seed( _seed );
    memset( rctx->randrsl, 0, RANDSIZ );
    memcpy( rctx->randrsl, _seed, RANDSIZ );
    randinit( rctx, TRUE);
}

/* isaac(n) returns a string of `n` random bytes, or nil+error msg. */
static int api_isaac( lua_State *L) {
    int nbytes = luaL_checkinteger( L, 1);
    int roundedup = (nbytes + 3) & ~3;
    int i;
    unsigned char *result = malloc( roundedup);
    if( ! result) { lua_pushnil( L); lua_pushliteral( L, "memory"); return 2; }
    static int initialized = 0;
    static struct randctx ctx;
    if( ! initialized) {
        rand_init( & ctx);
        initialized = 1;
    }
    for( i=0; i<roundedup; i+=4) {
        unsigned int rand_word = rand( & ctx);
        result[i]   = rand_word >> 24;
        result[i+1] = (rand_word >> 16) & 0xff;
        result[i+2] = (rand_word >> 8) & 0xff;
        result[i+3] = rand_word & 0xff;
    }
    lua_pushlstring( L, (const char *) result, nbytes);
    free( result);
    return 1;
}

int luaopen_crypto_random( lua_State *L) {
    lua_pushcfunction( L, api_isaac);
    return 1;
}
