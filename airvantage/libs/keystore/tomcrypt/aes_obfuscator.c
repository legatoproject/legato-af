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

#include "keystore.h"
#include "tomcrypt.h"
#include <string.h>
#include "obfuscation_prekey.h"

/* Puts the obfuscation key in `obfuscation_bin_key`.
 *
 * No sensitive local variable that can be cleared.
 *
 * @param key_index the 0-based index of the key whose (des)obfuscation is
 *   requested. different key indexes should have different obfuscated
 *   representations, to avoid making it obvious when the same key is used at
 *   several places.
 * @param obfuscation_bin_key where the obfuscation/desobfuscation key is written.
 * @return CRYPT_OK (cannot fail)
 */
int get_obfuscation_bin_key( int key_index, unsigned char *obfuscation_bin_key) {
    unsigned const char unrotated_obfuscation_key[16] = OBFUSCATION_PREKEY;
    int i;
    for( i = 0; i < 16; i++) {
        obfuscation_bin_key[i] = unrotated_obfuscation_key[(i + key_index) % 16];
    }
    return CRYPT_OK;
}

/* Initializes a symmetric ECB cipher context, to obfuscate or deobfuscate keys.
 * Used by `get_plain_bin_key` and `set_plain_bin_keys`.
 *
 * Sensitive local variables to clean: none
 *
 * @param ecb_ctx the encryption context to initialize.
 * @param obfuscation_bin_key the key to use for obfuscation.
 * @return CRYPT_OK or CRYPT_ERROR.
 */
static int get_ecb_obfuscator(
        symmetric_ECB *ecb_ctx,
        unsigned const char *obfuscation_bin_key) {
    if( register_cipher( & aes_desc) == -1) return CRYPT_ERROR;

    if( ecb_start(
        find_cipher( "aes"),
        (const unsigned char*) obfuscation_bin_key,
        16, 0, ecb_ctx))
        return CRYPT_ERROR;
    else return CRYPT_OK;
}

int keystore_obfuscate( int key_index, unsigned char *obfuscated_bin_key, unsigned const char *plain_bin_key) {
    symmetric_ECB ecb_ctx;
	unsigned char obfuscation_bin_key[16];
	if( get_obfuscation_bin_key( key_index, obfuscation_bin_key)) goto cleanup;
    if( get_ecb_obfuscator( & ecb_ctx, obfuscation_bin_key)) goto cleanup;
    if( ecb_encrypt( plain_bin_key, obfuscated_bin_key, 16, & ecb_ctx)) goto cleanup;
    return 0;

    cleanup:
    memset( & ecb_ctx, '\0', sizeof( ecb_ctx));
    memset( obfuscation_bin_key, '\0', 16);
    return CRYPT_ERROR;
}

int keystore_deobfuscate( int key_index, unsigned char *plain_bin_key, unsigned const char *obfuscated_bin_key) {
    symmetric_ECB ecb_ctx;
	unsigned char obfuscation_bin_key[16];
	if( get_obfuscation_bin_key( key_index, obfuscation_bin_key)) goto cleanup;
    if( get_ecb_obfuscator( & ecb_ctx, obfuscation_bin_key)) goto cleanup;
    if( ecb_decrypt( obfuscated_bin_key, plain_bin_key, 16, & ecb_ctx)) goto cleanup;
    return 0;

    cleanup:
    memset( & ecb_ctx, '\0', sizeof( ecb_ctx));
    memset( obfuscation_bin_key, '\0', 16);
    memset( plain_bin_key, '\0', 16);
    return CRYPT_ERROR;
}
