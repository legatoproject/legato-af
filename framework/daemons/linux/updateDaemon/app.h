//--------------------------------------------------------------------------------------------------
/**
 * @file app.h
 *
 * App module exported definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_APP_H_INCLUDE_GUARD
#define LEGATO_APP_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * File system path to where apps are unpacked.
 **/
//--------------------------------------------------------------------------------------------------
extern const char* app_UnpackPath;



//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the given application exists.
 */
//--------------------------------------------------------------------------------------------------
bool app_Exists
(
    const char* app  ///< [IN] The name or the hash of the app to find.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the hash ID for the named application.
 *
 * @return On return the application's hash is copied into the supplied hashBuffer.
 */
//--------------------------------------------------------------------------------------------------
void app_Hash
(
    const char* appNamePtr,                 ///< [IN]  The name of the application to read.
    char hashBuffer[LIMIT_MD5_STR_BYTES]    ///< [OUT] Buffer to hold the application hash ID.
);


//--------------------------------------------------------------------------------------------------
/**
 * Prepare the app unpack directory for use (delete any old one and create a fresh empty one).
 */
//--------------------------------------------------------------------------------------------------
void app_PrepUnpackDir
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Setup smack permission for contents in app's read-only directory.
 *
 * @return LE_OK if successful, LE_FAULT if fails.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetSmackPermReadOnly
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set up a given app's writeable files in the "unpack" system.
 *
 * Files will be copied to the system unpack area based on whether an app with the same name
 * exists in the current system.
 *
 * @warning Assumes the app identified by the hash is installed in /legato/apps/<hash>.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetUpAppWriteables
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
);


//--------------------------------------------------------------------------------------------------
/**
 * Install a new individual application update in the current running system.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_DUPLICATE if requested to install same app.
 *      - LE_FAULT for any other failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_InstallIndividual
(
    const char* appMd5Ptr,  ///< [IN] Hash ID of the application to install.
    const char* appNamePtr  ///< [IN] Name of the application to install.
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove the named app from the current running system.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if requested to remove non-existent app.
 *      - LE_FAULT for any other failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_RemoveIndividual
(
    const char* appNamePtr  ///< [IN] Name of the app to remove.
);

//--------------------------------------------------------------------------------------------------
/**
 * Determine if an app update was interrupted, and if so finish it.
 */
//--------------------------------------------------------------------------------------------------
void app_FinishUpdates
(
    void
);

#endif // LEGATO_APP_H_INCLUDE_GUARD
