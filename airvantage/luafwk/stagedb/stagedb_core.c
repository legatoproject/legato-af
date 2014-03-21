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
#include "stagedb.h"
#include "bysant_core.h"
#include "lua.h"
#include "lauxlib.h"
#include <stdarg.h>
#include <string.h>

#define MT_NAME "stagedb_table"

static const char *const state_names [] = {
        "unconfigured", "reading", "serializing", "broken"
};
static const char *const storage_options [] = {
        "ram",
#ifdef SDB_FLASH_SUPPORT
        "flash",
#endif
#ifdef SDB_FILE_SUPPORT
        "file",
#endif
        NULL
};
static const char *const consolidation_methods [] = {
        "first", "last", "max", "mean", "median", "middle", "min", "sum", NULL
};
static const char *const serialization_methods [] = {
        "fastest",
        "smallest",
        "list",
        "deltasvector",
        "quasiperiodicvector",
        NULL
};

/* Convert a numeric error into nil + lua string. */
static int push_sdb_error( lua_State *L, int n) {
  lua_pushnil( L);
  switch( n) {
#define CASE(x) case SDB_E##x: lua_pushstring( L, #x); break
  CASE( OK);
  CASE( BADSTATE);
  CASE( TOOBIG);
  CASE( INVALID);
  CASE( MEM);
  CASE( NOCONS);
  CASE( BADFILE);
  CASE( NILFORBIDDEN);
  CASE( FULL);
  CASE( EMPTY);
  CASE( INTERNAL);
#undef CASE
  default:
    lua_pushinteger( L, n);
  }
  return 2;
}

// some error handling helpers: these are more flexible adaptation of some luaL_check* functions

/* Make a formated error message for the narg argument */
#define lua_sdb_fargerror(L, narg, msg, ...) (luaL_argerror( L, narg, lua_pushfstring( L, msg, __VA_ARGS__)))

/* Same as luaL_checkoption but does not raise error, returns the index of NULL value if not found or not string */
static int lua_sdb_getoption( lua_State *L, int idx, const char *const lst[]) {
    int i;
    if( lua_isstring( L, idx)) {
        const char *name = lua_tostring(L, idx);
        for( i=0; lst[i]; i++) {
            if( strcmp( lst[i], name) == 0) return i;
        }
        return i;
    } else {
        for( i=0; lst[i]; i++); // search last index
        return i;
    }
}

/* Get the field `field` from column spec table at index idx and check it with lua_sdb_getoption, can raise
 * argument error (uses narg for column spec table position and colidx for the position in that table). */
static int lua_sdb_checkoptionfield( lua_State *L, int idx, const char *field,
        const char *const lst[], int narg, int colidx) {
    lua_getfield( L, idx, field);
    int option = lua_sdb_getoption(L, -1, lst);
    lua_pop( L, 1);

    if( !lst[option]) {
        return lua_sdb_fargerror( L, narg, "invalid option for [%d][" LUA_QS "]", colidx, field);
    }
    return option;
}

/* Same as above for numbers. */
static lua_Number lua_sdb_checknumberfield( lua_State *L, int idx, const char *field, int narg, int colidx) {
    lua_Number d;
    lua_getfield( L, idx, field);
    if( !lua_isnumber(L, -1)) {
        lua_sdb_fargerror(L, narg, "Expected number for [%d][" LUA_QS "], got %s.",
                colidx, field, lua_typename( L, lua_type( L, idx)));
    }
    d = lua_tonumber(L, idx);
    lua_pop(L, 1);
    return d;
}


sdb_table_t *lua_sdb_checktable( lua_State *L, int idx) {
  return (sdb_table_t *) luaL_checkudata( L, idx, MT_NAME);
}

int lua_sdb_serialize( lua_State *L, int idx, sdb_table_t *tbl) {
    switch( lua_type( L, idx)) {
    case LUA_TNIL:     return sdb_null( tbl);
    case LUA_TBOOLEAN: return sdb_bool( tbl, lua_toboolean( L, idx));
    case LUA_TSTRING: {
        size_t len; const char *data;
        data = luaL_checklstring( L, idx, & len);
        return sdb_lstring( tbl, data, len);
    }
    case LUA_TNUMBER: {
        lua_Number n = luaL_checknumber( L, idx);
        return sdb_number( tbl, n);
    }
    default:
        return BSS_EINVALID;
    }
}

static sdb_ncolumn_t gettablencolumns( lua_State *L, int idx) {
    sdb_ncolumn_t ncolumns;
    for (ncolumns=0, lua_pushnil( L); lua_next( L, idx);) {
        lua_pop( L, 1); ncolumns++;
    }
    return ncolumns;
}

/* Parses and pops a column specification table.
 * After the call, all out parameters are fixed and the column name is left at the top of the stack
 * (in place of table) in order to assure it is not GCed.
 * Parameters narg (column specification table position) and colidx (index in column specification
 * table) are used only for error messages.
 * Raises errors in case of errors.
 */
static void lua_sdb_getcolumnspec( lua_State *L, double *arg, enum sdb_serialization_method_t *s_method,
        enum sdb_consolidation_method_t *c_method, int narg, int colidx) {

    *s_method = lua_sdb_checkoptionfield( L, -1, "serialization", serialization_methods, narg, colidx);

    if( *s_method == SDB_SM_SMALLEST) {
        lua_getfield( L, -1, "factor"); // ..., colspec, factor
        if( lua_isnumber( L, -1)) {
            *s_method |= SDB_SM_FIXED_PRECISION;
            *arg = lua_tonumber( L, -1);
        }
        lua_pop( L, 1);  // ..., colspec
    } else if( *s_method == SDB_SM_DELTAS_VECTOR) {
        *arg = lua_sdb_checknumberfield( L, -1, "factor", narg, colidx);
    } else if( *s_method == SDB_SM_QUASIPERIODIC_VECTOR) {
        *arg = lua_sdb_checknumberfield( L, -1, "period", narg, colidx);
    }

    lua_getfield( L, -1, "asfloat"); // ..., colspec, asfloat
    if( lua_toboolean( L, -1)) {
        *s_method |= SDB_SM_4_BYTES_FLOATS;
    }
    lua_pop( L, 1); // ..., colspec

    if( NULL != c_method) {
        *c_method = lua_sdb_checkoptionfield( L, -1, "consolidation", consolidation_methods, narg, colidx);
    }

    lua_getfield( L, -1, "name"); // ..., colspec, name
    if( !lua_isstring(L, -1)) {
        lua_sdb_fargerror(L, narg, "Expected string for [%d][" LUA_QS "], got %s.",
                "name", colidx, lua_typename( L, lua_type( L, -1)));
    }
    luaL_checktype( L, -1, LUA_TSTRING);
    lua_replace( L, -2); // ..., name
}

// TODO: integrate storage scheme into ID?
// TODO: do it at C level?
// sdb.init(id, "ram"|"flash", { <colspec>, ... })
// <colspec> : see stagedb Lua documentation
static int api_init( lua_State *L) {
    struct sdb_table_t *tbl;
    const char *id;
    enum sdb_storage_kind_t storage;
    int r, i;
    sdb_ncolumn_t ncolumns;

    // id, storage, columns
    id       = luaL_checkstring(   L, 1);
    storage  = luaL_checkoption(   L, 2, NULL, storage_options);
    tbl      = lua_newuserdata(    L, sizeof( *tbl));
    ncolumns = lua_objlen(         L, 3);
    // id, storage, columns, udata

    r = sdb_initwithoutcolumns( tbl, id, ncolumns, storage);
    if( r) { lua_pushnil( L); lua_pushinteger( L, r); return 2; }

    lua_getfield(     L, LUA_REGISTRYINDEX, MT_NAME); // id, storage, columns, udata, mt
    lua_setmetatable( L, -2); // id, storage, columns, udata

    luaL_checktype( L, 3, LUA_TTABLE);

    for(i=1, lua_rawgeti( L, 3, 1); !lua_isnil( L, -1); lua_rawgeti( L, 3, ++i)) { // id, storage, columns, udata, val
        const char *colname;
        enum sdb_serialization_method_t method;
        double arg;
        if( lua_isstring( L, -1)) {
            method  = SDB_DEFAULT_SERIALIZATION_METHOD;
            arg = 0.0;
        } else if ( lua_istable(L, -1)) {
            lua_sdb_getcolumnspec( L, &arg, &method, NULL, 3, i); // id, storage, columns, udata, colname
        } else {
            lua_sdb_fargerror( L, 3, "wrong descriptor for column %d (expected a table or a string)", i);
        }
        colname = lua_tostring( L, -1);
        sdb_setcolumn( tbl, colname, method, arg);
        lua_pop( L, 1);  // id, storage, columns, udata
    }
    // id, storage, columns, udata, nil
    lua_pop( L, 1);  // id, storage, columns, udata
    return 1;
}

/* sdb :newconsolidation(id, storage, { col1=CM1, ..., coln=CMn })
 * sdb :newconsolidation(id, storage, { <columnspec>, ... })
 * CM are Consolidation Methods. <columnspec> are standard column
 * specifications with the `consolidation` field set.
 *
 * Column names are shared between src and dst: the column src.foobar,
 * if it is consolidated, is consolidated in dst.foobar.
 *
 * The dst consolidation table is created on the fly, rather than passed
 * as a parameter. */
static int api_newconsolidation( lua_State *L) {
    struct sdb_table_t *src, *dst;
    const char *id;
    enum sdb_storage_kind_t storage;
    int r, i, ncolumns;

    // src, id, storage, columns
    src      = lua_sdb_checktable( L, 1);
    id       = luaL_checkstring(   L, 2);
    storage  = luaL_checkoption(   L, 3, NULL, storage_options);
    ncolumns = gettablencolumns(   L, 4);
    dst      = lua_newuserdata(    L, sizeof( *dst));
    // src, id, storage, columns, dst

    r = sdb_initwithoutcolumns( dst, id, ncolumns, storage);
    if( r) { lua_pushnil( L); lua_pushinteger( L, r); return 2; }

    lua_getfield(     L, LUA_REGISTRYINDEX, MT_NAME); // id, storage, columns, udata, mt
    lua_setmetatable( L, -2); // id, storage, columns, udata

    luaL_checktype( L, 4, LUA_TTABLE);

    r = sdb_setconstable( src, dst);
    if( r) {
      sdb_close( dst);
      return push_sdb_error( L, r);
    }

    if( lua_objlen(L, 4) == ncolumns) {
        // full column description sequence
        for(i=1, lua_rawgeti( L, 4, 1); !lua_isnil( L, -1); lua_rawgeti( L, 4, ++i)) { // src, id, storage, columns, dst, val
            const char *colname;
            enum sdb_serialization_method_t s_method;
            enum sdb_consolidation_method_t c_method;
            double arg;
            sdb_ncolumn_t src_col;

            lua_sdb_getcolumnspec( L, &arg, &s_method, &c_method, 4, i); // src, id, storage, columns, dst, colname
            colname = lua_tostring( L, -1);
            src_col = sdb_getcolnum( src, colname);
            if( SDB_NCOLUMN_INVALID == src_col) {
                lua_sdb_fargerror( L, 4, "Unknown column %s.", colname);
            }

            r = sdb_setcolumn( dst, colname, s_method, arg);
            if( r) { sdb_close( dst); return push_sdb_error( L, r); }

            r = sdb_setconscolumn( src, src_col, c_method);
            if( r) { sdb_close( dst); return push_sdb_error( L, r); }

            lua_pop( L, 1); // src, id, storage, columns, dst
        }
        // src, id, storage, columns, dst, nil
        lua_pop( L, 1); // src, id, storage, columns, dst
    } else {
        // short colname/consolidation method mapping
        for( lua_pushnil( L); lua_next( L, 4);) { // src, id, storage, columns, dst, key, val
            if( !lua_isstring(L, -2))
                lua_sdb_fargerror( L, 4, "Expected string for columns names, got %s", lua_typename( L, lua_type( L, -1)));
            const char *colname = luaL_checkstring( L, -2);
            enum sdb_serialization_method_t s_method = SDB_DEFAULT_SERIALIZATION_METHOD;
            enum sdb_consolidation_method_t c_method = lua_sdb_getoption( L, -1, consolidation_methods);
            if( !consolidation_methods[c_method]) {
                lua_sdb_fargerror( L, 4, "Invalid consolidation method for %s", colname);
            }

            sdb_ncolumn_t src_col = sdb_getcolnum( src, colname);
            if( SDB_NCOLUMN_INVALID == src_col) {
                lua_sdb_fargerror( L, 4, "Unknown column %s", colname);
            }

            r = sdb_setcolumn( dst, colname, s_method, 0.0);
            if( r) { sdb_close( dst); return push_sdb_error( L, r); }

            r = sdb_setconscolumn( src, src_col, c_method);
            if( r) { sdb_close( dst); return push_sdb_error( L, r); }

            lua_pop( L, 1); // src, id, storage, columns, dst
        }
    }
    // src, id, storage, columns, dst
    return 1;
}

/* Add a row to a table. The row must be passed as first non-self parameter,
 * with the column names as values. */
static int api_row( lua_State *L) {
    struct sdb_table_t *tbl;
    sdb_ncolumn_t icol;
    int r;

    tbl = lua_sdb_checktable( L, 1);

    if( tbl->state != SDB_ST_READING) {
        return push_sdb_error( L, SDB_EBADSTATE);
    }

    // tbl, map
    for( icol=0; icol < tbl->ncolumns; icol++) { // tbl, map
        const char *colname = sdb_getcolname( tbl, icol);
        lua_getfield( L, -1, colname); // tbl, map, map[getcolname(icol)]
        r = lua_sdb_serialize( L, -1, tbl);
        if( r) return push_sdb_error( L, r);  // TODO: provide colname
        lua_pop( L, 1); // tbl, map
    }
    lua_pop( L, 1); // tbl
    return 1; // push back table
}

static int api_state( lua_State *L) {
    sdb_ncolumn_t i;
    sdb_table_t *tbl = lua_sdb_checktable( L, 1);
    lua_newtable( L);
    lua_pushinteger( L, tbl->nwrittenobjects); lua_setfield( L, -2, "nwrittenobjects");
    lua_pushinteger( L, tbl->nwrittenbytes); lua_setfield( L, -2, "nwrittenbytes");
    lua_pushstring( L, state_names[tbl->state]); lua_setfield( L, -2, "state");
    lua_pushstring( L, storage_options[tbl->storage_kind]); lua_setfield( L, -2, "storage");
    lua_pushstring( L, tbl->conf_strings); lua_setfield( L, -2, "id");
    lua_pushinteger( L, tbl->nwrittenobjects/tbl->ncolumns); lua_setfield( L, -2, "nrows");
    lua_newtable( L);
    for( i=0; i<tbl->ncolumns; i++) {
        lua_pushinteger( L, i+1);
        lua_pushstring( L, sdb_getcolname( tbl, i));
        lua_settable( L, -3);
    }
    lua_setfield( L, -2, "columns");
    if( tbl->maxwrittenobjects) {
        lua_pushinteger( L, tbl->maxwrittenobjects/tbl->ncolumns);
        lua_setfield( L, -2, "maxrows");
    }
    return 1;
}

// mysdb :consolidate()
static int api_consolidate( lua_State *L) {
    int r = sdb_consolidate( lua_sdb_checktable( L, 1));
    if( r) return push_sdb_error( L, r);
    return 1; // push table back
}

// mysdb :trim()
static int api_trim( lua_State *L) {
    int r = sdb_trim( lua_sdb_checktable( L, 1));
    if( r) return push_sdb_error( L, r);
    return 1; // push table back
}

// mysdb :serialize(bss)
static int api_serialize( lua_State *L) {
    struct sdb_table_t *tbl = lua_sdb_checktable( L, 1);
    struct bss_ctx_t   *bss = lua_bss_checkctx( L, 2);
    int r = sdb_serialize( tbl, bss);
    if( BSS_EAGAIN == r) {
        lua_pushnil( L); lua_pushstring( L, "again"); return 2;
    } else if( BSS_EOK == r) {
        return 1; // push table back
    } else {
        return push_sdb_error( L, r);
    }
}

// mysdb :serialize_cancel()
static int api_serialize_cancel( lua_State *L) {
    int r = sdb_serialize_cancel( lua_sdb_checktable( L, 1));
    lua_settop( L, 1); // in case of extra args
    lua_pushinteger( L, r); // no lua_error, just a status
    return 2; // push table back
}

// mysdb :reset()
static int api_reset( lua_State *L) {
    int r = sdb_reset( lua_sdb_checktable( L, 1));
    if( r) return push_sdb_error( L, r);
    return 1; // push table back
}

// mysdb :close()
static int api_close( lua_State *L) {
    sdb_close( lua_sdb_checktable( L, 1));
    lua_pushboolean( L, 1);
    return 1;
}

int luaopen_stagedb_core( lua_State *L) {


    /* Register C functions. */
    luaL_findtable( L, LUA_GLOBALSINDEX, "stagedb.core", 8); // stagedb.core
#define REG(x) lua_pushcfunction( L, api_##x); lua_setfield( L, -2, #x)
    REG( init);
    REG( newconsolidation);
    REG( row);
    REG( consolidate);
    REG( serialize);
    REG( serialize_cancel);
    REG( reset);
    REG( close);
    REG( state);
    REG( trim);
#undef REG

    lua_newtable(   L); // mt
    lua_pushstring( L, MT_NAME); // stagedb.core, mt, name
    lua_setfield(   L, -2, "__type"); // stagedb.core, mt
    lua_pushvalue(  L, -2); // stagedb.core, mt, stagedb.core
    lua_setfield(   L, -2, "__index"); // stagedb.core, mt
    /* Register cleanup upon garbage collection. */
    lua_pushcfunction( L, api_close); // stagedb.core, mt, api_close
    lua_setfield( L, -2, "__gc"); // stagedb.core, mt
    lua_setfield(   L, LUA_REGISTRYINDEX, MT_NAME); // stagedb.core
    return 1;
}
