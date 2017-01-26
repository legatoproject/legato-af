//--------------------------------------------------------------------------------------------------
/** @file user.c
 *
 * API for creating/deleting Linux users and groups.
 *
 * Users are created and deleted by modifying the /etc/passwd files.  When a function is called to
 * create or delete a user a backup of the passwd file is first created.  The passwd file is then
 * modified accordingly and then the backup file is deleted.  If the device is shutdown/restarted
 * while adding/deleting a user the passwd file may be partially modified and in a bad state.  The
 * system can recover from this by restoring the backup file using the user_RestoreBackup() call.
 *
 * The passwd file is always locked when they are opened for reading or writing.  This ensures that
 * the file does not get corrupted due to multiple simultaneous access.  Locking the file allows
 * this API to be thread safe.  However, the file locking mechanism used here is only advisory which
 * means that other threads may access the passwd file simultaneously if they are not using this
 * API.
 *
 * The file locking mechanism used here is blocking so a deadlock will occur if an attempt is made
 * to obtain a lock on a file that has already been locked in the same thread.  This API
 * implementation contains helper functions and API functions.  The API functions are responsible
 * for locking the file and calling the helper functions while the helper functions do the actual
 * work.  The helper functions never locks the file.  This allows helper functions to be re-used
 * without causing deadlocks.
 *
 * Groups are created and deleted by modifying the /etc/group file.  File backup and recover and
 * file locking is handled in the same way as the passwd file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "user.h"
#include "limit.h"
#include <pwd.h>
#include <grp.h>
#include <sys/file.h>
#include "fileDescriptor.h"
#include <sys/sendfile.h>


//--------------------------------------------------------------------------------------------------
/**
 * The local user and group ID ranges read from /etc/login.defs.
 */
//--------------------------------------------------------------------------------------------------
static uid_t MinLocalUid = 1000;
static uid_t MaxLocalUid = 60000;
static gid_t MinLocalGid = 1000;
static gid_t MaxLocalGid = 60000;


//--------------------------------------------------------------------------------------------------
/**
 * The local user and group ID range strings in /etc/login.defs.
 */
//--------------------------------------------------------------------------------------------------
#define UID_MIN_STR         "UID_MIN"
#define UID_MAX_STR         "UID_MAX"
#define GID_MIN_STR         "GID_MIN"
#define GID_MAX_STR         "GID_MAX"


//--------------------------------------------------------------------------------------------------
/**
 * Username prefix.  The prefix to prepend to the application name to create the username for the
 * application.
 */
//--------------------------------------------------------------------------------------------------
#define USERNAME_PREFIX                                 "app"


//--------------------------------------------------------------------------------------------------
/**
 * The maximum size in bytes of a password entry and group entry.  The initial default values are a
 * best guess.  These values may be updated on initialization.
 */
//--------------------------------------------------------------------------------------------------
static size_t MaxPasswdEntrySize = LIMIT_MAX_PATH_BYTES * 3;
static size_t MaxGroupEntrySize = LIMIT_MAX_PATH_BYTES;


//--------------------------------------------------------------------------------------------------
/**
 * Names of the password file, group file and temporary password and group files and the login
 * definition file.
 */
//--------------------------------------------------------------------------------------------------
#define PASSWORD_FILE           "/etc/passwd"
#define GROUP_FILE              "/etc/group"
#define BACKUP_PASSWORD_FILE    "/etc/passwd.bak"
#define BACKUP_GROUP_FILE       "/etc/group.bak"
#define LOGIN_DEF_FILE          "/etc/login.defs"


//--------------------------------------------------------------------------------------------------
/**
 * Updates the user or group ID range value from a string.  If the string contains the value to
 * update, this function parses the string and updates the value.
 *
 * @return
 *      true if the value was updated successfully.
 *      false if the value was not updated.
 */
//--------------------------------------------------------------------------------------------------
static bool UpdateLocalUidGidFromStr
(
    const char* strPtr,
    const char* nameOfValueToUpdatePtr,
    int* valueToUpdatePtr
)
{
    if ( (strncmp(strPtr, nameOfValueToUpdatePtr, sizeof(nameOfValueToUpdatePtr) - 1) == 0) &&
                 (le_utf8_NumBytes(strPtr) > sizeof(nameOfValueToUpdatePtr)) )
    {
        // Get the value.
        char* valueStr = (char*)strPtr + sizeof(nameOfValueToUpdatePtr);
        char* endPtr;
        errno = 0;
        int value = strtol(valueStr, &endPtr, 10);

        if ( (errno == 0) && (endPtr != valueStr) )
        {
            // This is a valid number.
            *valueToUpdatePtr = value;
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFile
(
    const char* fileNamePtr
)
{
    if ( (unlink(fileNamePtr) == -1) && (errno != ENOENT) )
    {
        LE_ERROR("Could not delete file '%s'.  %m", fileNamePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Flushes the file stream.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FlushFile
(
    FILE* filePtr           ///< [IN] Pointer to the file stream to flush.
)
{
    int result;
    do
    {
        result = fflush(filePtr);
    }
    while ( (result != 0) && (errno == EINTR) );

    if (result != 0)
    {
        LE_ERROR("Cannot flush stream.  %m.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets a file to a specified size.  If the size is smaller than the original file size, the file is
 * truncated and the extra data is lost.  If the size is larger than the original file size, the
 * file is extended and the extended portion is filled with NULLs ('\0').
 *
 * The file must be opened for writing.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetFileLength
(
    FILE* filePtr,          ///< [IN] Pointer to the file to size.
    off_t size              ///< [IN] The size in bytes to set the file to.
)
{
    // Flush the file stream so that we can start using low level file I/O functions.
    if (FlushFile(filePtr) != LE_OK)
    {
        return LE_FAULT;
    }

    // Get the file descriptor for this stream.
    int fd = fileno(filePtr);
    LE_FATAL_IF(fd == -1, "Could not get the file descriptor for a stream.  %m.");

    // Truncate the file to the desired length.
    int result;
    do
    {
        result = ftruncate(fd, size);
    }
    while ( (result == -1) && (errno == EINTR) );

    if (result == -1)
    {
        LE_ERROR("Could not set the file size.  %m.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies the contents of original file to the new file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CopyFile
(
    FILE* origFilePtr,          ///< [IN] Pointer to the original file.
    FILE* newFilePtr            ///< [IN] Pointer to the the new file.
)
{
    // Start at the beginning of each file.
    rewind(origFilePtr);
    rewind(newFilePtr);

    // Copy the contents of the orig file to the new file.
    char buf[MaxPasswdEntrySize];
    while(fgets(buf, sizeof(buf), origFilePtr) != NULL)
    {
        if (fputs(buf, newFilePtr) < 0)
        {
            LE_ERROR("Cannot copy file.  %m.");
            return LE_FAULT;
        }
    }

    if (FlushFile(newFilePtr) != LE_OK)
    {
        return LE_FAULT;
    }

    sync();

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a backup file.  Creates a backup file and copies the contents of the original file into it.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MakeBackup
(
    FILE* origFilePtr,                  ///< [IN] Pointer to the file that will be backed up.
    const char* backupFileNamePtr       ///< [IN] Pointer to the file name of the backup file to
                                        ///       create.
)
{
    // Create the backup file and open it for writing.
    FILE* backupFilePtr = le_flock_CreateStream(backupFileNamePtr, LE_FLOCK_WRITE,
                                                LE_FLOCK_REPLACE_IF_EXIST, S_IRUSR | S_IWUSR, NULL);

    if (backupFilePtr == NULL)
    {
        return LE_FAULT;
    }

    // Copy the orig file into the backup file.
    if (CopyFile(origFilePtr, backupFilePtr) == LE_FAULT)
    {
        le_flock_CloseStream(backupFilePtr);
        DeleteFile(backupFileNamePtr);

        return LE_FAULT;
    }

    le_flock_CloseStream(backupFilePtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restore backup file.  Copies the contents of the backup file into the original file and then
 * deletes the backup file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the backup file could not be found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RestoreBackup
(
    FILE* origFilePtr,                  ///< [IN] Pointer to the file that will be restored.
    const char* backupFileNamePtr       ///< [IN] Pointer to the file name of the backup file.
)
{
    // Open the backup file for reading.
    le_result_t result;
    FILE* backupFilePtr = le_flock_OpenStream(backupFileNamePtr, LE_FLOCK_READ, &result);

    if (backupFilePtr == NULL)
    {
        return result;
    }

    // Delete data that is currently in the file.
    if (SetFileLength(origFilePtr, 0) == LE_FAULT)
    {
        return LE_FAULT;
    }

    // Copy the backup file into the orig file.
    if (CopyFile(backupFilePtr, origFilePtr) == LE_FAULT)
    {
        le_flock_CloseStream(backupFilePtr);
        return LE_FAULT;
    }

    LE_INFO("Restored backup file '%s'.", backupFileNamePtr);

    le_flock_CloseStream(backupFilePtr);
    DeleteFile(backupFileNamePtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restores a backup file if it exists.  If the backup file exists then it is copied over the
 * original file and the backup is deleted.
 */
//--------------------------------------------------------------------------------------------------
static void RestoreBackupFile
(
    const char* origFileNamePtr,        ///< [IN] Pointer to the file name of the file that may need
                                        ///       to be restored.
    const char* backupFileNamePtr       ///< [IN] Pointer to the file name of the backup file.
)
{
    // Only restore backup if we have the write access to it.
    if (-1 == access(origFileNamePtr, W_OK))
    {
        return;
    }

    // Open the original file for writing.
    FILE* origFilePtr = le_flock_OpenStream(origFileNamePtr, LE_FLOCK_APPEND, NULL);
    LE_ASSERT(origFilePtr != NULL);

    LE_ASSERT(RestoreBackup(origFilePtr, backupFileNamePtr) != LE_FAULT);

    le_flock_CloseStream(origFilePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the user system.  This should be called before any other function in this API.
 */
//--------------------------------------------------------------------------------------------------
void user_Init
(
    void
)
{
    // Get the min and max values for local user IDs and group IDs.
    FILE *filePtr;

    filePtr = fopen(LOGIN_DEF_FILE, "r"); // Read mode

    if (filePtr != NULL)
    {
        bool gotMinUidValue = false;
        bool gotMaxUidValue = false;
        bool gotMinGidValue = false;
        bool gotMaxGidValue = false;

        char line[100];
        while ( (fgets(line, sizeof(line), filePtr)) != NULL )
        {
            if (UpdateLocalUidGidFromStr(line, UID_MIN_STR, (int*)(&MinLocalUid)))
            {
                gotMinUidValue = true;
            }
            else if (UpdateLocalUidGidFromStr(line, UID_MAX_STR, (int*)(&MaxLocalUid)))
            {
                gotMaxUidValue = true;
            }
            else if (UpdateLocalUidGidFromStr(line, GID_MAX_STR, (int*)(&MaxLocalGid)))
            {
                gotMaxGidValue = true;
            }
            else if (UpdateLocalUidGidFromStr(line, GID_MIN_STR, (int*)(&MinLocalGid)))
            {
                gotMinGidValue = true;
            }
        }

        // Close the file.
        int r;
        do
        {
            r = fclose(filePtr);
        } while ( (r != 0) && (errno == EINTR) );

        LE_FATAL_IF(r != 0, "Could not close open file.  %m.");

        // Check for errors.
        if (!gotMinUidValue)
        {
            LE_DEBUG("Could not read UID_MIN from '/etc/login.defs'.  Using default value.");
        }

        if (!gotMaxUidValue)
        {
            LE_DEBUG("Could not read UID_MAX from '/etc/login.defs'.  Using default value.");
        }

        if (!gotMinGidValue)
        {
            LE_DEBUG("Could not read GID_MIN from '/etc/login.defs'.  Using default value.");
        }

        if (!gotMaxGidValue)
        {
            LE_DEBUG("Could not read GID_MAX from '/etc/login.defs'.  Using default value.");
        }
    }
    else
    {
        LE_DEBUG("Could not read UID_MIN, UID_MAX, GID_MIN and GID_MAX from '/etc/login.defs'."
                 "  Using default values.");
    }

    // Get a suggestion on the size of the password entry buffer.
    size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buflen != -1)
    {
        MaxPasswdEntrySize = buflen;
    }

    // Get a suggestion on the size of the group entry buffer.
    buflen = sysconf(_SC_GETGR_R_SIZE_MAX);

    if (buflen == -1)
    {
        MaxGroupEntrySize = buflen;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Restores the passwd and/or group backup files if the backup files exist.  This function should be
 * called once on system startup.
 */
//--------------------------------------------------------------------------------------------------
void user_RestoreBackup
(
    void
)
{
    RestoreBackupFile(PASSWORD_FILE, BACKUP_PASSWORD_FILE);
    RestoreBackupFile(GROUP_FILE, BACKUP_GROUP_FILE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a user name from a user ID.
 *
 * @note Does not lock the passwd file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the user name was copied.
 *      LE_NOT_FOUND if the user was not found.
 *      LE_FAULT if there was an error getting the user name.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetName
(
    uid_t uid,                  ///< [IN] The uid of the user to get the name for.
    char* nameBufPtr,           ///< [OUT] The buffer to store the user name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the user name will be stored in.
)
{
    // Get the entry in the passwd file.
    char buf[MaxPasswdEntrySize];
    struct passwd pwd;
    struct passwd* resultPtr;
    int err;

    do
    {
        err = getpwuid_r(uid, &pwd, buf, sizeof(buf), &resultPtr);
    }
    while ((resultPtr == NULL) && (err == EINTR));

    if (resultPtr == NULL)
    {
        if (err == 0)
        {
            // The uid was not found.
            return LE_NOT_FOUND;
        }
        else
        {
            // There was an error.
            errno = err;
            LE_ERROR("Could not read the passwd entry for user id: %d.  %m", uid);
            return LE_FAULT;
        }
    }

    // Copy the username to the caller's buffer.
    return le_utf8_Copy(nameBufPtr, pwd.pw_name, nameBufSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a group name from a group ID.
 *
 * @note Does not lock the group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the group name was copied.
 *      LE_NOT_FOUND if the group was not found.
 *      LE_FAULT if there was an error getting the group name.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetGroupName
(
    gid_t gid,                  ///< [IN] The gid of the group to get the name for.
    char* nameBufPtr,           ///< [OUT] The buffer to store the group name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the group name will be stored in.
)
{
    // Get the entry in the group file.
    char buf[MaxGroupEntrySize];
    struct group grp;
    struct group* resultPtr;
    int err;

    do
    {
        err = getgrgid_r(gid, &grp, buf, sizeof(buf), &resultPtr);
    }
    while ((resultPtr == NULL) && (err == EINTR));

    if (resultPtr == NULL)
    {
        if (err == 0)
        {
            // The gid was not found.
            return LE_NOT_FOUND;
        }
        else
        {
            // There was an error.
            errno = err;
            LE_ERROR("Could not read the group entry for group id: %d.  %m", gid);
            return LE_FAULT;
        }
    }

    // Copy the group name to the caller's buffer.
    return le_utf8_Copy(nameBufPtr, grp.gr_name, nameBufSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID and group ID of a user and stores them in the locations pointed to by uidPtr and
 * gidPtr.  If there was an error then the values at uidPtr and gidPtr are undefined.
 *
 * @note Does not lock the passwd or group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the user does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetIDs
(
    const char* usernamePtr,    ///< [IN] The name of the user to get.
    uid_t* uidPtr,              ///< [OUT] A pointer to a location to store the uid for this user.
                                ///        This can be NULL if the uid is not needed.
    gid_t* gidPtr               ///< [OUT] A pointer to a location to store the gid for this user.
                                ///        This can be NULL if the gid is not needed.
)
{
    // Get the entry in the passwd file.
    char buf[MaxPasswdEntrySize];
    struct passwd pwd;
    struct passwd* resultPtr;
    int err;

    do
    {
        err = getpwnam_r(usernamePtr, &pwd, buf, sizeof(buf), &resultPtr);
    }
    while ((resultPtr == NULL) && (err == EINTR));

    if (resultPtr == NULL)
    {
        if (err == 0)
        {
            // The uid was not found.
            return LE_NOT_FOUND;
        }
        else
        {
            // There was an error.
            errno = err;
            LE_ERROR("Could not read the passwd entry for user '%s'.  %m", usernamePtr);
            return LE_FAULT;
        }
    }

    if (uidPtr != NULL)
    {
        *uidPtr = pwd.pw_uid;
    }

    if (gidPtr != NULL)
    {
        *gidPtr = pwd.pw_gid;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID for a user name.
 *
 * @note Does not lock the passwd file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the user does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetUid
(
    const char* usernamePtr,        ///< [IN] The name of the user to get.
    uid_t* uidPtr                   ///< [OUT] Pointer to store the uid of the user.
)
{
    gid_t gid;

    return GetIDs(usernamePtr, uidPtr, &gid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the group ID for a group name.
 *
 * @note Does not lock the group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the group does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetGid
(
    const char* groupNamePtr,       ///< [IN] The name of the group to get.
    gid_t* gidPtr                   ///< [OUT] Pointer to store the gid of the group.
)
{
    // Get the entry in the group file.
    char buf[MaxGroupEntrySize];
    struct group grp;
    struct group* resultPtr;
    int err;

    do
    {
        err = getgrnam_r(groupNamePtr, &grp, buf, sizeof(buf), &resultPtr);
    }
    while ((resultPtr == NULL) && (err == EINTR));

    if (resultPtr == NULL)
    {
        if (err == 0)
        {
            // The gid was not found.
            return LE_NOT_FOUND;
        }
        else
        {
            // There was an error.
            errno = err;
            LE_ERROR("Could not read the group entry for group '%s'.  %m", groupNamePtr);
            return LE_FAULT;
        }
    }

    *gidPtr = grp.gr_gid;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if a user or group with the specified name already exits.
 *
 * @note Does not lock the group file.
 *
 * @return
 *      LE_NOT_FOUND if neither a user or group has name.
 *      LE_DUPLICATE if the name alreadly exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckIfUserOrGroupExist
(
    const char* namePtr
)
{
    // Check if the user already exists.
    uid_t uid;

    le_result_t result = GetUid(namePtr, &uid);

    if (result == LE_OK)
    {
        LE_DEBUG("User '%s' already exists.", namePtr);
        return LE_DUPLICATE;
    }
    else if (result == LE_FAULT)
    {
        return LE_FAULT;
    }

    // Check if the group name (same as the user name) already exists.
    gid_t gid;

    result = GetGid(namePtr, &gid);

    if (result == LE_OK)
    {
        LE_DEBUG("Group '%s' already exists.", namePtr);
        return LE_DUPLICATE;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first available user ID.
 *
 * @note Does not lock the passwd file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more available IDs.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAvailUid
(
    uid_t* uidPtr                   ///< [OUT] Pointer to store the uid.
)
{
    // Get the first available uid.
    char dummy[1];
    uid_t uid = MinLocalUid;

    for (; uid <= MaxLocalUid; uid++)
    {
        le_result_t r = GetName(uid, dummy, sizeof(dummy));

        if (r == LE_NOT_FOUND)
        {
            // This uid is available.
            *uidPtr = uid;
            return LE_OK;
        }
        else if (r == LE_FAULT)
        {
            return LE_FAULT;
        }
    }

    LE_CRIT("There are too many users in the system.  No more users can be created.");
    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first available group ID.
 *
 * @note Does not lock the group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more available IDs.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAvailGid
(
    gid_t* gidPtr                   ///< [OUT] Pointer to store the gid.
)
{
    // Get the first available uid.
    char dummy[1];
    gid_t gid = MinLocalGid;

    for (; gid <= MaxLocalGid; gid++)
    {
        le_result_t r = GetGroupName(gid, dummy, sizeof(dummy));

        if (r == LE_NOT_FOUND)
        {
            // This gid is available.
            *gidPtr = gid;
            return LE_OK;
        }
        else if (r == LE_FAULT)
        {
            return LE_FAULT;
        }
    }

    LE_CRIT("There are too many groups in the system.  No more groups can be created.");
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first available user ID and group ID.
 *
 * @note Does not lock the passwd or group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more available IDs either for the user or the group.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAvailIDs
(
    uid_t* uidPtr,          ///< [OUT] Pointer to location to store first available uid.
    gid_t* gidPtr           ///< [OUT] Pointer to location to store first available gid.
)
{
    uid_t uid;

    le_result_t result = GetAvailUid(&uid);

    if (result != LE_OK)
    {
        return result;
    }

    gid_t gid;

    result = GetAvailGid(&gid);

    if (result == LE_OK)
    {
        *uidPtr = uid;
        *gidPtr = gid;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a group with the specified name and group ID.
 *
 * @note Does not lock the passwd or group files.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateGroup
(
    const char* namePtr,    ///< [IN] Pointer to the name of group to create.
    gid_t gid,              ///< [IN] The group ID to use.
    FILE* groupFilePtr      ///< [IN] Pointer to the group file.
)
{
    // Create a backup file for the group file.
    if (MakeBackup(groupFilePtr, BACKUP_GROUP_FILE) != LE_OK)
    {
        return LE_FAULT;
    }

    // Create the group entry for this user.
    struct group groupEntry = {.gr_name = (char*)namePtr,
                               .gr_passwd = "*",     // No password.
                               .gr_gid = gid,
                               .gr_mem = NULL};      // No group members.

    if (putgrent(&groupEntry, groupFilePtr) != 0)
    {
        LE_ERROR("Could not write to group file.  %m.");
        DeleteFile(BACKUP_GROUP_FILE);

        return LE_FAULT;
    }

    // Flush all data to the file system.
    if (FlushFile(groupFilePtr) != LE_OK)
    {
        LE_ERROR("Could flush group file.  %m.");
        DeleteFile(BACKUP_GROUP_FILE);

        return LE_FAULT;
    }

    sync();

    // Delete the backup file.
    DeleteFile(BACKUP_GROUP_FILE);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a user and group with the same name.  The created group will be the primary group of the
 * created user.
 *
 * @note Does not lock the passwd or group files.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateUserAndGroup
(
    const char* namePtr,    ///< [IN] Pointer to the name of user and group to create.
    uid_t uid,              ///< [IN] The uid for the user.
    gid_t gid,              ///< [IN] The gid for the group.
    FILE* passwdFilePtr,    ///< [IN] Pointer to the passwd file.
    FILE* groupFilePtr      ///< [IN] Pointer to the group file.
)
{
    // Generate home directory path.
    char homeDir[LIMIT_MAX_PATH_BYTES];
    if ( snprintf(homeDir, sizeof(homeDir), "/home/%s", namePtr) >= sizeof(homeDir))
    {
        LE_ERROR("Home directory path too long for user '%s'. ", namePtr);
        return LE_FAULT;
    }

    // Create a backup file for the passwd file.
    if (MakeBackup(passwdFilePtr, BACKUP_PASSWORD_FILE) != LE_OK)
    {
        return LE_FAULT;
    }

    // Create the user entry.
    struct passwd passEntry = {.pw_name = (char*)namePtr,
                               .pw_passwd = "*",    // No password.
                               .pw_uid = uid,
                               .pw_gid = gid,
                               .pw_gecos = (char*)namePtr,
                               .pw_dir = (char*)homeDir,
                               .pw_shell = "/"};    // No shell.

    if (putpwent(&passEntry, passwdFilePtr) == -1)
    {
        DeleteFile(BACKUP_PASSWORD_FILE);
        LE_FATAL("Could not write to passwd file.  %m.");
    }

    if (CreateGroup(namePtr, gid, groupFilePtr) != LE_OK)
    {
        // Revert passwd file.
        if (RestoreBackup(passwdFilePtr, BACKUP_PASSWORD_FILE) != LE_OK)
        {
            LE_FATAL("Could not restore the passwd file.");
        }
        return LE_FAULT;
    }

    // Flush all passwd file stream to the file system.
    if (FlushFile(passwdFilePtr) != LE_OK)
    {
        // Revert passwd file.
        if (RestoreBackup(passwdFilePtr, BACKUP_PASSWORD_FILE) != LE_OK)
        {
            LE_FATAL("Could not restore the passwd file.");
        }

        return LE_FAULT;
    }

    sync();

    // Delete the backup file.
    DeleteFile(BACKUP_PASSWORD_FILE);

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Creates a user account with the specified name.  A group with the same name as the username will
 * also be created and the group will be set as the user's primary group.  If the user and group are
 * successfully created the user ID and group ID are stored at the location pointed to by uidPtr and
 * gidPtr respectively.  If the user account alread exists then LE_DUPLICATE will be returned and
 * the user account's user ID and group ID are stored at the location pointed to by uidPtr and
 * gidPtr respectively.  If there is an error then LE_FAULT will be returned and the values at
 * uidPtr and gidPtr are undefined.
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
    uid_t* uidPtr,              ///< [OUT] Pinter to a location to store the uid for the created
                                ///        user.  This can be NULL if the uid is not needed.
    gid_t* gidPtr               ///< [OUT] Pointer to a location to store the gid for the created
                                ///        user.  This can be NULL if the gid is not needed.
)
{
    // Lock the passwd file for reading and writing.
    FILE* passwdFilePtr = le_flock_OpenStream(PASSWORD_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (passwdFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    // Lock the group file for reading and writing.
    FILE* groupFilePtr = le_flock_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (groupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
        le_flock_CloseStream(passwdFilePtr);
        return LE_FAULT;
    }

    // Check if the user already exists.
    le_result_t result = CheckIfUserOrGroupExist(usernamePtr);
    if (result == LE_FAULT)
    {
        goto cleanup;
    }

    if (result == LE_DUPLICATE)
    {
        if (GetIDs(usernamePtr, uidPtr, gidPtr) != LE_OK)
        {
            result = LE_FAULT;
        }

        goto cleanup;
    }

    // Get the first available uid and gid.
    uid_t uid;
    gid_t gid;
    result = GetAvailIDs(&uid, &gid);
    if (result != LE_OK)
    {
        goto cleanup;
    }

    // Create the user and the group.
    result = CreateUserAndGroup(usernamePtr, uid, gid, passwdFilePtr, groupFilePtr);

    if (result == LE_OK)
    {
        LE_INFO("Created user '%s' with uid %d and gid %d.", usernamePtr, uid, gid);

        if (uidPtr != NULL)
        {
            *uidPtr = uid;
        }

        if (gidPtr != NULL)
        {
            *gidPtr = gid;
        }
    }

cleanup:
    le_flock_CloseStream(passwdFilePtr);
    le_flock_CloseStream(groupFilePtr);

    return result;
}


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
)
{
    // Lock the group file for reading and writing.
    FILE* groupFilePtr = le_flock_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (groupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
        return LE_FAULT;
    }

    // Check if the group name already exists.
    gid_t gid;
    le_result_t result = GetGid(groupNamePtr, &gid);

    if (result == LE_OK)
    {
        LE_WARN("Group '%s' already exists.", groupNamePtr);
        le_flock_CloseStream(groupFilePtr);

        *gidPtr = gid;
        return LE_DUPLICATE;
    }
    else if (result == LE_FAULT)
    {
        le_flock_CloseStream(groupFilePtr);
        return LE_FAULT;
    }

    // Get available gid.
    result = GetAvailGid(&gid);

    if (result != LE_OK)
    {
        le_flock_CloseStream(groupFilePtr);
        return LE_FAULT;
    }

    result = CreateGroup(groupNamePtr, gid, groupFilePtr);
    le_flock_CloseStream(groupFilePtr);

    if (result == LE_OK)
    {
        LE_INFO("Created group '%s' with gid %d.", groupNamePtr, gid);
        *gidPtr = gid;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a group.
 *
 * @note Does not lock the group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteGroup
(
    const char* namePtr,        ///< [IN] Pointer to the name of the user and group to delete.
    FILE* groupFilePtr          ///< [IN] Pointer to the group file.
)
{
    struct group groupEntry;
    char groupBuf[MaxGroupEntrySize];
    struct group *groupEntryPtr;

    // Create a backup file for the group file.
    if (MakeBackup(groupFilePtr, BACKUP_GROUP_FILE) != LE_OK)
    {
        return LE_FAULT;
    }

    // Open the back up file for reading.
    FILE* backupFilePtr = le_flock_OpenStream(BACKUP_GROUP_FILE, LE_FLOCK_READ, NULL);
    if (backupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", BACKUP_GROUP_FILE);

        DeleteFile(BACKUP_GROUP_FILE);
        return LE_FAULT;
    }

    // Delete data that is currently in the group file.
    if (SetFileLength(groupFilePtr, 0) == LE_FAULT)
    {
        goto cleanup;
    }

    // Read each entry in the backup file.
    int result;

    while ( (result = fgetgrent_r(backupFilePtr, &groupEntry, groupBuf, sizeof(groupBuf), &groupEntryPtr)) == 0)
    {
        if (strcmp(groupEntry.gr_name, namePtr) != 0)
        {
            // Write the entry into the group file.
            if (putgrent(&groupEntry, groupFilePtr) == -1)
            {
                LE_ERROR("Could not write into group file.  %m.");
                goto cleanup;
            }
        }
    }

    if (result == ERANGE)
    {
        LE_ERROR("Could not read group file buffer size (%zd) is too small.", sizeof(groupBuf));
        goto cleanup;
    }

    // Flush all data to the file system.
    if (FlushFile(groupFilePtr) != LE_OK)
    {
        goto cleanup;
    }

    sync();

    LE_INFO("Successfully deleted group '%s'.", namePtr);
    le_flock_CloseStream(backupFilePtr);
    DeleteFile(BACKUP_GROUP_FILE);

    return LE_OK;

cleanup:
    le_flock_CloseStream(backupFilePtr);

    if (RestoreBackup(groupFilePtr, BACKUP_GROUP_FILE) != LE_OK)
    {
        LE_FATAL("Could not restore the group file.");
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a user and their primary group of the same name.
 *
 * @note Does not lock the passwd or group file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteUserAndGroup
(
    const char* namePtr,        ///< [IN] Pointer to the name of the user and group to delete.
    FILE* passwdFilePtr,        ///< [IN] Pointer to the passwd file.
    FILE* groupFilePtr          ///< [IN] Pointer to the group file.
)
{
    struct passwd passwdEntry;
    char buf[MaxPasswdEntrySize];
    struct passwd *passwdEntryPtr;

    // Create a backup file for the passwd file.
    if (MakeBackup(passwdFilePtr, BACKUP_PASSWORD_FILE) != LE_OK)
    {
        return LE_FAULT;
    }

    // Open the back up file for reading.
    FILE* backupFilePtr = le_flock_OpenStream(BACKUP_PASSWORD_FILE, LE_FLOCK_READ, NULL);
    if (backupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", BACKUP_PASSWORD_FILE);

        DeleteFile(BACKUP_PASSWORD_FILE);
        return LE_FAULT;
    }

    // Delete data that is currently in the passwd file.
    if (SetFileLength(passwdFilePtr, 0) == LE_FAULT)
    {
        goto cleanup;
    }

    // Read each entry in the backup file.
    int result;
    while ( (result = fgetpwent_r(backupFilePtr, &passwdEntry, buf, sizeof(buf), &passwdEntryPtr)) == 0)
    {
        if (strcmp(passwdEntry.pw_name, namePtr) != 0)
        {
            // Write the entry into the passwd file.
            if (putpwent(&passwdEntry, passwdFilePtr) == -1)
            {
                LE_ERROR("Could not write into passwd file.  %m.");
                goto cleanup;
            }
        }
    }

    if (result == ERANGE)
    {
        LE_ERROR("Could not read passwd file buffer size (%zd) is too small.", sizeof(buf));
        goto cleanup;
    }

    if (DeleteGroup(namePtr, groupFilePtr) != LE_OK)
    {
        goto cleanup;
    }

    // Flush all data to the file system.
    if (FlushFile(passwdFilePtr) != LE_OK)
    {
        goto cleanup;
    }

    sync();

    LE_INFO("Successfully deleted user '%s'.", namePtr);
    le_flock_CloseStream(backupFilePtr);
    DeleteFile(BACKUP_PASSWORD_FILE);

    return LE_OK;

cleanup:
    le_flock_CloseStream(backupFilePtr);

    if (RestoreBackup(passwdFilePtr, BACKUP_PASSWORD_FILE) != LE_OK)
    {
        LE_FATAL("Could not restore the passwd file.");
    }

    return LE_FAULT;
}


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
)
{
    // Lock the passwd file for reading and writing.
    FILE* passwdFilePtr = le_flock_OpenStream(PASSWORD_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (passwdFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    // Lock the group file for reading and writing.
    FILE* groupFilePtr = le_flock_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (groupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
        le_flock_CloseStream(passwdFilePtr);
        return LE_FAULT;
    }

    // Check if the user already exists.
    le_result_t result = CheckIfUserOrGroupExist(usernamePtr);
    if (result != LE_DUPLICATE)
    {
        goto cleanup;
    }

    result = DeleteUserAndGroup(usernamePtr, passwdFilePtr, groupFilePtr);

cleanup:
    le_flock_CloseStream(passwdFilePtr);
    le_flock_CloseStream(groupFilePtr);

    return result;
}


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
)
{
    // Lock the group file for reading and writing.
    FILE* groupFilePtr = le_flock_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    if (groupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
        return LE_FAULT;
    }

    // Check if the group name already exists.
    gid_t gid;
    le_result_t result = GetGid(groupNamePtr, &gid);

    if (result != LE_OK)
    {
        le_flock_CloseStream(groupFilePtr);
        return result;
    }

    result = DeleteGroup(groupNamePtr, groupFilePtr);

    le_flock_CloseStream(groupFilePtr);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the user ID and group ID of a user and stores them in the locations pointed to by uidPtr and
 * gidPtr.  If there was an error then the values at uidPtr and gidPtr are undefined.
 *
 * @return
 *      LE_OK if the successful.
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
)
{
    // Lock the passwd file for reading.
    int fd = le_flock_Open(PASSWORD_FILE, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_ERROR("Could not read file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    le_result_t r = GetIDs(usernamePtr, uidPtr, gidPtr);

    // Release the lock on the passwd file.
    le_flock_Close(fd);

    return r;
}


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
)
{
    // Lock the passwd file for reading.
    int fd = le_flock_Open(PASSWORD_FILE, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_ERROR("Could not read file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    uid_t uid;

    le_result_t result = GetUid(usernamePtr, &uid);

    // Release the lock on the passwd file.
    le_flock_Close(fd);

    if (result == LE_OK)
    {
        *uidPtr = uid;
    }

    return result;
}


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
)
{
    // Lock the group file for reading.
    int fd = le_flock_Open(GROUP_FILE, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_ERROR("Could not read file %s.  %m.", GROUP_FILE);
        return LE_FAULT;
    }

    gid_t gid;

    le_result_t result = GetGid(groupNamePtr, &gid);

    // Release the lock on the group file.
    le_flock_Close(fd);

    if (result == LE_OK)
    {
        *gidPtr = gid;
    }

    return result;
}


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
)
{
    // Lock the passwd file for reading.
    int fd = le_flock_Open(PASSWORD_FILE, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_ERROR("Could not read file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    le_result_t r = GetName(uid, nameBufPtr, nameBufSize);

    // Release the lock on the passwd file.
    le_flock_Close(fd);

    return r;
}


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
)
{
    // Lock the group file for reading.
    int fd = le_flock_Open(GROUP_FILE, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_ERROR("Could not read file %s.  %m.", GROUP_FILE);
        return LE_FAULT;
    }

    le_result_t r = GetGroupName(gid, nameBufPtr, nameBufSize);

    // Release the lock on the group file.
    le_flock_Close(fd);

    return r;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's name for a user.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the name was copied.
 *      LE_NOT_FOUND if the user does not have an application or the user ID is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetAppName
(
    uid_t uid,                  ///< [IN] The uid of the user.
    char* nameBufPtr,           ///< [OUT] The buffer to store the app name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the app name will be stored in.
)
{
    char username[LIMIT_MAX_USER_NAME_BYTES];

    le_result_t result = user_GetName(uid, username, sizeof(username));

    if (result != LE_OK)
    {
        return result;
    }

    if (strstr(username, USERNAME_PREFIX) != username)
    {
        // This is not an app.
        return LE_NOT_FOUND;
    }

    return le_utf8_Copy(nameBufPtr, username + sizeof(USERNAME_PREFIX) - 1, nameBufSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Converts an application's name to a user name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the provided buffer is too small and only part of the name was copied.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_AppNameToUserName
(
    const char* appName,        ///< [IN] The application's name.
    char* nameBufPtr,           ///< [OUT] The buffer to store the user name in.
    size_t nameBufSize          ///< [IN] The size of the buffer that the user name will be stored in.
)
{
    if (snprintf(nameBufPtr, nameBufSize, "%s%s", USERNAME_PREFIX, appName) >= nameBufSize)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's an application's user ID.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the application does not exist.
 *      LE_OVERFLOW if the application name is too long.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetAppUid
(
    const char* appName,        ///< [IN] Name of the application to get the uid for.
    uid_t* uidPtr               ///< [OUT] UID of the application.
)
{
    char userName[LIMIT_MAX_APP_NAME_BYTES];

    if (user_AppNameToUserName(appName, userName, sizeof(userName)) == LE_OVERFLOW)
    {
        return LE_OVERFLOW;
    }

    return user_GetUid(userName, uidPtr);
}
