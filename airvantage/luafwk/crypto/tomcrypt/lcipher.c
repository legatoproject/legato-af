/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Gilles Cannenterre for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "tomcrypt.h"
#include "lauxlib.h"
#include "keystore.h"


#define MYNAME      "cipher"
#define MYVERSION   MYNAME " library for " LUA_VERSION
#define MYTYPE      MYNAME " handle"

typedef enum {
    MODE_ENC = 1,
    MODE_DEC = 2,
} EModes;

typedef enum {
    CHAIN_ECB = 1,
    CHAIN_CBC = 2,
    CHAIN_CTR = 3
} EChains;

typedef enum {
    PADDING_NONE = 1,
    PADDING_PKCS5 = 2
} EPaddings;

typedef struct SCipherDesc_ {
    EModes mode;
    size_t keysize;
    size_t nonce_size;
    int cipher_id;
    int keyidx;
    unsigned char* nonce;
    unsigned char* key;
} SCipherDesc;

typedef struct SCipherChain_ {
    EChains name;
    size_t ivsize;
    unsigned char* iv;
} SCipherChain;

typedef struct SCipherPadding_ {
    EPaddings name;
} SCipherPadding;

typedef struct SCipher_ {
    SCipherDesc desc;
    SCipherChain chain;
    SCipherPadding padding;
    int chunk_size;
    void* state;
} SCipher;

static int get_cipher_desc(lua_State* L, int index, SCipherDesc* desc) {
    if (lua_istable(L, index)) {
        lua_getfield(L, index, "name");
        const char* name = NULL;
        if (!lua_isnil(L, -1))
            name = lua_tostring(L, -1);
        if (name != NULL && strcmp(name, "aes") == 0) {
            register_cipher(&aes_desc);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "'desc.cipher' should be 'aes'");
            return 2;
        }
        desc->cipher_id = find_cipher(name);
        if (desc->cipher_id == -1) {
            lua_pushnil(L);
            lua_pushstring (L, "cannot find cipher implementation");
            return 2;
        }
        lua_pop(L, 1);

        lua_getfield(L, index, "mode");
        const char* mode = NULL;
        if (!lua_isnil(L, -1))
            mode = lua_tostring(L, -1);
        desc->mode = MODE_ENC;
        if (mode != NULL && strcmp(mode, "enc") == 0) {
            desc->mode = MODE_ENC;
        } else if (mode != NULL && strcmp(mode, "dec") == 0) {
            desc->mode = MODE_DEC;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "'desc.mode' should be 'enc' or 'dec'");
            return 2;
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "nonce");
        desc->nonce = NULL;
        if (!lua_isnil(L, -1)) {
            desc->nonce = (unsigned char*) lua_tolstring(L, -1, &(desc->nonce_size));
            lua_getfield(L, 1, "keyidx");
            desc->keyidx = -1;
            if (!lua_isnil(L, -1))
                desc->keyidx = (int)lua_tointeger(L, -1) -1; // 1-based in Lua, 0-based in C
            lua_pop(L, 1);
            if (desc->keyidx < 0) {
                lua_pushnil(L);
                lua_pushstring(L, "'desc.keyidx' should be > 0");
                return 2;
            }

            lua_getfield(L, 1, "keysize");
            desc->keysize = 16;
            if (!lua_isnil(L, -1))
                desc->keysize = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            if (desc->keysize < 1) {
                lua_pushnil(L);
                lua_pushstring(L, "'desc.keysize' should be > 0'");
                return 2;
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "key");
        desc->key = NULL;
        if (!lua_isnil(L, -1))
            desc->key = (unsigned char*) lua_tolstring(L, -1, &(desc->keysize));
        lua_pop(L, 1);
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "'desc' should be a table");
        return 2;
    }
    return 0;
}

static int get_cipher_chain(lua_State* L, int index, SCipherChain* chain) {
    if (lua_istable(L, index)) {
        lua_getfield(L, index, "name");
        const char* name = NULL;
        if (!lua_isnil(L, -1))
            name = lua_tostring(L, -1);
        chain->name = CHAIN_CTR;
        if (name != NULL && strcmp(name, "ecb") == 0) {
            chain->name = CHAIN_ECB;
        } else if (name != NULL && strcmp(name, "cbc") == 0) {
            chain->name = CHAIN_CBC;
        } else if (name != NULL && strcmp(name, "ctr") == 0) {
            chain->name = CHAIN_CTR;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "'chain.chain' should be 'ecb', 'cbc', or 'ctr'");
            return 2;
        }
        lua_pop(L, 1);

        lua_getfield(L, index, "iv");
        chain->iv = NULL;
        chain->ivsize = 0;
        if (!lua_isnil(L, -1))
            chain->iv = (unsigned char*)lua_tolstring(L, -1, &(chain->ivsize));
        lua_pop(L, 1);
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "'chain' should be a table");
        return 2;
    }
    return 0;
}

static int get_cipher_padding(lua_State* L, int index, SCipherPadding* padding) {
    if (lua_istable(L, index)) {
        lua_getfield(L, index, "name");
        const char* name = NULL;
        if (!lua_isnil(L, -1))
            name = lua_tostring(L, -1);
        if (name != NULL && strcmp(name, "none") == 0) {
            padding->name = PADDING_NONE;
        } else if (name != NULL && strcmp(name, "pkcs5") == 0) {
            padding->name = PADDING_PKCS5;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "'padding.name' should be 'none' or 'pkcs5'");
            return 2;
        }
        lua_pop(L, 1);
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "'padding' should be a table");
        return 2;
    }
    return 0;
}

static int Lnew(lua_State* L) {
    SCipher* cipher = (SCipher*) lua_newuserdata(L, sizeof(SCipher));
    cipher->state = NULL; // mark as invalid for GC
    luaL_getmetatable(L, MYTYPE);
    lua_setmetatable(L, -2);

    cipher->state = NULL;
    int param = get_cipher_desc(L, 1, &(cipher->desc));
    if (param > 0)
        return param;
    param = get_cipher_chain(L, 2, &(cipher->chain));
    if (param > 0)
        return param;

    cipher->chunk_size = cipher_descriptor[cipher->desc.cipher_id].block_length;

    unsigned char* key = NULL;
    unsigned char* keycrypt = NULL;
    SCipherDesc  *d = & cipher->desc;
    SCipherChain *c = & cipher->chain;

    if (d->nonce != NULL) {
        keycrypt = malloc(d->keysize);
        if( CRYPT_OK != get_cipher_key(d->nonce, d->nonce_size, d->keyidx, keycrypt, d->keysize)) {
            if (keycrypt != NULL)
                memset(keycrypt, 0, d->keysize);
            lua_pushnil(L);
            lua_pushstring (L, "cannot retrieve key from keystore");
            return 2;
        }
        key = keycrypt;
    } else {
        key = d->key;
    }

    int status = CRYPT_ERROR;
    switch (c->name) {
    case CHAIN_ECB:
        cipher->state = malloc(sizeof(symmetric_ECB));
        if (cipher->state != NULL && key != NULL)
            status = ecb_start(
                d->cipher_id,
                (const unsigned char*) key,
                d->keysize, 0,
                (symmetric_ECB*) cipher->state);
        break;
    case CHAIN_CBC:
        cipher->state = malloc(sizeof(symmetric_CBC));
        if (cipher->state != NULL && key != NULL && c->iv != NULL)
            status= cbc_start(
                d->cipher_id,
                (const unsigned char*) c->iv,
                (const unsigned char*) key,
                d->keysize, 0,
                (symmetric_CBC*) cipher->state);
        break;
    case CHAIN_CTR:
        cipher->state = malloc(sizeof(symmetric_CTR));
        if (cipher->state != NULL && key != NULL && c->iv != NULL)
            status = ctr_start(
                d->cipher_id,
                (const unsigned char*) c->iv,
                (const unsigned char*) key,
                d->keysize, 0, CTR_COUNTER_BIG_ENDIAN,
                (symmetric_CTR*) cipher->state);
        break;
    default:
        break;
    }
    if (keycrypt != NULL)
        memset(keycrypt, 0, d->keysize);
    if (status != CRYPT_OK) {
        lua_pushnil(L);
        lua_pushstring(L, error_to_string(status));
        return 2;
    }
    return 1;
}

static int ciphertext(SCipher* cipher, unsigned char* text, size_t text_size) {
    switch (cipher->chain.name) {
    case CHAIN_ECB:
        if (cipher->desc.mode == MODE_ENC) {
            return ecb_encrypt(text, text, text_size, (symmetric_ECB*)cipher->state);
        } else {
            return ecb_decrypt(text, text, text_size, (symmetric_ECB*)cipher->state);
        }
        break;
    case CHAIN_CBC:
        if (cipher->desc.mode == MODE_ENC) {
            return cbc_encrypt(text, text, text_size, (symmetric_CBC*)cipher->state);
        } else {
            return cbc_decrypt(text, text, text_size, (symmetric_CBC*)cipher->state);
        }
        break;
    case CHAIN_CTR:
        if (cipher->desc.mode == MODE_ENC) {
            return ctr_encrypt(text, text, text_size, (symmetric_CTR*)cipher->state);
        } else {
            return ctr_decrypt(text, text, text_size, (symmetric_CTR*)cipher->state);
        }
        break;
    default:
        break;
    }
    return CRYPT_ERROR;
}

#define CHECK(X) \
    do { \
        int status = X; \
        if (status != CRYPT_OK) { \
            lua_pushnil(L); \
            lua_pushstring(L, error_to_string(status)); \
            return 2; \
        } \
    } while (0)

static int Lprocess(lua_State* L) {
    SCipher* cipher = luaL_checkudata(L, 1, MYTYPE);
    size_t text_size;
    const char* text = luaL_checklstring(L, 2, &text_size);
    unsigned char* copy = (unsigned char*) lua_newuserdata(L, text_size);   // buffer copy
    memcpy(copy, text, text_size);
    CHECK(ciphertext(cipher, copy, text_size));                             // push copy
    lua_pushlstring(L, (const char *) copy, text_size);
    return 1;
}

static int Ldone(lua_State* L) {
    SCipher* cipher = luaL_checkudata(L, 1, MYTYPE);
    if (cipher->state == NULL) {
        return 0;
    }

    switch (cipher->chain.name) {
    case CHAIN_ECB:
        ecb_done((symmetric_ECB*)cipher->state);
        break;
    case CHAIN_CBC:
        cbc_done((symmetric_CBC*)cipher->state);
        break;
    case CHAIN_CTR:
        ctr_done((symmetric_CTR*)cipher->state);
        break;
    default:
        break;
    }
    free(cipher->state);
    return 0;
}

static int Ltostring(lua_State* L) {
    SCipher* p = luaL_checkudata(L, 1, MYTYPE);
    lua_pushfstring(L, "%s %p", MYTYPE, (void*) p);
    return 1;
}

static int aes_filter_dec(lua_State* L) {
    SCipher* cipher = luaL_checkudata(L, lua_upvalueindex(1), MYTYPE);
    unsigned char* partial = lua_touserdata(L, lua_upvalueindex(2));
    int partial_size = lua_tointeger(L, lua_upvalueindex(3));
    if (lua_isnil(L, 1)) /* chunk == nil */{
        // printf("\ndec[nil]");
        // cipher if data in partial
        if (partial_size > 0) {
            CHECK(ciphertext(cipher, partial, partial_size));
            // apply padding
            switch (cipher->padding.name) {
            case PADDING_PKCS5:
                if ((cipher->chunk_size - partial[cipher->chunk_size-1]) > 0) {
                    lua_pushlstring(L, (const char *) partial, (size_t) (cipher->chunk_size - partial[cipher->chunk_size-1]));
                }
                break;
            default:
            case PADDING_NONE:
                lua_pushlstring(L, (const char *) partial, partial_size);
                break;
            }
        } else {
            lua_pushnil(L);
        }
        partial_size = -1;
    } else /* chunk */ {
        size_t pt_size;
        unsigned char* pt = (unsigned char*) luaL_checklstring(L, 1, &pt_size);
        // printf("\ndec[%p:%d]", pt, pt_size);
        int size_tmp;
        if (partial_size > 0) {
            size_tmp = MIN(pt_size, cipher->chunk_size - partial_size);
            if (size_tmp > 0) {
                memcpy(partial + partial_size, pt, size_tmp);
                partial_size += size_tmp;
                pt_size -= size_tmp;
                pt += size_tmp;
            }
        }
        luaL_Buffer b;
        luaL_buffinit(L, &b);
        if ((partial_size == cipher->chunk_size) && (pt_size > 0)) {
            CHECK(ciphertext(cipher, partial, partial_size));
            luaL_addlstring(&b, (const char *) partial, partial_size);
            partial_size = 0;
        }
        if (pt_size > 0) {
            size_tmp = ((int)(pt_size/cipher->chunk_size)) * cipher->chunk_size;
            partial_size = pt_size - size_tmp;
            if (partial_size == 0) {
                size_tmp -= cipher->chunk_size;
                partial_size = cipher->chunk_size;
            }
            memcpy(partial, pt + size_tmp, partial_size);
            if (size_tmp > 0) {
                CHECK(ciphertext(cipher, pt, size_tmp));
                luaL_addlstring(&b, (const char *) pt, size_tmp);
            }
        }
        luaL_pushresult(&b);
    }
    lua_pushinteger(L, partial_size);
    lua_replace(L, lua_upvalueindex(3));
    return 1;
}

static int aes_filter_enc(lua_State* L) {
    SCipher* cipher = luaL_checkudata(L, lua_upvalueindex(1), MYTYPE);
    unsigned char* partial = lua_touserdata(L, lua_upvalueindex(2));
    int partial_size = lua_tointeger(L, lua_upvalueindex(3));
    if (lua_isnil(L, 1)) /* chunk == nil */{
        // printf("\nenc[nil]");
        // apply padding
        if (partial_size >= 0) {
            switch (cipher->padding.name) {
            case PADDING_PKCS5:
                memset(partial + partial_size, cipher->chunk_size - partial_size, cipher->chunk_size - partial_size);
                partial_size = cipher->chunk_size;
                break;
            default:
            case PADDING_NONE:
                break;
            }
        }
        // cipher if data in partial
        if (partial_size > 0) {
            CHECK(ciphertext(cipher, partial, partial_size));
            lua_pushlstring(L, (const char *) partial, partial_size);
        } else {
            lua_pushnil(L);
        }
        partial_size = -1;
    } else /* chunk */ {
        size_t pt_size;
        unsigned char* pt = (unsigned char*) luaL_checklstring(L, 1, &pt_size);
        // printf("\nenc[%p:%d]", pt, pt_size);
        int size_tmp;
        if (partial_size > 0) {
            size_tmp = MIN(pt_size, cipher->chunk_size - partial_size);
            if (size_tmp > 0) {
                memcpy(partial + partial_size, pt, size_tmp);
                partial_size += size_tmp;
                pt_size -= size_tmp;
                pt += size_tmp;
            }
        }
        luaL_Buffer b;
        luaL_buffinit(L, &b);
        if (partial_size == cipher->chunk_size) {
            CHECK(ciphertext(cipher, partial, partial_size));
            luaL_addlstring(&b, (const char *) partial, partial_size);
            partial_size = 0;
        }
        if (pt_size > 0) {
            size_tmp = ((int)(pt_size/cipher->chunk_size)) * cipher->chunk_size;
            partial_size = pt_size - size_tmp;
            if (partial_size > 0) {
                memcpy(partial, pt + size_tmp, partial_size);
            }
            if (size_tmp > 0) {
                CHECK(ciphertext(cipher, pt, size_tmp));
                luaL_addlstring(&b, (const char *) pt, size_tmp);
            }
        }
        luaL_pushresult(&b);
    }
    lua_pushinteger(L, partial_size);
    lua_replace(L, lua_upvalueindex(3));
    return 1;
}

/** Lfilterdec(userdata, s) */
static int Lfilter(lua_State* L) {
    SCipher* cipher = luaL_checkudata(L, 1, MYTYPE);
    int param = get_cipher_padding(L, 2, &(cipher->padding));
    if (param > 0)
        return param;
    lua_remove(L, 2);                               // userdata cipher
    lua_newuserdata(L, cipher->chunk_size);         // partial buffer
    lua_pushinteger(L, 0);                          // partial size
    if (cipher->desc.mode == MODE_ENC) {
        lua_pushcclosure (L, aes_filter_enc, 3);    // encoder filter
    } else {
        lua_pushcclosure (L, aes_filter_dec, 3);    // decoder filter
    }
    return 1;
}

/** Lwrite(first_idx, keys) */
static int Lwrite( lua_State *L) {
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

static const luaL_Reg R[] = {
        { "__gc", Ldone },
        { "process", Lprocess },
        { "filter", Lfilter },
        { "new", Lnew },
        { "tostring", Ltostring },
        { "write", Lwrite },
        { NULL, NULL } };

int luaopen_crypto_cipher(lua_State* L) {
    luaL_newmetatable(L, MYTYPE);
    lua_setglobal(L, MYNAME);
    luaL_register(L, MYNAME, R);
    lua_pushliteral(L, "version");
    lua_pushliteral(L, MYVERSION);
    lua_settable(L, -3);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    lua_pushliteral(L, "__tostring");
    lua_pushliteral(L, "tostring");
    lua_gettable(L, -3);
    lua_settable(L, -3);
    return 1;
}
