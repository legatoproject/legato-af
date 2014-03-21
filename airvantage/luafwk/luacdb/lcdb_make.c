/* public domain */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "open.h"
#include "alloc.h"
#include "byte.h"
#include "cdb_make.h"
#include "lua.h"
#include "lauxlib.h"

#define MYNAME "cdb_make"

typedef struct Cdbmake Cdbmake;

struct Cdbmake
{
	struct cdb_make cdbm;
	char *fn, *fntmp;
};

static Cdbmake *
lcdb_make_get(lua_State *L, int index)
{
	Cdbmake *cdbm;

	cdbm = lua_touserdata(L, index);
	if(cdbm && lua_getmetatable(L, index) && lua_rawequal(L, -1, LUA_ENVIRONINDEX)){
		lua_pop(L, 1);
		return cdbm;
	}
	luaL_typerror(L, index, MYNAME);
	return 0;
}

static int
lcdb_make_start(lua_State *L)
{
	Cdbmake *cdbm;
	size_t len_fn, len_fntmp;
	const char *fn, *fntmp;
	int fd, xerrno;

	fn = luaL_checklstring(L, 1, &len_fn);
	fntmp = luaL_checklstring(L, 2, &len_fntmp);

	if((fd = open_trunc((char *)fntmp)) < 0)
		goto error;

	cdbm = lua_newuserdata(L, sizeof(*cdbm)+len_fn+len_fntmp+2);
	memset(&cdbm->cdbm, 0, sizeof(cdbm->cdbm));
	cdbm->fn = (char *)(cdbm+1);
	cdbm->fntmp = (char *)(cdbm+1)+len_fn+1;
	byte_copy(cdbm->fn, len_fn+1, fn);
	byte_copy(cdbm->fntmp, len_fntmp+1, fntmp);

	if(cdb_make_start(&cdbm->cdbm, fd) < 0)
		goto error;

	lua_pushvalue(L, LUA_ENVIRONINDEX);
	lua_setmetatable(L, -2);
	return 1;

error:	xerrno = errno;
	lua_pushnil(L);
	lua_pushnumber(L, xerrno);
	lua_pushstring(L, strerror(xerrno));
	if(fd >= 0)
		close(fd);
	return 3;
}

static void
lcdb_make_free(struct cdb_make *cdbm)
{
	struct cdb_hplist *head, *next;

	if(cdbm->split)
		alloc_free(cdbm->split);
	head = cdbm->head;
	while(head){
		next = head->next;
		alloc_free(head);
		head = next;
	}
	if(cdbm->fd >= 0)
		close(cdbm->fd);		/* XXX ignores error */
	cdbm->fd = -1;
}

static int
lcdb_make_gc(lua_State *L)
{
	struct cdb_make *cdbm;

	cdbm = &lcdb_make_get(L, 1)->cdbm;
	if(cdbm->fd >= 0)
		lcdb_make_free(cdbm);
	return 0;
}

static int
lcdb_make_add(lua_State *L)
{
	struct cdb_make *cdbm;
	size_t klen, dlen;
	const char *k, *d;

	cdbm = &lcdb_make_get(L, 1)->cdbm;
	k = luaL_checklstring(L, 2, &klen);
	d = luaL_checklstring(L, 3, &dlen);
	if(cdb_make_add(cdbm, (char *)k, klen, (char *)d, dlen) < 0){
		int xerrno = errno;
		lua_pushnumber(L, xerrno);
		lua_pushstring(L, strerror(xerrno));
		lcdb_make_free(cdbm);
		return 2;
	}
	return 0;
}

static int
lcdb_make_finish(lua_State *L)
{
	Cdbmake *cdbm;
	int xerrno;

	cdbm = lcdb_make_get(L, 1);
	if(cdb_make_finish(&cdbm->cdbm) < 0)
		goto error;
	if(fsync(cdbm->cdbm.fd) < 0)
		goto error;
	if(close(cdbm->cdbm.fd) < 0)
		goto error;
	cdbm->cdbm.fd = -1;
	if(rename(cdbm->fntmp, cdbm->fn) < 0)
		goto error;
	lcdb_make_free(&cdbm->cdbm);
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	return 0;

error:	xerrno = errno;
	lua_pushnumber(L, xerrno);
	lua_pushstring(L, strerror(xerrno));
	lcdb_make_free(&cdbm->cdbm);
	return 2;
}

static int
lcdb_make_tostring(lua_State *L)
{
	Cdbmake *cdbm;
	char s[64];

	cdbm = lcdb_make_get(L, 1);
	lua_pushlstring(L, s, snprintf(s, sizeof(s), MYNAME " %p", cdbm));
	return 1;
}

static struct luaL_reg lcdb_make_methods[] =
{
	{ "__gc",	lcdb_make_gc },
	{ "__tostring",	lcdb_make_tostring },
	{ "add",	lcdb_make_add },
	{ "finish",	lcdb_make_finish },
	{ 0, 0 }
};

static struct luaL_reg lcdb_make_funcs[] =
{
	{ "start",	lcdb_make_start },
	{ 0, 0 }
};

int
luaopen_cdb_make(lua_State *L)
{
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_replace(L, LUA_ENVIRONINDEX);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_register(L, 0, lcdb_make_methods);
	luaL_register(L, MYNAME, lcdb_make_funcs);
	return 1;
}
