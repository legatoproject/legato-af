#ifndef LPACK_H
#define LPACK_H

#include "lua.h"

#ifndef LPACK_API
#define LPACK_API extern
#endif


LPACK_API int luaopen_pack(lua_State *L);

#endif

