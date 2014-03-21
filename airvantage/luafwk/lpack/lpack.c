/***
  A library for packing and unpacking binary data.

The library adds two functions to the [string](http://www.lua.org/manual/5.1/manual.html#5.4) library: pack and unpack.

Supported letter codes:

- 'z': zero-terminated string
- 'p': string preceded by length byte
- 'P': string preceded by length word
- 'a': string preceded by length size_t
- 'A': string
- 'f': float
- 'd': double
- 'n': Lua number
- 'c': char
- 'b': byte = unsigned char
- 'h': short
- 'H': unsigned short
- 'i': int
- 'I': unsigned int
- 'l': long
- 'L': unsigned long
- 'x': unsigned char
- '<': little endian
- '>': big endian
- '=': native endian
- '{': unbreakable little endian
- '}': unbreakable big endian

This library was made by Luiz Henrique de Figueiredo (<lhf@tecgraf.puc-rio.br>) with
contributions from Ignacio Castaño (<castanyo@yahoo.es>) and Roberto Ierusalimschy (<roberto@inf.puc-rio.br>).
Original library can be found [here](http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/).

@module pack

@usage require"pack"
local bindata = string.pack("zf", "foo", 4.2)
local mystring,myfloat = string.unpack(bindata, "zf")
 */


/*
* lpack.c
* a Lua library for packing and unpacking binary data
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 29 Jun 2007 19:27:20
* This code is hereby placed in the public domain.
* with contributions from Ignacio Castaño <castanyo@yahoo.es> and
* Roberto Ierusalimschy <roberto@inf.puc-rio.br>.
*/

#define	OP_ZSTRING	'z'		/* zero-terminated string */
#define	OP_BSTRING	'p'		/* string preceded by length byte */
#define	OP_WSTRING	'P'		/* string preceded by length word */
#define	OP_SSTRING	'a'		/* string preceded by length size_t */
#define	OP_STRING	'A'		/* string */
#define	OP_FLOAT	'f'		/* float */
#define	OP_DOUBLE	'd'		/* double */
#define	OP_NUMBER	'n'		/* Lua number */
#define	OP_CHAR		'c'		/* char */
#define	OP_BYTE		'b'		/* byte = unsigned char */
#define	OP_SHORT	'h'		/* short */
#define	OP_USHORT	'H'		/* unsigned short */
#define	OP_INT		'i'		/* int */
#define	OP_UINT		'I'		/* unsigned int */
#define	OP_LONG		'l'		/* long */
#define	OP_ULONG	'L'		/* unsigned long */
#define OP_BOOL		'x'		/* unsigned char */
#define	OP_LITTLEENDIAN		'<'		/* little endian */
#define	OP_BIGENDIAN		'>'		/* big endian */
#define	OP_NATIVE			'='		/* native endian */
#define	OP_ULITTLEENDIAN	'{'		/* unbreakable little endian */
#define	OP_UBIGENDIAN		'}'		/* unbreakable big endian */

#include <ctype.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lpack.h"

static void badcode(lua_State *L, int c)
{
 char s[]="bad code `?'";
 s[sizeof(s)-3]=c;
 luaL_argerror(L,1,s);
}

static int doendian(int c)
{
 int x=1;
 int e=*(char*)&x;
 if (c==OP_LITTLEENDIAN) return !e;
 if (c==OP_ULITTLEENDIAN) return (!e)^80;
 if (c==OP_BIGENDIAN) return e;
 if (c==OP_UBIGENDIAN) return e^80;
 if (c==OP_NATIVE) return 0;
 return 0;
}

static void doswap(int swap, void *p, size_t n)
{
 char *a=p;
 int i,j;
 if (swap >= 80)
 {
  swap = swap^80;
  if (n%4==0)
  {
   for (i=0; i<n; i+=2)
   {
    char t=a[i]; a[i]=a[i+1]; a[i+1]=t;
   }
  }
 }
 if (swap)
 {
  for (i=0, j=n-1, n=n/2; n--; i++, j--)
  {
   char t=a[i]; a[i]=a[j]; a[j]=t;
  }
 }
}

#define UNPACKNUMBER(OP,T)		\
   case OP:				\
   {					\
    T a;				\
    int m=sizeof(a);			\
    if (i+m>len) goto done;		\
    memcpy(&a,s+i,m);			\
    i+=m;				\
    doswap(swap,&a,m);			\
    lua_pushnumber(L,(lua_Number)a);	\
    ++n;				\
    break;				\
   }

#define UNPACKINTEGER(OP,T)		\
   case OP:				\
   {					\
    T a;				\
    int m=sizeof(a);			\
    if (i+m>len) goto done;		\
    memcpy(&a,s+i,m);			\
    i+=m;				\
    doswap(swap,&a,m);			\
    lua_pushinteger(L,(lua_Integer)a);	\
    ++n;				\
    break;				\
   }

#define UNPACKSTRING(OP,T)		\
   case OP:				\
   {					\
    T l;				\
    int m=sizeof(l);			\
    if (i+m>len) goto done;		\
    memcpy(&l,s+i,m);			\
    doswap(swap,&l,m);			\
    if (i+m+l>len) goto done;		\
    i+=m;				\
    lua_pushlstring(L,s+i,l);		\
    i+=l;				\
    ++n;				\
    break;				\
   }

#define UNPACKBOOL(OP,T)      \
   case OP:             \
   {                    \
    T a;                \
    int bindex;         \
    int m=sizeof(a);            \
    if (i+m>len) goto done;     \
    memcpy(&a,s+i,m);           \
    i+=m;                       \
    /*doswap(swap,&a,m);*/          \
    for (bindex = 0; bindex < 8; bindex++) {    \
        if (a&(1<<bindex)) {                    \
            lua_pushboolean(L,1);   \
        } else {                                \
            lua_pushboolean(L,0);    \
        }                                       \
        ++n;                                    \
    }                                           \
    break;              \
   }


/***
 Unpacks a binary string into values.

 The letters codes are the same as for pack, except that numbers following 'A'
 are interpreted as the number of characters to read into the string and not as
 repetitions.
 The first value returned by unpack is the next unread position in s, which can
 be used as the init position in a subsequent call to unpack.
 This allows you to unpack values in a loop or in several steps.

 @function [parent=#pack] unpack
 @param s is a (binary) string containing data packed as if by pack
 @param F is a format string describing what is to be read from s
 @param init optional init marks where in s to begin reading the values
 @return the next unread position in s, followed by one value per letter in F until F or s is exhausted.
 @return nil if either s has been exhausted or init is bigger than the length of the s
 @usage
  require"pack"
  local _,mystring,myfloat = string.unpack(bindata, "zf")
*/
static int l_unpack(lua_State *L) 		/** unpack(s,f,[init]) */
{
 size_t len;
 const char *s=luaL_checklstring(L,1,&len);
 const char *f=luaL_checkstring(L,2);
 int i=luaL_optnumber(L,3,1)-1;
 int n=0;
 int swap=0;
 lua_pushnil(L);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f))
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
   if (N==0 && c==OP_STRING) { lua_pushliteral(L,""); ++n; }
  }
  if (c == OP_BOOL && ((N > 0 && N%8 != 0) || N == 0)) luaL_error(L, "number following 'x' should be a multiple of 8", i);
  lua_checkstack(L, N);
  if (c == OP_BOOL) N = N>>3;
  while (N--) switch (c)
  {
   case OP_LITTLEENDIAN:
   case OP_ULITTLEENDIAN:
   case OP_BIGENDIAN:
   case OP_UBIGENDIAN:
   case OP_NATIVE:
   {
    swap=doendian(c);
    N=0;
    break;
   }
   case OP_STRING:
   {
    ++N;
    if (i+N>len) goto done;
    lua_pushlstring(L,s+i,N);
    i+=N;
    ++n;
    N=0;
    break;
   }
   case OP_ZSTRING:
   {
    size_t l;
    if (i>=len) goto done;
    l=strlen(s+i);
    lua_pushlstring(L,s+i,l);
    i+=l+1;
    ++n;
    break;
   }
   UNPACKSTRING(OP_BSTRING, unsigned char)
   UNPACKSTRING(OP_WSTRING, unsigned short)
   UNPACKSTRING(OP_SSTRING, size_t)
   UNPACKNUMBER(OP_NUMBER, lua_Number)
   UNPACKNUMBER(OP_DOUBLE, double)
   UNPACKNUMBER(OP_FLOAT, float)
   UNPACKINTEGER(OP_CHAR, char)
   UNPACKINTEGER(OP_BYTE, unsigned char)
   UNPACKINTEGER(OP_SHORT, short)
   UNPACKINTEGER(OP_USHORT, unsigned short)
   UNPACKINTEGER(OP_INT, int)
   UNPACKINTEGER(OP_UINT, unsigned int)
   UNPACKINTEGER(OP_LONG, long)
   UNPACKINTEGER(OP_ULONG, unsigned long)
   UNPACKBOOL(OP_BOOL, unsigned char)
   case ' ': case ',':
    break;
   default:
    badcode(L,c);
    break;
  }
 }
done:
 lua_pushnumber(L,i+1);
 lua_replace(L,-n-2);
 return n+1;
}

#define PACKNUMBER(OP,T)			\
   case OP:					\
   {						\
    T a=(T)luaL_checknumber(L,i++);		\
    doswap(swap,&a,sizeof(a));			\
    luaL_addlstring(&b,(void*)&a,sizeof(a));	\
    break;					\
   }

#define PACKSTRING(OP,T)			\
   case OP:					\
   {						\
    size_t l;					\
    const char *a=luaL_checklstring(L,i++,&l);	\
    T ll=(T)l;					\
    doswap(swap,&ll,sizeof(ll));		\
    luaL_addlstring(&b,(void*)&ll,sizeof(ll));	\
    luaL_addlstring(&b,a,l);			\
    break;					\
   }

#define PACKBOOL(OP,T)            \
   case OP:                 \
   {                        \
    T a = 0;     \
    int bindex;  \
    for (bindex = 0; bindex < 8; bindex++) {                \
        if (lua_isboolean(L,i)) {                         \
            a = a | ( (T)lua_toboolean(L,i) << bindex);     \
            /*doswap(swap,&a,sizeof(a));*/                  \
        } else {                                            \
            luaL_error(L, "value %d should be a boolean", i);     \
        }                                                   \
        i++;                                                \
    }                                                       \
    luaL_addlstring(&b,(void*)&a,sizeof(a));    \
    break;                  \
   }

/***
 Pack binary data into a string.

 The letter codes understood by pack are listed above in module description.
 Numbers following letter codes indicate repetitions.

 @function [parent=#pack] pack
 @param F a string describing how the values x1, x2, ... are to be interpreted and formatted.
  Each letter in the format string F consumes one of the given values.
 @param x1, x2, ... : variable number of values. Only values of type number or string are accepted.
 @return a (binary) string containing the values packed as described in F.
 @usage require"pack"
local bindata = string.pack("zf", "foo", 4.2)
 */
static int l_pack(lua_State *L) 		/** pack(f,...) */
{
 int i=2;
 const char *f=luaL_checkstring(L,1);
 int swap=0;
 luaL_Buffer b;
 luaL_buffinit(L,&b);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f))
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
  }
  if (c == OP_BOOL && ((N > 0 && N%8 != 0) || N == 0)) luaL_error(L, "number following 'x' should be a multiple of 8", i);
  if (c == OP_BOOL) N = N>>3;
  while (N--) switch (c)
  {
   case OP_LITTLEENDIAN:
   case OP_ULITTLEENDIAN:
   case OP_BIGENDIAN:
   case OP_UBIGENDIAN:
   case OP_NATIVE:
   {
    swap=doendian(c);
    N=0;
    break;
   }
   case OP_STRING:
   case OP_ZSTRING:
   {
    size_t l;
    const char *a=luaL_checklstring(L,i++,&l);
    luaL_addlstring(&b,a,l+(c==OP_ZSTRING));
    break;
   }
   PACKSTRING(OP_BSTRING, unsigned char)
   PACKSTRING(OP_WSTRING, unsigned short)
   PACKSTRING(OP_SSTRING, size_t)
   PACKNUMBER(OP_NUMBER, lua_Number)
   PACKNUMBER(OP_DOUBLE, double)
   PACKNUMBER(OP_FLOAT, float)
   PACKNUMBER(OP_CHAR, char)
   PACKNUMBER(OP_BYTE, unsigned char)
   PACKNUMBER(OP_SHORT, short)
   PACKNUMBER(OP_USHORT, unsigned short)
   PACKNUMBER(OP_INT, int)
   PACKNUMBER(OP_UINT, unsigned int)
   PACKNUMBER(OP_LONG, long)
   PACKNUMBER(OP_ULONG, unsigned long)
   PACKBOOL(OP_BOOL, unsigned char)
   case ' ': case ',':
    break;
   default:
    badcode(L,c);
    break;
  }
 }
 luaL_pushresult(&b);
 return 1;
}

static const luaL_reg R[] =
{
	{"pack",	l_pack},
	{"unpack",	l_unpack},
	{NULL,	NULL}
};

LPACK_API int luaopen_pack(lua_State *L)
{
#ifdef USE_GLOBALS
 lua_register(L,"bpack",l_pack);
 lua_register(L,"bunpack",l_unpack);
#else
 luaL_openlib(L, LUA_STRLIBNAME, R, 0);
#endif
 return 0;
}
