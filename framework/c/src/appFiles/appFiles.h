//--------------------------------------------------------------------------------------------------
/**
 * Install/remove operations pertaining to an app's files, inclucing file permissions,
 * and SMACK labels.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef APP_FILES_H_INCLUDE_GUARD
#define APP_FILES_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Sets file permissions and SMACK labels for an application's files according to the settings in
 * the configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
LE_SHARED void appFiles_SetPermissions
(
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's files from the file system.
 *
 * @return LE_OK on success.
 **/
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t appFiles_Remove
(
    const char* appNamePtr
);


#endif // APP_FILES_H_INCLUDE_GUARD
