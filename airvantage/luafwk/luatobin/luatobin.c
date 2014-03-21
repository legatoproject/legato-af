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

/*
 * Protocol description
 * |type(1b)|[size(2b)]|[data(sizeb)]| (big endian)
 * 'number' type = LUA_TNUMBER, size = sizeof(lua_Number) (not serialized)
 * 'integer' type = LUA_TINT, size = sizeof(lua_Integer) (not serialized)
 * 'string' type = LUA_TSTRING, size = sizeof('string')
 * 'bool' type = LUA_TBOOLEAN, size = 1 (not serialized)
 * 'function' type = LUA_TFUNCTION, size = sizeof(function)
 * 'table' type = LUA_TTABLE, size = #table
 * 'nil' type = LUA_TNIL, size = 0 (not serialized)
 */
#include "rc_endian.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <stdint.h>

typedef enum {
    BIN_NONE = 0xFF,
    BIN_NIL = 0x00,
    BIN_BOOLEAN = 0x01,
    BIN_DOUBLE = 0x03,
    BIN_STRING = 0x04,
    BIN_TABLE = 0x05,
    BIN_FUNCTION = 0x06,
    BIN_INTEGER = 0x07,
    BIN_REF = 0x14
} LuatobinTypes;

typedef struct Dfunction_ {
    int counter;
    int size;
    const char* buffer;
} Dfunction;

static SEndian sendian;

//static void pprint(unsigned char *p, size_t n) {
//  int i;
//  printf("\r\n");
//  for (i = 0; i < n; i++) {
//      printf("[%02X]", p[i]);
//  }
//  printf("\r\n");
//}

static int concat_table(lua_State *L, int pos, luaL_Buffer* b) {
    int i, last;
    last = lua_objlen(L, pos);
    luaL_buffinit(L, b);
    for (i = 1; i <= last; i++) {
        lua_rawgeti(L, pos, i);
        luaL_addvalue(b);
    }
    luaL_pushresult(b);
    return 1;
}

static int f_writer(lua_State *L, const void* b, size_t size, void* B) {
    luaL_addlstring((luaL_Buffer*) B, (const char *) b, size);
    return 0;
}

static void serialize_function(lua_State *L, luaL_Buffer* b, int pos) {
    lua_pushvalue(L, pos);
    luaL_buffinit(L, b);
    if (lua_dump(L, f_writer, b) != 0)
        luaL_error(L, "cannot serialize: unable to dump function");
    luaL_pushresult(b);
    lua_remove(L, -2); // consume the function, leave the serialized string on the stack
}

static const char* f_reader(lua_State* L, void* data, size_t* size) {
    if (((Dfunction*) data)->counter == 0) {
        ((Dfunction*) data)->counter = 1;
        *size = (size_t)((Dfunction*) data)->size;
        return ((Dfunction*) data)->buffer;
    } else {
        return NULL;
    }
}

static uint16_t verify_cache(lua_State* L, int object, uint16_t* key) {
    // get objet in luatobin_cache
    lua_pushvalue(L, object); //LookUpObject
    lua_rawget(L, 2); // RefToObject or nil
    int number = 0;
    if (lua_isnil(L, -1)) {
        *key = *key + 1;
        // t[object] = key
        lua_pushvalue(L, object); // nil / LookUpObject
        lua_pushinteger(L, *key); // nil / LookUpObject / RefToObject
        lua_rawset(L, 2); // nil
    } else {
        number = (uint16_t) lua_tointeger(L, -1); // RefToObject
    }
    lua_pop(L, 1);
    return number;
}

static void add_to_cache(lua_State* L, uint16_t *key) {
    // t[key] = object
    *key = *key + 1;
    lua_pushvalue(L, -1);
    lua_rawseti(L, 2, *key);
    //printf("c[%d]", *key);
}

static void get_from_cache(lua_State* L, uint16_t key) {
    // get objet in luatobin_cache
    lua_rawgeti(L, 2, key);
}

static uint8_t get_luatobin_type(lua_State* L, int pos) {
    switch (lua_type(L, pos)) {
    case LUA_TNIL:
        return BIN_NIL;
    case LUA_TBOOLEAN:
        return BIN_BOOLEAN;
    case LUA_TNUMBER:
        if (lua_tonumber(L, pos) == lua_tointeger(L, pos))
            return BIN_INTEGER;
        return BIN_DOUBLE;
    case LUA_TSTRING:
        return BIN_STRING;
    case LUA_TTABLE:
        return BIN_TABLE;
    case LUA_TFUNCTION:
        return BIN_FUNCTION;
    default:
        return BIN_NONE;
    }
}

static int get_local_index(lua_State *L, int gindex, int indice, int last) {
    int objlen = lua_objlen(L, -1); // 5
    int lindex = 0;
    // printf("\r\n\tindex[l=%d,g=%d,indice=%d]", lindex, gindex, indice);
    while ((lindex + objlen) < gindex) {
        if (indice >= last) {
            luaL_error(L, "cannot deserialize: end of table");
        }
        lua_pop(L, 1); //4
        lindex = lindex + objlen;
        indice++;
        lua_rawgeti(L, 1, indice); //5
        objlen = lua_objlen(L, -1);
        // printf("\r\n\tindex+[l=%d,g=%d,indice=%d]", lindex, gindex, indice);
    }
    lua_pushinteger(L, indice); //6
    lua_replace(L, 4); //5
    return gindex - lindex;
}

static void read_object(lua_State *L, luaL_Buffer* b, char** frame, int* index, int call, size_t* len) {
    // printf("\r\n\tread[%d/call=%d/index=%d/len=%d/%p]", (*index + call - ((int) (*len))), call,*index, *len, *frame);
    if ((*index + call - ((int) (*len))) > 0) {
        if (lua_istable(L, 1)) {
            int last = lua_tointeger(L, 3); // TableMax 3
            int indice = lua_tointeger(L, 4); // TableCurrentIndice 4
            size_t current = 0;
            luaL_buffinit(L, b);
            if (*index < *len){
                lua_pushvalue(L, 5);
                luaL_addvalue(b);
                current = *len-*index;
                // printf("\r\n\taddcur[indice=%d,index=%d,current=%d]", indice, *index, current);
            } else {
                *index = 0;
            }
            while (current < call) {
                if (indice >= last) {
                    luaL_error(L, "cannot deserialize: end of table");
                }
                indice++;
                lua_rawgeti(L, 1, indice);
                current = current + lua_objlen(L, -1);
                luaL_addvalue(b);
                // printf("\r\n\tadd[indice=%d,index=%d,current=%d]", indice, *index, current);
            }
            luaL_pushresult(b); // StringBuffer 5
            lua_replace(L, 5);
            lua_pushinteger(L, indice); // TableCurrentIndice 4
            lua_replace(L, 4);
            *frame = (char*) luaL_checklstring(L, 5, len);
            // printf("\r\n\t->[%p]", *frame);
        } else {
            luaL_error(L, "cannot deserialize: end of string");
        }
    }
}

static int write_object(lua_State *L, int* indice) {
    *indice = *indice + 1;
    lua_rawseti(L, 3, *indice);
    return *indice;
}

static void serialize_value(lua_State *L, int pos, luaL_Buffer* frame, int* indice, uint16_t* key) {
    if (!lua_checkstack(L, 2)) {
        luaL_error(L, "cannot serialize: stack won't grow");
    }
    uint8_t type = get_luatobin_type(L, pos);
    switch (type) {
    case BIN_NIL: {
        //printf("(nil[1])");
        lua_pushlstring(L, (char *) &type, 1);
        write_object(L, indice);
        break;
    }

    case BIN_BOOLEAN: {
        //printf("(boolean[2])");
        int boolean = lua_toboolean(L, pos);
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        luaL_addchar(frame, boolean & 0xFF);
        luaL_pushresult(frame);
        write_object(L, indice);
        break;
    }

    case BIN_DOUBLE: {
        double number = lua_tonumber(L, pos);
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        //printf("(number[%d])", sizeof(number));
        if (sendian.double_) hton(&number, sizeof(number), sendian.double_);
        luaL_addlstring(frame, (char*) &number, sizeof(number));
        luaL_pushresult(frame);
        write_object(L, indice);
        break;
    }

    case BIN_INTEGER: {
        int32_t number = lua_tointeger(L, pos);
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        //printf("(number[%d])", sizeof(number));
        if (sendian.int32_) hton(&number, sizeof(number), sendian.int32_);
        luaL_addlstring(frame, (char*) &number, sizeof(number));
        luaL_pushresult(frame);
        write_object(L, indice);
        break;
    }

    case BIN_STRING: {
        uint16_t cached = verify_cache(L, pos, key);
        if (cached != 0) {
            //printf("(stringref[%d])", cached);
            luaL_buffinit(L, frame);
            luaL_addchar(frame, BIN_REF);
            if (sendian.int16_) hton(&cached, sizeof(cached), sendian.int16_);
            luaL_addlstring(frame, (char*) &cached, sizeof(cached));
            luaL_pushresult(frame);
            write_object(L, indice);
            break;
        }
        size_t s;
        const char* string = lua_tolstring(L, pos, &s);
        if (s >= 0xFFFF)
            luaL_error(L, "cannot serialize: string length > 65k");
        uint16_t size = (uint16_t) s;
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        uint16_t nsize = size;
        if (sendian.int16_) hton(&nsize, sizeof(nsize), sendian.int16_);
        luaL_addlstring(frame, (char*) &nsize, sizeof(nsize));
        //printf("(string[%d][%d])", *key, size);
        luaL_addlstring(frame, string, size);
        luaL_pushresult(frame);
        write_object(L, indice);
        break;
    }

    case BIN_FUNCTION: {
        uint16_t cached = verify_cache(L, pos, key);
        if (cached != 0) {
            //printf("(functionref[%d])", cached);
            luaL_buffinit(L, frame);
            luaL_addchar(frame, BIN_REF);
            if (sendian.int16_) hton(&cached, sizeof(cached), sendian.int16_);
            luaL_addlstring(frame, (char*) &cached, sizeof(cached));
            luaL_pushresult(frame);
            write_object(L, indice);
            break;
        }
        serialize_function(L, frame, pos);
        size_t s;
        const char* string = lua_tolstring(L, -1, &s);
        if (s >= 0xFFFF)
            luaL_error(L, "cannot serialize: function length > 65k");
        uint16_t size = (uint16_t) s;
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        uint16_t nsize = size;
        if (sendian.int16_) hton(&nsize, sizeof(nsize), sendian.int16_);
        luaL_addlstring(frame, (char*) &nsize, sizeof(nsize));
        //printf("(function[%d][%d])", *key, size);
        luaL_addlstring(frame, string, size);
        luaL_pushresult(frame);
        write_object(L, indice);
        lua_pop(L, 1); // remove the serialized function
        break;
    }

    case BIN_TABLE: {
        uint16_t cached = verify_cache(L, pos, key);
        if (cached != 0) {
            //printf("(tableref[%d])", cached);
            luaL_buffinit(L, frame);
            luaL_addchar(frame, BIN_REF);
            if (sendian.int16_) hton(&cached, sizeof(cached), sendian.int16_);
            luaL_addlstring(frame, (char*) &cached, sizeof(cached));
            luaL_pushresult(frame);
            write_object(L, indice);
            break;
        }
        // reserved for table header (as size cannot be calculated beforehand)
        *indice = *indice + 1;
        int header = *indice;
        //printf("{table=");
        int top;
        size_t s = 0;
        lua_pushnil(L);
        while (lua_next(L, pos) != 0) {
            top = lua_gettop(L);
            serialize_value(L, top - 1, frame, indice, key);
            serialize_value(L, top, frame, indice, key);
            lua_pop(L, 1);
            s++;
        }
        if (s >= 0xFFFF)
            luaL_error(L, "cannot serialize: table length > 65k");
        uint16_t size = (uint16_t) s;
        luaL_buffinit(L, frame);
        luaL_addchar(frame, type);
        uint16_t nsize = size;
        if (sendian.int16_) hton(&nsize, sizeof(nsize), sendian.int16_);
        luaL_addlstring(frame, (char*) &nsize, sizeof(nsize));
        luaL_pushresult(frame);
        // set table header
        lua_rawseti(L, 3, header);
        //printf("[%d]}", size);
        break;
    }

    default:
        luaL_error(L, "cannot serialize: unsupported type (%d)", lua_type(L, pos));
    }
}

static void deserialize_value(lua_State *L, luaL_Buffer* buffer, char** frame, int* index, int* gindex, size_t* len, uint16_t* key) {
    if (!lua_checkstack(L, 2)) {
        luaL_error(L, "cannot deserialize: stack won't grow");
    }
    uint16_t size = 0x0000;
    read_object(L, buffer, frame, index, 1, len);
    uint8_t type = (*frame)[*index];
    *index = *index + 1;
    *gindex = *gindex + 1;
    switch (type) {
    case BIN_NIL: {
        // printf("\r\n\t(nil[1])[%p]", *frame);
        lua_pushnil(L);
        break;
    }

    case BIN_BOOLEAN: {
        read_object(L, buffer, frame, index, 1, len);
        // printf("\r\n\t(boolean[2][%p]", *frame);
        lua_pushboolean(L, (*frame)[*index]);
        *index = *index + 1;
        *gindex = *gindex + 1;
        break;
    }

    case BIN_DOUBLE: {
        double number = 0;
        size = sizeof(number);
        read_object(L, buffer, frame, index, size, len);
        // printf("\r\n\t(number[%d])[%p]", size, *frame);
        char* b = (char*) &number;
        int i;
        for (i = 0; i < size; i++) {
            b[i] = (*frame)[*index + i];
        }
        if (sendian.double_) ntoh(&number, sizeof(number), sendian.double_);
        lua_pushnumber(L, number);
        *index = *index + size;
        *gindex = *gindex + size;
        break;
    }

    case BIN_INTEGER: {
        int32_t number = 0;
        size = sizeof(number);
        read_object(L, buffer, frame, index, size, len);
        // printf("\r\n\t(number[%d])[%p]", size, *frame);
        char* b = (char*) &number;
        int i;
        for (i = 0; i < size; i++) {
            b[i] = (*frame)[*index + i];
        }
        if (sendian.int32_) ntoh(&number, sizeof(number), sendian.int32_);
        lua_pushinteger(L, number);
        *index = *index + size;
        *gindex = *gindex + size;
        break;
    }

    case BIN_REF: {
        read_object(L, buffer, frame, index, 2, len);
        ((char*) &size)[0] =(*frame)[*index];
        ((char*) &size)[1] = (*frame)[*index + 1];
        if (sendian.int16_) ntoh(&size, sizeof(size), sendian.int16_);
        *index = *index + 2;
        *gindex = *gindex + 2;
        // printf("\r\n\t(ref[%d])[%p]", size, *frame);
        get_from_cache(L, size);
        break;
    }

    case BIN_STRING: {
        read_object(L, buffer, frame, index, 2, len);
        ((char*) &size)[0] = (*frame)[*index];
        ((char*) &size)[1] = (*frame)[*index + 1];
        if (sendian.int16_) ntoh(&size, sizeof(size), sendian.int16_);
        *index = *index + 2;
        *gindex = *gindex + 2;
        read_object(L, buffer, frame, index, size, len);
        // printf("\r\n\t(string[%d])[%p]", size, *frame);
        lua_pushlstring(L, &((*frame)[*index]), size);
        *index = *index + size;
        *gindex = *gindex + size;
        add_to_cache(L, key);
        break;
    }

    case BIN_FUNCTION: {
        read_object(L, buffer, frame, index, 2, len);
        ((char*) &size)[0] = (*frame)[*index];
        ((char*) &size)[1] = (*frame)[*index + 1];
        if (sendian.int16_) ntoh(&size, sizeof(size), sendian.int16_);
        *index = *index + 2;
        *gindex = *gindex + 2;
        // printf("\r\n\t(function[%d])[%p]", size, *frame);
        read_object(L, buffer, frame, index, size, len);
        Dfunction dfunction;
        dfunction.counter = 0;
        dfunction.size = size;
        dfunction.buffer = &((*frame)[*index]);
        lua_load(L, f_reader, &dfunction, "function");
        *index = *index + size;
        *gindex = *gindex + size;
        add_to_cache(L, key);
        break;
    }

    case BIN_TABLE: {
        read_object(L, buffer, frame, index, 2, len);
        ((char*) &size)[0] = (*frame)[*index];
        ((char*) &size)[1] = (*frame)[*index + 1];
        if (sendian.int16_) ntoh(&size, sizeof(size), sendian.int16_);
        *index = *index + 2;
        *gindex = *gindex + 2;
        lua_newtable(L);
        // printf("\r\n\t{table[%d][%p]=", size, *frame);
        add_to_cache(L, key);
        int top = lua_gettop(L);
        while (size > 0) {
            // printf("\r\n\t|key[%d][%p]", *index, *frame);
            deserialize_value(L, buffer, frame, index, gindex, len, key);
            // printf("\r\n\t+value[%d][%p]", *index, *frame);
            deserialize_value(L, buffer, frame, index, gindex, len, key);
            lua_rawset(L, top);
            size--;
        }
        // printf("\r\n\t)");
        // printf("\r\n\t}");
        break;
    }

    default:
        luaL_error(L, "cannot deserialize: unsupported type [%d] at %d", type, *gindex);
    }
}

/*
 * serialize(obj, totable)
 *   - obj: object to serialize
 *   - totable: if true, returns a string buffer containing the serialized object instead of a buffer
 *  returns a string buffer (table) or a string containing obj serialization
 */
static int serialize(lua_State *L) {
    //printf("\r\nserialize");
    int buffer = 0;
    if (lua_isboolean(L, 2)) {
        buffer = lua_toboolean(L, 2);
    }
    lua_settop(L, 1); // Object 1
    lua_newtable(L); // CacheTable 2
    lua_newtable(L); // OutTable 3
    luaL_Buffer frame;
    uint16_t key = 0;
    int indice = 0;
    serialize_value(L, 1, &frame, &indice, &key);
    if (buffer) {
        lua_settop(L, 3); // return OutTable
    } else {
        concat_table(L, 3, &frame); // return table.concat(OutTable) (SerializedObject)
    }
    //printf("\r\nendserialize");
    return 1;
}

/*
 * deserialize(buffer, nobj, offset)
 *   - buffer: string or string buffer (table)
 *   - nobj: number of objects to deserialize, optional default value 0 means all objects
 *   - offset: starting offset in the buffer, optional default value 1
 *  returns: index of the next object + nobj objects
 */
static int deserialize(lua_State *L) {
    // printf("\r\ndeserialize");
    if (lua_isnil(L, 1)) {
        luaL_error(L, "cannot deserialize: nothing to process");
    }
    int nobj = luaL_optint(L, 2, 0);
    int gindex = luaL_optint(L, 3, 1)-1;
    int lindex = 0;
    uint16_t key = 0;
    luaL_Buffer buffer;
    int result = 0;
    size_t len = 0;
    lua_settop(L, 1); // SerializedObject 1
    lua_newtable(L); // CacheTable 2
    // get frame
    char* frame = NULL;
    if (lua_istable(L, 1)) {
        int last = lua_objlen(L, 1);
        lua_pushinteger(L, last); // TableMax 3
        lua_pushinteger(L, 1); // TableCurrentIndice 4
        lua_rawgeti(L, 1, 1); // StringBuffer 5
        lindex = get_local_index(L, gindex, 1, last);
        frame = (char*)luaL_checklstring(L, 5, &len);
    } else {
        lindex = gindex;
        frame = (char*)luaL_checklstring(L, 1, &len);
    }
    // printf("\r\n\tlindex=%d, gindex=%d", lindex, gindex);
    // deserialize frame
    lua_pushnil(L); // index of next obj
    while ((lindex < len) && ((nobj <= 0) || (result < nobj))) {
        deserialize_value(L, &buffer, &frame, &lindex, &gindex, &len, &key);
        result++;
        lua_newtable(L); // New CacheTable 2
        lua_replace(L, 2);
        key = 0;
    }
    lua_pushinteger(L, gindex+1); // index of next obj
    lua_replace(L, -result-2);
    // printf("\r\nenddeserialize[%d]", result);
    return result + 1;
}

/**
 * Register functions.
 */
static const luaL_Reg R[] = {
        { "serialize", serialize },
        { "deserialize", deserialize },
        { NULL, NULL } };

int luaopen_luatobin(lua_State* L) {
    check_endian(&sendian);
    luaL_register(L, "luatobin", R);
    return 1;
}
