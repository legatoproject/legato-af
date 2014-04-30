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
#include <string.h>

int keystore_obfuscate( int index, unsigned char *obfuscated_bin_key, unsigned const char *plain_bin_key) {
    memcpy( obfuscated_bin_key, plain_bin_key, 16);
    return 0;
}

int keystore_deobfuscate( int key_index, unsigned char *plain_bin_key, unsigned const char *obfuscated_bin_key) {
    memcpy( plain_bin_key, obfuscated_bin_key, 16);
    return 0;
}
