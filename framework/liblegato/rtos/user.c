//--------------------------------------------------------------------------------------------------
/** @file user.c
 *
 * Stub API for simulating users and groups on RTOS.
 *
 * On RTOS treat everyone as 'root' user.
 *
 * @note Since there is no actual user/group management, APIs are added here on an as-needed basis.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Gets a user name from a user ID.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the user name was copied.
 *      LE_NOT_FOUND if the user was not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetName
(
    uid_t    uid,           ///< [IN]  The uid of the user to get the name for.
    char    *nameBufPtr,    ///< [OUT] The buffer to store the user name in.
    size_t   nameBufSize    ///< [IN]  The size of the buffer that the user name will be stored in.
)
{
    // Only simulate root user on RTOS.
    if (uid != 0)
    {
        return LE_NOT_FOUND;
    }

    return le_utf8_Copy(nameBufPtr, "root", nameBufSize, NULL);
}
