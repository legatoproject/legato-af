/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef LOGSTORE_H_
#define LOGSTORE_H_

#include "lua.h"
#include "lauxlib.h"


//Lua utilities

#define LUA_RETURN_ERROR(ERROR_STRING) \
  do{ \
    lua_pushnil(L); \
    lua_pushstring(L, ERROR_STRING); \
    return 2; \
  }while(0)


//check field type from table located at specified stack index
//field is pushed on top (may be nil)
#define LUA_CHECK_FIELD_TYPE(TABLE_INDEX, FIELD_NAME, LUATYPE, ERROR_PREFIX) \
  do{ \
    lua_getfield(L, TABLE_INDEX, FIELD_NAME); \
    if (lua_type(L, -1) != LUATYPE) \
      LUA_RETURN_ERROR(ERROR_PREFIX": Provided parameter is not correct for field: "FIELD_NAME", expected to be of type: " #LUATYPE); \
  } while(0)



//Lua module API
int luaopen_log_store(lua_State* L);

#endif /* LOGSTORE_H_ */
