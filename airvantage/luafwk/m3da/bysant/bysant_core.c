/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "bysant_core.h"

int luaopen_m3da_bysant_core( lua_State *L) {
    return luaopen_m3da_bysant_core_serialize( L) +
            luaopen_m3da_bysant_core_deserialize( L);
}
