/* public domain */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "byte.h"
#include "cdb.h"
#include "lua.h"
#include "lauxlib.h"

#define MYNAME "cdb"

typedef struct Cdb Cdb;

struct Cdb
{
	struct cdb cdb;
	char *name;
};

static void
xfree(void *ptr)
{
	if(ptr)
		free(ptr);
}

static Cdb *
lcdb_get(lua_State *L, int index)
{
	Cdb *cdb;

	cdb = lua_touserdata(L, index);
	if(cdb && lua_getmetatable(L, index) && lua_rawequal(L, -1, LUA_ENVIRONINDEX)){
		lua_pop(L, 1);
		return cdb;
	}
	luaL_typerror(L, index, MYNAME);
	return 0;
}

static int
lcdb_init(lua_State *L)
{
	Cdb *cdb;
	int fd=-1;
	size_t len_fn=0;
	const char *fn=0;

	if(lua_type(L, 1) == LUA_TNUMBER)
		fd = (int)lua_tonumber(L, 1);
	else
		fn = luaL_checklstring(L, 1, &len_fn);

	if(fn && (fd = open(fn, O_RDONLY, 0)) < 0){
		int xerrno = errno;
		lua_pushnil(L);
		lua_pushnumber(L, xerrno);
		lua_pushstring(L, strerror(xerrno));
		return 3;
	}

	cdb = lua_newuserdata(L, sizeof(*cdb)+len_fn+1);
	memset(cdb, 0, sizeof(*cdb));
	cdb_init(&cdb->cdb, fd);
	if(fn){
		cdb->name = (char *)(cdb+1);
		byte_copy(cdb->name, len_fn+1, fn);
	}
	lua_pushvalue(L, LUA_ENVIRONINDEX);
	lua_setmetatable(L, -2);
	return 1;
}

static int
lcdb_free(lua_State *L)
{
	Cdb *cdb;
	int fd;

	cdb = lcdb_get(L, 1);
	fd = cdb->cdb.fd;
	cdb_free(&cdb->cdb);
	if(cdb->name)
		close(fd);		/* XXX ignores error */
	cdb->cdb.fd = -1;
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	return 0;
}

static int
lcdb_findstart(lua_State *L)
{
	struct cdb *cdb;

	cdb = &lcdb_get(L, 1)->cdb;
	cdb_findstart(cdb);
	return 0;
}

static int
lcdb_findnext(lua_State *L)
{
	struct cdb *cdb;
	int ret;
	size_t klen;
	const char *k;

	cdb = &lcdb_get(L, 1)->cdb;
	k = luaL_checklstring(L, 2, &klen);
	ret = cdb_findnext(cdb, (char *)k, klen);
	if(ret < 0){
		int xerrno = errno;
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnumber(L, xerrno);
		lua_pushstring(L, strerror(xerrno));
		return 4;
	}else if(ret == 0)
		return 0;
	else{
		lua_pushnumber(L, cdb_datalen(cdb));
		lua_pushnumber(L, cdb_datapos(cdb));
		return 2;
	}
}

static int
lcdb_read(lua_State *L)
{
	struct cdb *cdb;
	int dpos, dlen, nret;
	char *d;

	cdb = &lcdb_get(L, 1)->cdb;
	dpos = luaL_checkint(L, 2);
	dlen = luaL_checkint(L, 3);
	if(!(d = malloc(dlen)) || (cdb_read(cdb, d, dlen, dpos) < 0)){
		int xerrno = errno;
		lua_pushnil(L);
		lua_pushnumber(L, xerrno);
		lua_pushstring(L, strerror(xerrno));
		nret = 3;
	}else{
		lua_pushlstring(L, d, dlen);		/* XXX double copy */
		nret = 1;
	}
	xfree(d);
	return nret;
}

static int
lcdb_pairs_aux(lua_State *L)
{
	struct cdb *cdb;
	uint32 pos;
	char buf[8], *kd=0;
	uint32 eod, klen, dlen;
	int xerrno;

	cdb = &lcdb_get(L, lua_upvalueindex(1))->cdb;
	pos = lua_tonumber(L, lua_upvalueindex(2));
	if(cdb_read(cdb, buf, 4, 0) < 0)
		goto error;
	uint32_unpack(buf, &eod);
	if((pos < 2048) || (pos >= eod))
		return 0;
	if(cdb_read(cdb, buf, 8, pos) < 0)
		goto error;
	pos += 8;
	uint32_unpack(buf, &klen);
	uint32_unpack(buf+4, &dlen);
	if(!(kd = malloc(klen+dlen)) || (cdb_read(cdb, kd, klen+dlen, pos) < 0))
		goto error;
	lua_pushlstring(L, kd, klen);		/* XXX double copy */
	lua_pushlstring(L, kd+klen, dlen);	/* XXX double copy */
	xfree(kd);
	pos += klen+dlen;
	lua_pushnumber(L, pos);
	lua_replace(L, lua_upvalueindex(2));
	return 2;

error:	xerrno = errno;
	xfree(kd);
	return luaL_error(L, "lcdb_pairs_aux: %s (errno=%d)", strerror(xerrno), xerrno);
}

static int
lcdb_pairs(lua_State *L)
{
	(void)lcdb_get(L, 1);
	lua_pushnumber(L, 2048);
	lua_pushcclosure(L, lcdb_pairs_aux, 2);
	return 1;
}

static int
lcdb_values_aux(lua_State *L)
{
	struct cdb *cdb;
	const char *k;
	size_t klen;
	char *d=0;
	int ret;

	cdb = &lcdb_get(L, lua_upvalueindex(1))->cdb;
	k = lua_tolstring(L, lua_upvalueindex(2), &klen);
	if((ret = cdb_findnext(cdb, (char *)k, klen)) == 0)
		return 0;
	else if(ret < 0)
		goto error;
	else{
		uint32 dpos, dlen;
		dpos = cdb_datapos(cdb);
		dlen = cdb_datalen(cdb);
		d = malloc(dlen);
		if(!d || (cdb_read(cdb, d, dlen, dpos) < 0))
			goto error;
		lua_pushlstring(L, d, dlen);		/* XXX double copy */
		xfree(d);
		return 1;
	}

error:	xfree(d);
	return luaL_error(L, "lcdb_values_aux: %s (errno=%d)", strerror(errno), errno);
}

static int
lcdb_values(lua_State *L)
{
	struct cdb *cdb;

	cdb = &lcdb_get(L, 1)->cdb;
	cdb_findstart(cdb);
	(void)luaL_checkstring(L, 2);
	lua_pushcclosure(L, lcdb_values_aux, 2);
	return 1;
}

static int
lcdb_fd(lua_State *L)
{
	struct cdb *cdb;

	cdb = &lcdb_get(L, 1)->cdb;
	lua_pushnumber(L, cdb->fd);
	return 1;
}

static int
lcdb_name(lua_State *L)
{
	Cdb *cdb;

	cdb = lcdb_get(L, 1);
	lua_pushstring(L, cdb->name);
	return 1;
}

static int
lcdb_tostring(lua_State *L)
{
	Cdb *cdb;
	char s[64];

	cdb = lcdb_get(L, 1);
	lua_pushlstring(L, s, snprintf(s, sizeof(s), MYNAME " %p", cdb));
	return 1;
}

static struct luaL_reg lcdb_methods[] =
{
	{ "__gc",	lcdb_free },
	{ "__tostring",	lcdb_tostring },
	{ "free",	lcdb_free },
	{ "findstart",	lcdb_findstart },
	{ "findnext",	lcdb_findnext },
	{ "read",	lcdb_read },
	{ "pairs",	lcdb_pairs },
	{ "values",	lcdb_values },
	{ "fd",		lcdb_fd },
	{ "name",	lcdb_name },
	{ 0, 0 }
};

static struct luaL_reg lcdb_funcs[] =
{
	{ "init",	lcdb_init },
	{ 0, 0 }
};

int
luaopen_cdb(lua_State *L)
{
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_replace(L, LUA_ENVIRONINDEX);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_register(L, 0, lcdb_methods);
	luaL_register(L, MYNAME, lcdb_funcs);
	return 1;
}
