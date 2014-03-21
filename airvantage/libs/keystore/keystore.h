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
 *    Fabien Fleutot     for Sierra Wireless - initial API and implementation
 *
 ******************************************************************************/

#ifndef _KEYSTORE_H_
#define _KEYSTORE_H_

#include <stdio.h>

/* Where obfuscated keys are stored, relative to LUA_AF_RW_PATH. */
#define KEYSTORE_FILE_NAME "crypto/crypto.key"

int get_cipher_key(const unsigned char* nonce, size_t size_nonce, int idx_K, unsigned char* key_CK, int size_CK);
int get_plain_bin_key(int key_index, unsigned char* key);
int set_plain_bin_keys(int first_index, int n_keys, unsigned const char *plain_bin_keys);

/* Porting functions. */
int keystore_obfuscate( int index, unsigned char *obfuscated_bin_key, const unsigned char *plain_bin_key);
int keystore_deobfuscate( int key_index, unsigned char *plain_bin_key, const unsigned char *obfuscated_bin_key);
int keystore_hmac_md5( const unsigned char *key, const unsigned char *data, size_t length, unsigned char *hash);

#endif
