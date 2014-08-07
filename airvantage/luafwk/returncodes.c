/*******************************************************************************
 * Copyright (c) 2013 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *     Romain Perier  for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/* Wraps returncodes library, which converts between string and numeric
 * representations of status codes. */

#include <returncodes.h>
#include <lauxlib.h>


/**
 Common return codes.

This library provides an homogeneous return code list for Agent related APIs.

Here is the list of defined return codes:

- `"RC_OK"`: Successful.
- `"RC_NOT_FOUND"`: The referenced item does not exist or could not be found.
- `"RC_OUT_OF_RANGE"`: An index or other value is out of range.
- `"RC_NO_MEMORY"`: Insufficient memory is available.
- `"RC_NOT_PERMITTED"`: Current user does not have permission to perform requested action.
- `"RC_UNSPECIFIED_ERROR"`: An unspecified error happened.
- `"RC_COMMUNICATION_ERROR"`: Communications error.
- `"RC_TIMEOUT"`: A time-out occurred.
- `"RC_WOULD_BLOCK"`: Would have blocked if non-blocking behavior was not requested.
- `"RC_DEADLOCK"`: Would have caused a deadlock.
- `"RC_BAD_FORMAT"`: Inputs or data are not formated correctly.
- `"RC_DUPLICATE"`: Duplicate entry found or operation already performed.
- `"RC_BAD_PARAMETER"`: Parameter is not valid.
- `"RC_CLOSED"`: The file, stream or object was closed.
- `"RC_IO_ERROR"`: An IO error occurred.
- `"RC_NOT_IMPLEMENTED"`: This feature is not implemented.
- `"RC_BUSY"`: The component or service is busy.
- `"RC_NOT_INITIALIZED"`: The service or object is not initialized.
- `"RC_END"`: The file, stream or buffer reached the end.
- `"RC_NOT_AVAILABLE"`: The service is not available.

@module returncodes

*/

/**

@field toto titi

*/


/**
 Converts a name into a numeric status.
 
@function [parent=#returncodes] tonumber
@param code, a string representing the return code to be converted into a number
@return the integer representing the return code, or `1` if unknown.
*/
static int api_tonumber( lua_State *L) {
    const char *name = luaL_checkstring( L, 1);
    lua_pushinteger( L, rc_StringToReturnCode( name));
    return 1;
}


/**
 Converts a numeric status into a name, 
@function [parent=#returncodes] tostring
@param code, an integer representing the return code to be converted into a string
@return the string representing the return code, or `nil` if not found.
*/
static int api_tostring( lua_State *L) {
    int num = luaL_checkinteger( L, 1);
    lua_pushstring( L, rc_ReturnCodeToString( num));
    return 1;
}

/* Loads the library. */
int luaopen_returncodes( lua_State *L) {
    lua_newtable( L);
    lua_pushcfunction( L, api_tonumber);
    lua_setfield( L, -2, "tonumber");
    lua_pushcfunction( L, api_tostring);
    lua_setfield( L, -2, "tostring");
    return 1;
}

