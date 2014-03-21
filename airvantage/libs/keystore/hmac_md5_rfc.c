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
#include <hmac-md5.h>

int keystore_hmac_md5( const unsigned char *key, const unsigned char *data, size_t length, unsigned char *hash) {
    hmac_md5( (unsigned char *) data, length, (unsigned char *) key, 16, hash);
    return 0;
}


