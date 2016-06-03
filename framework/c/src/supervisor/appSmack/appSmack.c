//--------------------------------------------------------------------------------------------------
/** @file appSmack.c
 *
 * This component implements the appSmack API which manages the SMACK label for applications.
 *
 *  - @ref c_appSmack_Labels
 *
 * @section c_appSmack_Labels Application SMACK Labels
 *
 * Each Legato application is assigned a SMACK label.  This label is given to all the application's
 * processes and bundled files in the app's sandbox.
 *
 * An application does not need to exist before it is given a SMACK label.  In fact to satisfy
 * bindings between a client app and a server app, where the server app is not yet installed, a
 * SMACK label must be generated for the server before the server exists.
 *
 * This module only manages SMACK labels for applications.  All application labels have the "app."
 * prefix.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "smack.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appSmack_GetName
(
    int pid,                ///< [IN] PID of the process.
    char* bufPtr,           ///< [OUT] Buffer to hold the name of the app.
    size_t bufSize          ///< [IN] Size of the buffer.
)
{
    // Get the SMACK label for the process.
    char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];

    le_result_t result = smack_GetProcLabel(pid, smackLabel, sizeof(smackLabel));

    if (result != LE_OK)
    {
        return result;
    }

    // Strip the prefix from the label.
    if (strncmp(smackLabel, SMACK_APP_PREFIX, sizeof(SMACK_APP_PREFIX)-1) == 0)
    {
        return le_utf8_Copy(bufPtr, &(smackLabel[strlen(SMACK_APP_PREFIX)]), bufSize, NULL);
    }

    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's SMACK label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function will kill the client if there is an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appSmack_GetLabel
(
    const char* appName,
        ///< [IN]
        ///< The name of the application.

    char* label,
        ///< [OUT]
        ///< The smack label for the application.

    size_t labelNumElements
        ///< [IN]
)
{
    smack_GetAppLabel(appName, label, labelNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's the application's smack label with the access mode appended to it as a string.  For
 * example, if the accessMode is ACCESS_FLAG_READ | ACCESS_FLAG_WRITE then "rw" will be appended to
 * the application's smack label.  If the accessMode is 0 (empty) then "-" will be appended to the
 * app's smack label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 *
 * @warning
 *      This function will kill the client if there is an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void appSmack_GetAccessLabel
(
    const char* appName,
        ///< [IN]
        ///< The name of the application.

    appSmack_AccessFlags_t accessMode,
        ///< [IN]
        ///< The access mode flags.

    char* label,
        ///< [OUT]
        ///< The smack label for the application.

    size_t labelNumElements
        ///< [IN]
)
{
    // Get the app label.
    char appLabel[LIMIT_MAX_SMACK_LABEL_LEN];
    appSmack_GetLabel(appName, appLabel, sizeof(appLabel));

    // Translate the accessMode to a string.
    char modeStr[4] = "-";
    int index = 0;

    if ((accessMode & APPSMACK_ACCESS_FLAG_READ) > 0)
    {
        modeStr[index++] = 'r';
    }
    if ((accessMode & APPSMACK_ACCESS_FLAG_WRITE) > 0)
    {
        modeStr[index++] = 'w';
    }
    if ((accessMode & APPSMACK_ACCESS_FLAG_EXECUTE) > 0)
    {
        modeStr[index++] = 'x';
    }

    modeStr[index] = '\0';

    // Create the access label.
    if (snprintf(label, labelNumElements, "%s%s", appLabel, modeStr) >= labelNumElements)
    {
        LE_KILL_CLIENT("User buffer is too small to hold SMACK access label %s for app %s.",
                       appLabel, appName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * App info's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
