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

#ifndef __BYSANT_CORE_H_INCLUDED__
#define __BYSANT_CORE_H_INCLUDED__

#include "lua.h"
#include "bysant.h"

/* Load module bysant.core.serialize. */
int luaopen_m3da_bysant_core_serialize( lua_State *L);

/* Load module bysant.core.deserialize. */
int luaopen_m3da_bysant_core_deserialize( lua_State *L);

/* Load both modules. */
int luaopen_m3da_bysant_core( lua_State *L);

/* Checks context id from given stack position.
 * Pass BS_CTXID_LAST to not set a default value.
 * Uses luaL_checkoption internally. */
bs_ctxid_t lua_bs_checkctxid( lua_State *L, int narg, bs_ctxid_t def);

/* Allocates a new bs_class_t as a single memory block from given Lua
 * table based description. It is up to caller to free allocated memory
 * when appropriate. Returned class definition is in MANAGED mode.
 *
 * The table description is as following :
 *  { name = "ClassName",
 *    id   = 1,           -- numeric class identifier
 *    -- array part are field definitions
 *    { name = "field1", context="global" },
 *    { name = "field2", context="number" }, ... }
 * If class name does not exist, so the class is considered as unnamed
 * and fields should not be named either.
 * If class name is given, so all fields must be named too.
 *
 * Can throw errors if description is not as expected but no memory is
 * allocated in that case.
 */
bs_class_t* lua_bs_toclassdef( lua_State *L, int narg);

/* Checks that a bss context wrapped in a userdata is at index `idx` of the Lua
 * stack and returns it. */
struct bss_ctx_t *lua_bss_checkctx( lua_State *L, int idx);

#endif
