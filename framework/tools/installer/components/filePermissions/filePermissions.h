//--------------------------------------------------------------------------------------------------
/**
 * Sets file permissions and SMACK labels for an installled application's files according to the
 * settings in the configuration tree.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef APP_FILE_PERMISSIONS_H_INCLUDE_GUARD
#define APP_FILE_PERMISSIONS_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Sets file permissions and SMACK labels for an application's files according to the settings in
 * the configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void filePermissions_Set
(
    const char* appNamePtr
);


#endif // APP_FILE_PERMISSIONS_H_INCLUDE_GUARD
