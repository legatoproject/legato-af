/**
 * @page c_args Command Line Arguments API
 *
 * @ref le_args.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * 
 * When a program starts, arguments may be passed from the command line.  
 * This sample uses the program <b>foo</b> is started with arguments <b>bar</b> and <b>barz</b>:
 *
 * @verbatim
$ foo bar barz
@endverbatim
 *
 *
 * The arguments can be accessed using @c le_arg_GetArg() using an index to specify which argument to
 * access.  The first argument has index 0, the second argument has index 1, etc.  The code sample
 *  argument <b>bar</b> has index 0 and the argument <b>barz</b> has index 1. 
 * The number of available arguments is obtained using @c le_arg_NumArgs().
 *
 * The name of the program is obtained using @c le_arg_GetProgramName().
 *
 * The program name and all arguments are assumed to be Null-terminated UTF-8 strings.  For more
 * information about UTF-8 strings see @ref c_utf8.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_args.h
 *
 * Legato @ref c_args include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ARGS_INCLUDE_GUARD
#define LEGATO_ARGS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Gets the program name.  The name of the running program is copied into the provided buffer.
 *
 * The name is assumed to be a UTF-8 string.  UTF-8 characters may be more than one byte long.
 * Only copies whole characters, not partial characters.  Even if LE_OVERFLOW is returned, 
 * argBufferPtr may not be completely filled.  See @ref utf8_trunc for
 * more details.
 *
 * The copied string is always Null-terminated if it is available.
 *
 * @return
 *      LE_OK if the program name was copied in full.
 *      LE_OVERFLOW if the program name was truncated when copied.
 *      LE_NOT_FOUND if the program is not available because argv and argc were not available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetProgramName
(
    char*   nameBuffPtr,    ///< [OUT] Buffer to copy the program name to.
    size_t  nameBuffSize,   ///< [IN] Size of the buffer.
    size_t* nameLenPtr      ///< [OUT] Length of the name copied to the buffer not including the
                            ///        NULL-terminator.  This can be NULL if the name length is not
                            ///        needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the number of command line arguments available not including the program name.
 *
 * @return
 *      Number of command line arguments available.
 */
//--------------------------------------------------------------------------------------------------
size_t le_arg_NumArgs
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the command line argument by index.  If argIndex is valid then the argument string is copied
 * into argBufferPtr. At most only argBufferSize - 1 bytes will be copied to argBufferPtr and a
 * terminating NULL character will always be inserted after the last copied byte.
 *
 * The argument string is assumed to be a UTF-8 string.  UTF-8 characters may be more than one byte
 * long. Only copies whole characters, not partial characters. Even
 * if LE_OVERFLOW is returned, argBufferPtr may not be completely filled.  See @ref utf8_trunc for
 * more details.
 *
 * @return
 *      LE_OK if the argument was copied to argBufferPtr.
 *      LE_OVERFLOW if the argument was truncated when it was copied to argBufferPtr.
 *      LE_NOT_FOUND if the specified argument is not available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetArg
(
    const size_t    argIndex,       ///< [IN] Index of the argument to get.
    char*           argBufferPtr,   ///< [OUT] Buffer to copy the argument to.
    size_t          argBufferSize   ///< [IN] Size of the buffer.
);


#endif // LEGATO_ARGS_INCLUDE_GUARD
