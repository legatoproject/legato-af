//--------------------------------------------------------------------------------------------------
/** @file user.h
 *
 * API for creating/deleting Linux users and groups.
 *
 * This API is thread-safe.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#ifndef LEGATO_SRC_USER_INCLUDE_GUARD
#define LEGATO_SRC_USER_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the user system.  This should be called before any other functions in this API.
 */
//--------------------------------------------------------------------------------------------------
void user_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Restores the passwd and/or group backup files if the backup files exist.  This function should be
 * called once on system startup.
 *
 * A backup copy of the passwd and/or group files is created when either of them is modified.  The
 * backups are automatically deleted when the modifications are completed.  If the modification is
 * interrupted by a power outage this function will restore the back up files.
 */
//--------------------------------------------------------------------------------------------------
void user_RestoreBackup
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a user with the specified name, and home directory.  A group with the same name as the
 * username will also be created and the group will be set as the user's primary group.  If the user
 * and group are successfully created the user ID and group ID are stored at the location pointed to
 * by uidPtr and gidPtr respectively.  If there was an error the user is not created and the values
 * at uidPtr and gidPtr are undefined.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the user or group alreadly exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_Create
(
    const char* usernamePtr,    ///< [IN] Pointer to the name of the user and group to create.
    const char* homeDirPtr,     ///< [IN] Pointer to the home directory for this user.
    uid_t* uidPtr,              ///< [OUT] Pinter to a location to store the uid for the created
                                ///        user.  This can be NULL if the uid is not needed.
    gid_t* gidPtr               ///< [OUT] Pointer to a location to store the gid for the created
                                ///        user.  This can be NULL if the gid is not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a group with the specified name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the group alreadly exists.  If the group already exists the gid of the group
 *                   is still returned in *gidPtr.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_CreateGroup
(
    const char* groupNamePtr,    ///< [IN] Pointer to the name of the group to create.
    gid_t* gidPtr                ///< [OUT] Pointer to store the gid.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a user and its primary group.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the user could not be found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_Delete
(
    const char* usernamePtr     ///< [IN] Pointer to the name of the user to delete.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a group.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the group could not be found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_DeleteGroup
(
    const char* groupNamePtr     ///< [IN] Pointer to the name of the group to delete.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID and group ID of a user and stores them in the locations pointed to by uidPtr and
 * gidPtr.  If there was an error then the values at uidPtr and gidPtr are undefined.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the user does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetIDs
(
    const char* usernamePtr,    ///< [IN] Pointer to the name of the user to get.
    uid_t* uidPtr,              ///< [OUT] Pinter to a location to store the uid for this user.
                                ///        This can be NULL if the uid is not needed.
    gid_t* gidPtr               ///< [OUT] Pointer to a location to store the gid for this user.
                                ///        This can be NULL if the gid is not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID from a user name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the user does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetUid
(
    const char* usernamePtr,    ///< [IN] Pointer to the name of the user to get.
    uid_t* uidPtr               ///< [OUT] Pointer to store the uid.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the group ID from a group name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the group does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetGid
(
    const char* groupNamePtr,   ///< [IN] Pointer to the name of the group.
    gid_t* gidPtr                ///< [OUT] Pointer to store the gid.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a user name from a user ID.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the user name was copied.
 *      LE_NOT_FOUND if the user was not found.
 *      LE_FAULT if there was an error getting the user name.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetName
(
    uid_t uid,                  ///< [IN] The uid of the user to get the name for.
    char* nameBufPtr,           ///< [OUT] The buffer to store the user name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the user name will be stored in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a group name from a group ID.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the group name was copied.
 *      LE_NOT_FOUND if the group was not found.
 *      LE_FAULT if there was an error getting the group name.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetGroupName
(
    gid_t gid,                  ///< [IN] The gid of the group to get the name for.
    char* nameBufPtr,           ///< [OUT] The buffer to store the group name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the group name will be stored in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's name for a user.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the name was copied.
 *      LE_NOT_FOUND if the user does not have an application.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetAppName
(
    uid_t uid,                  ///< [IN] The uid of the user.
    char* nameBufPtr,           ///< [OUT] The buffer to store the app name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the app name will be stored in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts an application's name to a user name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the name was copied.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_ConvertToUserName
(
    const char* appName,        ///< [IN] The application's name.
    char* nameBufPtr,           ///< [OUT] The buffer to store the user name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the user name will be stored in.
);


#endif  // LEGATO_SRC_USER_INCLUDE_GUARD
