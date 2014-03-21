/*******************************************************************************
 *
 * Copyright (c) 2012 Sierra Wireless and others.
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

/**
 Argument type checking API.

This library declares a `checks()` function and a `checkers` table, which
allow to check the parameters passed to a Lua function in a fast and
unobtrusive  way.

`checks (type_1, ..., type_n)`, when called directly inside function
`f`, checks that `f`'s 1st argument conforms to `type_1`, that its 2nd
argument conforms to `type_2`, etc. until `type_n`. Type specifiers
are strings, and if the arguments passed to `f` don't conform to their
specification, a proper error message is produced, pinpointing the
call to `f` as the faulty expression.

Each type description `type_n` must be a string, and can describe:

* the Lua type of an object, such as `"table"`, `"number"` etc.;
* an arbitrary name, which would be stored in the `__type` field of
  the argument's metatable;
* a type-checking function, which would be stored in the `checkers`
  global table. This table uses type names as keys, test functions
  returning Booleans as keys.

Moreover, types can be prefixed with a `"?"`, which makes them
optional. For instance, `"?table"` accepts tables as well as `nil`
values.

A `"?"` alone accepts anything. It is mainly useful as a placeholder,
to skip an argument which doesn't need to be checked.

A `"!"` accepts everything except `nil`.

Finally, several types can be accepted, if their names are
concatenated with a bar `"|"` between them. For instance,
`"table|number"` accepts tables as well as numbers. It can be combined
with the question mark, so `"?table|number"` accepts tables, numbers
and nil values. It is actually equivalent to `"nil|table|number"`.

More formally, let's specify `conform(a, t)`, the property that
argument `a` conforms to the type denoted by `t`. `conform(a,t)` is
true if and only if at least one of the following propositions is
verified:

 * `conforms(a, t:match "^(.-)|.*"`
 * `t == "?"`
 * `t == "!" and a ~= nil`
 * `t:sub(1, 1) == "?" and (conforms(a, t:sub(2, -1)) or a==nil)`
 * `type(a) == t`
 * `getmetatable(a) and getmetatable(a).__type == t`
 * `checkers[t] and checkers[t](a) is true`
 * `conforms(a, t:match "^.-|(.*)")`

 The above propositions are listed in the order in which they are
 tried by `check`. The higher they appear in the list, the faster
 `checks` accepts aconforming argument. For instance,
 `checks("number")` is faster than
 `checkers.mynumber=function(x) return type(x)=="number" end; checks("mynumber")`.

Usage examples
--------------

     require 'checks'

     -- Custom checker function --
     function checkers.port(p)
       return type(p)=='number' and p>0 and p<0x10000
     end

     -- A new named type --
     socket_mt = { __type='socket' }
     asocket = setmetatable ({ }, socket_mt)

     -- A function that checks its parameters --
     function take_socket_then_port_then_maybe_string (sock, port, str)
       checks ('socket', 'port', '?string')
     end

     take_socket_then_port_then_maybe_string (asocket, 1024, "hello")
     take_socket_then_port_then_maybe_string (asocket, 1024)

     -- A couple of other parameter-checking options --

     function take_number_or_string()
       checks("number|string")
     end

     function take_number_or_string_or_nil()
       checks("?number|string")
     end

     function take_anything_followed_by_a_number()
       checks("?", "number")
     end

     -- Catch some incorrect arguments passed to the function --

     function must_fail(...)
       assert (not pcall (take_socket_then_port_then_maybe_string, ...))
     end

     must_fail ({ }, 1024, "string")      -- 1st argument isn't a socket
     must_fail (asocket, -1, "string")   -- port number must be 0-0xffff
     must_fail (asocket, 1024, { })    -- 3rd argument cannot be a table

Caveat
------

`checks()` doesn't work properly on function arguments which are part of a
`...` variable parameters list. For instance, the behavior of the following
program is undefined:

     function f(...)
         checks('string')
     end
     f("some_string")

@module checks

*/


#include "checks.h"
#include "lauxlib.h"
#include <string.h>

/** Generate and throw an error.
@function [parent=#global] error
@param level stack level where the error must be reported
@param narg indice of the erroneous argument
@param expected name of the expected type
@param got name of the type actually found

@return never returns (throws a Lua error instead) */
static int error(
        lua_State *L, int level, int narg,
        const char *expected, const char *got) {
    lua_Debug ar;
    lua_getstack( L, level, & ar);
    lua_getinfo( L, "n", & ar);
    luaL_where( L, level+1);
    lua_pushfstring( L, " bad argument #%d to %s (%s expected, got %s)",
            narg, ar.name, expected, got);
    lua_concat( L, 2);

#if 0 /* Debugging cruft */
    int i;
    for(i=0;i<10;i++){
        if( ! lua_getstack( L, i, & ar)) break;
        lua_getinfo( L, "n", & ar);
        printf( "\tat level %d: '%s' / '%s'\n", i, ar.name, ar.namewhat);
    }
    printf( "\tend of error, level was %d\n\n", level);
#endif

    return lua_error( L);
}

#define WITH_SUM_TYPES
#ifdef  WITH_SUM_TYPES

/** Return true iff actualType occurs in expecteTypes, the later being
a list of type names separate by '|' chars.

If `WITH_SUM_TYPES` is disabled, the expectedTypes list must have one
element, i.e. no '|' separator character.

@function [parent=#global] matches
@param  actualType the type of the tested object
@param  expectedTypes the list of types listed as acceptable in `checks()`
 for this argument
@return whether `actualType` is listed in `expectedTypes`.
*/
static int matches( const char *actualType, const char *expectedTypes) {
    const char *p = expectedTypes;
    if( ! strcmp( actualType, expectedTypes)) return 1;
    while( 1) {
        const char *q = strchr( p, '|');
        if( ! q) {
            if( p == expectedTypes) return 0; // 1st loop, no '|' at all
            else q = p+strlen( expectedTypes);  // last loop
        }
        if( ! strncmp( p, actualType, q-p)) return 1;
        if( ! *q) return 0; else p = q+1;
    };
}
#else
#    define matches( a, b) ( ! strcmp( a, b))
#endif

/***
check whether the calling function's argument have the expected types.

`checks( [level], t_1, ..., t_n)` causes an error if the type of
argument #`i` in stack frame #`level` isn't as described by `t_i`, for
i in `[1...n]`.  `level` is optional, it defaults to one (checks the
function immediately calling `checks`).

@function [parent=#global] checks
@param level the number of stack levels to ignore in the error message,
 should it be produced. Optional, defaults to 1.
@param varargs one type string per expected argument.
@return nothing on success, throw an error on failure.
*/
static int checks( lua_State *L) {
    lua_Debug ar;
    int level=1, i=1, r;
    if( lua_isnumber( L, 1)) { i = 2; level = lua_tointeger( L, 1); }
    r = lua_getstack( L, level, & ar);
    if( ! r) luaL_error( L, "checks() must be called within a Lua function");

    /* loop for each checked argument in stack frame. */
    for( /* i already initialized. */; ! lua_isnoneornil( L, i); i++) {

        const char *expectedType = luaL_checkstring( L, i); // -
        lua_getlocal( L, & ar, i);  // val (value whose type is checked)

        /* 1. Check for nil if type is optional. */
        if( '?' == expectedType[0]) {
            if( ! expectedType[1]   /* expectedType == "?".   */
            || lua_isnoneornil( L, -1)) { /* actualType   == "nil". */
                lua_pop( L, 1); continue; // -
            }
            expectedType++;
        }

        const char *actualType = lua_typename( L, lua_type( L, -1));

        /* 1'. if the template is "!", check for non-nilness. */
        if( '!' == expectedType[0] && '\0' == expectedType[1]) {
            if( lua_isnoneornil( L, -1)) { // val==nil
                return error( L, level, i, "non-nil", actualType);
            } else { // val~=nil
                lua_pop( L, 1); continue; // -
            }
        }

        /* 2. Check real type. */
        if( matches( actualType, expectedType)) {
            lua_pop( L, 1); continue; // -
        }

        /* 3. Check for type name in metatable. */
        if( lua_getmetatable( L, -1)) {       // val, mt
            lua_getfield( L, -1, "__type");   // val, mt, __type?
            if( lua_isstring( L, -1)) {       // val, mt, __type
                if( matches( luaL_checkstring( L, -1), expectedType)) {
                    lua_pop( L, 3); continue; // -
                } else { /* non-matching __type field. */
                    lua_pop( L, 2);           // val
                }
            } else { /* no __type field. */
                lua_pop( L, 2);               // val
            }
        } else { /* no metatable. */ }        // val

        /* 4. Check for a custom typechecking function.  */
        lua_getfield( L, LUA_REGISTRYINDEX, "checkers");         // val, checkers
        const char *p = expectedType;
        while( 1) {
            const char *q = strchr( p, '|');
            if( ! q) q = p + strlen( p);
            lua_pushlstring( L, p, q-p);                // val, checkers, expType
            lua_gettable( L, -2);             // val, checkers, checkers.expType?
            if( lua_isfunction( L, -1)) {
                lua_pushvalue( L, -3);    // val, checkers, checkers.expType, val
                r = lua_pcall( L, 1, 1, 0);       // val, checkers, result || msg
                if( ! r && lua_toboolean( L, -1)) {// val, checkers, result==true
                    lua_pop( L, 3);                                          // -
                    break;
                } else {                               // val, checkers, errormsg
                    lua_pop( L, 1);                              // val, checkers
                }
            } else { /* no such custom checker */           // val, checkers, nil
                lua_pop( L, 1);                                  // val, checkers
            }
            if( ! *q) { /* last possible expected type. */
                lua_pop( L, 2);
                return error( L, level, i, expectedType, actualType);
            } else {
                p = q+1;
            }
        } /* for each expected type */
    } /* for each i */
    return 0;
}

/***
Table of custom type-checkers.  This table contain type-checking
functions, indexed by type name.  If an argument `a` is expected to be
of type `t`, and neither `type(a)` nor `getmetatable(a).__type` return
`t`, but `checkers[t]` contains a function, this function will be
called, with `a` as its only argument. If the function returns `true`,
then `a` is considered to be of type `t`.

Example
-------

     -- Create the type-checking function --
     function checkers.positive_number(x)
       return type(x)=='number' and x>0
     end

     -- Use the `positive_number` type-checking function --
     function sqrt(x)
       checks('positive_number')
       return x^(1/2)
     end

@type checkers
 */

int luaopen_checks( lua_State *L) {
    lua_newtable( L);                                // checkers
    lua_pushvalue( L, -1);                 // checkers, checkers
    lua_setfield( L, LUA_REGISTRYINDEX, "checkers"); // checkers
    lua_setglobal( L, "checkers");                          // -
    lua_pushcfunction( L, checks);                     // checks
    lua_pushvalue( L, -1);                     // checks, checks
    lua_setglobal( L, "checks");                       // checks
    return 1;
}
