//--------------------------------------------------------------------------------------------------
/** @file args.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Our copy of argc.
 */
//--------------------------------------------------------------------------------------------------
static size_t Argc = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Our pointer to argv.
 */
//--------------------------------------------------------------------------------------------------
static char** Argv = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Gets the program name.  The name of the running program is copied into the provided buffer.
 *
 * The name is assumed to be a UTF-8 string.  UTF-8 characters may be more than one byte long and
 * this function will only copy whole characters not partial characters.  Therefore, even
 * if LE_OVERFLOW is returned, argBufferPtr may not be completely filled.  See @ref utf8_trunc for
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
    char*   nameBuffPtr,    ///< [OUT] The buffer to copy the program name to.
    size_t  nameBuffSize,   ///< [IN] The size of the buffer.
    size_t* nameLenPtr      ///< [OUT] The length of the name copied to the buffer not including the
                            ///        NULL-terminator.  This can be NULL if the name length is not
                            ///        needed.
)
{
    if (Argv == NULL)
    {
        return LE_NOT_FOUND;
    }

    return le_utf8_Copy(nameBuffPtr, le_path_GetBasenamePtr(Argv[0], "/"), nameBuffSize, nameLenPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the number of command line arguments available not including the program name.
 *
 * @return
 *      The number of command line arguments available.
 */
//--------------------------------------------------------------------------------------------------
size_t le_arg_NumArgs
(
    void
)
{
    if (Argc == 0)
    {
        return 0;
    }

    return Argc - 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the command line argument by index.  If argIndex is valid then the argument string is copied
 * into argBufferPtr.  At most only argBufferSize - 1 bytes will be copied to argBufferPtr and a
 * terminating NULL character will always be inserted after the last copied byte.
 *
 * The argument string is assumed to be a UTF-8 string.  UTF-8 characters may be more than one byte
 * long and this function will only copy whole characters not partial characters.  Therefore, even
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
    const size_t    argIndex,       ///< [IN] The index of the argument to get.
    char*           argBufferPtr,   ///< [OUT] The buffer to copy the argument to.
    size_t          argBufferSize   ///< [IN] The size of the buffer.
)
{
    if ( (argIndex >= Argc - 1) || (Argc == 0) )
    {
        return LE_NOT_FOUND;
    }

    return le_utf8_Copy(argBufferPtr, Argv[argIndex + 1], argBufferSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets argc and argv for later use.  This function must be called by Legato's generated main
 * function.
 */
//--------------------------------------------------------------------------------------------------
void arg_SetArgs
(
    const size_t    argc,   ///< [IN] argc from main.
    char**          argv    ///< [IN] argv from main.
)
{
    Argc = argc;
    Argv = argv;
}
