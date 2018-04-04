//--------------------------------------------------------------------------------------------------
/** @file user.c
 *
 * API for creating/deleting Linux users and groups.
 *
 * Users are created and deleted by modifying the /etc/passwd files and this is done by using the
 * atomic file access mechanism. This guarantees against corruption of passwd file on unclean reboot.
 * Atomic access API also ensures file locking while opening for read or write.  This ensures that
 * the file does not get corrupted due to multiple simultaneous access. Locking the file allows
 * this API to be thread safe. However, the file locking mechanism provided by the atomic access API
 * is advisory only which means that other threads may access the passwd file simultaneously if they
 * are not using this API. The file locking mechanism used here is blocking so a deadlock will occur
 * if an attempt is made to obtain a lock on a file that has already been locked in the same thread.
 *
 * Groups are created and deleted by modifying the /etc/group file.  File update and locking is
 * handled in the same way as the passwd file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "user.h"
#include "limit.h"
#include "file.h"
#include <pwd.h>
#include <grp.h>
#include <sys/file.h>
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
 * The base local user and group ID to use if /etc/passwd and /etc/group are not writable.
 */
//--------------------------------------------------------------------------------------------------
#define BASE_MIN_UID       1100
#define BASE_MIN_GID       1100

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
 * Username and group aliases used in case of /etc/passwd and /etc/group are not writable. These
 * generic names should be already populated into the /etc/passwd and /etc/group.
 */
//--------------------------------------------------------------------------------------------------
#define USERNAME_TABLE_PREFIX                           "appLegato"

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
 * Name of the apps translation table used internally to replace the write access to /etc/passwd and
 * /etc/group.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_TRANSLATION_FILE   "/legato/systems/current/config/appsTab.bin"

//--------------------------------------------------------------------------------------------------
/**
 * Number of apps potentially supported by the apps translation table: 00 to 79.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t NbAppsInTranslationTable = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Apps translation table: contains the correspondance between an apps name and the appLegatoNN user
 * and group reserved for it.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   char name[LIMIT_MAX_APP_NAME_BYTES];  ///< Application name
}
appTab_t;

//--------------------------------------------------------------------------------------------------
/**
 * Apps translation table: contains the correspondance between an apps name and the appLegatoNN user
 * and group reserved for it.
 * The index of the apps name is the uid/gid reserved for this apps + the BASE_MIN_UID/GID.
 * When an apps is freed, the name is just filled to 0 and so may be used again for a new apps.
 */
//--------------------------------------------------------------------------------------------------
static appTab_t* AppsTab;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for apps translation table block.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppsTabPool = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Boolean to verify if /etc is writable or not.
 */
//--------------------------------------------------------------------------------------------------
static bool IsEtcWritable = false;

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

    // Get the file descriptor for this stream.
    int fd = fileno(filePtr);

    if (-1 == fd)
    {
        LE_CRIT("Could not get the file descriptor for a stream");
        return LE_FAULT;
    }

    // Truncate the file to the desired length.
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
 * Create backup file. Copies the contents of the original file to backup file. If /etc is not
 * writable, return directly.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the original file could not be found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MakeBackup
(
    const char* origFileNamePtr,         ///< [IN] Pointer to the original file name.
    const char* backupFileNamePtr        ///< [IN] Pointer to the backup file name.
)
{
    // Discard if /etc is not writable
    if (!IsEtcWritable)
    {
        return LE_OK;
    }

    // Delete old obsolete backup file if exists.
    if (file_Exists(backupFileNamePtr))
    {
        DeleteFile(backupFileNamePtr);
    }

    struct stat fileStatus;
    int backupFd;

    // Open the original file for reading and create backup file for writing.

    if (stat(origFileNamePtr, &fileStatus) == 0)
    {
        // File permission mode in backup file should be same as original file, so set the
        // process umask to 0.
        mode_t old_mode = umask((mode_t)0);

        // Caution: Use atomic file operation, don't use regular file copy operation as it may lead
        //          to corrupted backup in case of sudden power loss.
        backupFd = le_atomFile_Create(backupFileNamePtr,
                                      LE_FLOCK_WRITE,
                                      LE_FLOCK_REPLACE_IF_EXIST,
                                      fileStatus.st_mode);
        umask(old_mode);

        if (backupFd < 0)
        {
            return backupFd;
        }
    }
    else
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        else
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", origFileNamePtr);
            return LE_FAULT;
        }
    }

    int origFd = le_flock_Open(origFileNamePtr, LE_FLOCK_READ);

    if (origFd < 0)
    {
        le_atomFile_Cancel(backupFd);
        return origFd;
    }

    ssize_t origFileSize = lseek(origFd, 0L, SEEK_END);

    if (origFileSize < 0)
    {
        LE_CRIT("Error in getting size of '%s'. %m", origFileNamePtr);
        le_atomFile_Cancel(backupFd);
        le_flock_Close(origFd);
        return LE_FAULT;
    }

    // Get the kernel to copy the data over.  It may or may not happen in one go, so keep trying
    // until the whole file has been written or we error out.
    ssize_t sizeWritten = 0;
    off_t fileOffset = 0;

    while (sizeWritten < origFileSize)
    {
        ssize_t nextWritten = sendfile(backupFd,
                                       origFd,
                                       &fileOffset,
                                       origFileSize - sizeWritten);

        if (nextWritten == -1)
        {
            LE_CRIT("Error while copying file '%s' from '%s'. (%m)",
                    backupFileNamePtr,
                    origFileNamePtr);
            le_flock_Close(origFd);
            le_atomFile_Cancel(backupFd);
            return LE_FAULT;
        }

        sizeWritten += nextWritten;
    }


    le_result_t result = le_atomFile_Close(backupFd);
    le_flock_Close(origFd);

    if (LE_OK == result)
    {
        LE_DEBUG("Backed up original file '%s' to '%s'.", origFileNamePtr, backupFileNamePtr);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restore original file from backup file and removes the backup file. If /etc is not writable,
 * return directly.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the original file could not be found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RestoreBackup
(
    const char* origFileNamePtr,        ///< [IN] Pointer to the file name will be restored.
    const char* backupFileNamePtr       ///< [IN] Pointer to the file name of the backup file.
)
{
    // Discard if /etc is not writable
    if (!IsEtcWritable)
    {
        return LE_OK;
    }

    // Lock the original file
    int fd = le_flock_Open(origFileNamePtr, LE_FLOCK_WRITE);

    if (fd < 0)
    {
        return LE_FAULT;
    }

    // Now rename the backup file to original file
    if (rename(backupFileNamePtr, origFileNamePtr))
    {
        LE_CRIT("Failed restore '%s' from '%s' (%m).", origFileNamePtr, backupFileNamePtr);
        le_flock_Close(fd);
        return LE_FAULT;
    }

    le_flock_Close(fd);

    return LE_OK;
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

    // Check if /etc is writable and register the result for further checks
    IsEtcWritable = (0 == access( PASSWORD_FILE, W_OK ) ? true : false);
    LE_INFO("/etc is %swritable", IsEtcWritable ? "" : "NOT ");

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
        LE_CRIT_IF(fclose(filePtr) != 0, "Could not close open file.  %m.");

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

    if (!IsEtcWritable)
    {
        filePtr = fopen(PASSWORD_FILE, "r");
        if (filePtr)
        {
            // Get the entry in the passwd file.
            char buf[MaxPasswdEntrySize];
            struct passwd pwd;
            struct passwd* pwdPtr;
            int err;
            size_t appPrefixLen = strlen(USERNAME_TABLE_PREFIX);

            do
            {

                err = fgetpwent_r(filePtr, &pwd, buf, sizeof(buf), &pwdPtr);
                if (pwdPtr &&
                    (0 == strncmp(pwdPtr->pw_name, USERNAME_TABLE_PREFIX, appPrefixLen)))
                {
                    NbAppsInTranslationTable++;
                }
            }
            while (pwdPtr || (EINTR == err));
            fclose(filePtr);
            LE_INFO("Found %u appLegato for app translation table.", NbAppsInTranslationTable);
        }

        // Allocate the pool for apps translation table
        AppsTabPool = le_mem_CreatePool("AppsTabPool",
                                        sizeof(appTab_t) * NbAppsInTranslationTable);
        le_mem_ExpandPool(AppsTabPool, 1);

        AppsTab = (appTab_t *)le_mem_ForceAlloc(AppsTabPool);
        memset(AppsTab, 0, sizeof(appTab_t) * NbAppsInTranslationTable);
    }
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

    // If /etc is not writable, look first into the apps translation table indexed by the uid.
    if ((!IsEtcWritable) && (uid >= BASE_MIN_UID) &&
        ((uid - BASE_MIN_UID) < NbAppsInTranslationTable))
    {
        uint32_t ids = uid - BASE_MIN_UID;

        // /etc is not writable so try first to read the apps translation tab if it exist.
        FILE *fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }

            // Check if the apps already exists in the apps translation table.
            if ('\0' != AppsTab[ids].name[0])
            {
                // Copy the username to the caller's buffer.
                return le_utf8_Copy(nameBufPtr, AppsTab[ids].name, nameBufSize, NULL);
            }
        }
    }

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

    // If /etc is not writable, look first into the apps translation table indexed by the gid.
    if ((!IsEtcWritable) && (gid >= BASE_MIN_GID) &&
        ((gid - BASE_MIN_GID) < NbAppsInTranslationTable))
    {
        uint32_t ids = gid - BASE_MIN_UID;

        // /etc is not writable so try first to read the apps translation tab if it exist.
        FILE *fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }

            // Check if the apps already exists in the apps translation table.
            if ('\0' != AppsTab[ids].name[0])
            {
                // Copy the username to the caller's buffer.
                return le_utf8_Copy(nameBufPtr, AppsTab[ids].name, nameBufSize, NULL);
            }
        }
    }

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
    uint32_t ids;
    char appsUserName[LIMIT_MAX_APP_NAME_BYTES] = "";

    if (!IsEtcWritable)
    {
        // /etc is not writable so try first to read the apps translation tab if it exist.
        FILE *fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }
        }

        for (ids = 0; ids < NbAppsInTranslationTable; ids++)
        {
            // Check if the apps already exists in the apps translation table.
            if (0 == strcmp(AppsTab[ids].name, usernamePtr))
            {
                // App is found in the translation table. Get the username by the index.
                snprintf(appsUserName, sizeof(appsUserName), USERNAME_TABLE_PREFIX "%02u", ids);
                // The user name to check in /etc/passwd is appsLegatoNN.
                usernamePtr = appsUserName;
                break;
            }
        }
    }

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
    uint32_t ids;
    char appsGroupName[LIMIT_MAX_APP_NAME_BYTES] = "";

    if (!IsEtcWritable)
    {
        // /etc is not writable so try first to read the apps translation tab if it exist.
        FILE *fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }
        }

        for (ids = 0; ids < NbAppsInTranslationTable; ids++)
        {
            // Check if the apps already exists in the apps translation table.
            if (0 == strcmp(AppsTab[ids].name, groupNamePtr))
            {
                // App is found in the translation table. Get the group name by the index.
                snprintf(appsGroupName, sizeof(appsGroupName), USERNAME_TABLE_PREFIX "%02u", ids);
                // The group name to check in /etc/group is appsLegatoNN.
                groupNamePtr = appsGroupName;
                break;
            }
        }
    }

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
    uid_t uid;

    if (!IsEtcWritable)
    {
        // /etc is not writable so try first to look into the apps translation tab.
        for (uid = 0; uid < NbAppsInTranslationTable; uid++)
        {
            if ('\0' == AppsTab[uid].name[0])
            {
                *uidPtr = BASE_MIN_UID + uid;
                return LE_OK;
            }
        }
    }
    else
    {
        for (uid = MinLocalUid; uid <= MaxLocalUid; uid++)
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
    gid_t gid;

    if (!IsEtcWritable)
    {
        // /etc is not writable so try first to look into the apps translation tab.
        for (gid = 0; gid < NbAppsInTranslationTable; gid++)
        {
            if ('\0' == AppsTab[gid].name[0])
            {
                *gidPtr = BASE_MIN_GID + gid;
                return LE_OK;
            }
        }
    }
    else
    {
        for (gid = MinLocalGid; gid <= MaxLocalGid; gid++)
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
    }

    LE_CRIT("There are too many groups in the system.  No more groups can be created.");
    return LE_NOT_FOUND;
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
    // Create the group entry for this user.
    struct group groupEntry = {.gr_name = (char*)namePtr,
                               .gr_passwd = "*",     // No password.
                               .gr_gid = gid,
                               .gr_mem = NULL};      // No group members.

    if (putgrent(&groupEntry, groupFilePtr) != 0)
    {
        LE_ERROR("Could not write to group file.  %m.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a user, and sets its primary group.
 *
 * @note Does not lock the passwd or group files.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateUser
(
    const char* namePtr,    ///< [IN] Pointer to the name of user and group to create.
    uid_t uid,              ///< [IN] The uid for the user.
    gid_t gid,              ///< [IN] The gid for the group.
    FILE* passwdFilePtr     ///< [IN] Pointer to the passwd file.
)
{
    // Generate home directory path.
    char homeDir[LIMIT_MAX_PATH_BYTES];
    if ( snprintf(homeDir, sizeof(homeDir), "/home/%s", namePtr) >= sizeof(homeDir))
    {
        LE_ERROR("Home directory path too long for user '%s'. ", namePtr);
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
        LE_ERROR("Could not write to passwd file.  %m.");
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a user account with the specified name.  A group with the same name as the username will
 * also be created and the group will be set as the user's primary group.  If the user and group are
 * successfully created the user ID and group ID are stored at the location pointed to by uidPtr and
 * gidPtr respectively.  If the user/group accounts already exist then LE_DUPLICATE will be
 * returned and the user account's user ID and group ID are stored at the location pointed to by
 * uidPtr and gidPtr respectively.  If there is an error then LE_FAULT will be returned and the
 * values at uidPtr and gidPtr are undefined.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the user or group already exists.
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
    // Consider this a duplicate if either group or user do not exist
    bool isDuplicate = true;

    // Create a backup file for the group file.
    if (IsEtcWritable && MakeBackup(GROUP_FILE, BACKUP_GROUP_FILE) != LE_OK)
    {
        return LE_FAULT;
    }

    char appsUserName[LIMIT_MAX_APP_NAME_BYTES] = "";

    if (!IsEtcWritable)
    {
        // /etc is not writable. Use the apps translation table instead /etc/passwd.
        uint32_t ids;
        uint32_t uidfree = (uint32_t)-1;
        FILE *fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }
        }

        for (ids = 0; ids < NbAppsInTranslationTable; ids++)
        {
            // Check if the app is already present into the apps translation table.
            if (0 == strcmp(AppsTab[ids].name, usernamePtr))
            {
                snprintf(appsUserName, sizeof(appsUserName), USERNAME_TABLE_PREFIX "%02u", ids);
                usernamePtr = appsUserName;
                break;
            }
            // Register the first index free for future usage
            if (('\0' == AppsTab[ids].name[0]) && ((uint32_t)-1 == uidfree) )
            {
                uidfree = ids;
            }
        }

        // No apps already found?
        if (NbAppsInTranslationTable == ids)
        {
            // Free entry in the apps translation table?
            if ((uint32_t)-1 == uidfree)
            {
                LE_ERROR("No entry free in apps translation table");
            }
            else
            {
                // Yes. Copy the apps username into the apps translation table.
                snprintf(appsUserName, sizeof(appsUserName), USERNAME_TABLE_PREFIX "%02u", uidfree);
                snprintf(AppsTab[uidfree].name, sizeof(appTab_t), "%s", usernamePtr);
                // Write the apps translation table into flash
                fd = fopen(APPS_TRANSLATION_FILE, "w");
                if (fd)
                {
                    size_t rc;
                    rc = fwrite(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
                    fclose(fd);
                    if (NbAppsInTranslationTable != rc)
                    {
                        LE_ERROR("Write of apps translation table failed (rc %zu != %u)",
                                 rc, NbAppsInTranslationTable);
                        return LE_FAULT;
                    }
                }
                usernamePtr = appsUserName;
            }
        }
    }

    FILE* passwdFilePtr;
    if (IsEtcWritable)
    {
        // Lock the passwd file for reading and writing.
        passwdFilePtr = le_atomFile_OpenStream(PASSWORD_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    }
    else
    {
        passwdFilePtr = fopen( PASSWORD_FILE, "r" );
    }
    if (passwdFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", PASSWORD_FILE);
        return LE_FAULT;
    }

    FILE* groupFilePtr;
    // Lock the group file for reading and writing.
    if (IsEtcWritable)
    {
        groupFilePtr = le_atomFile_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    }
    else
    {
        groupFilePtr = fopen( GROUP_FILE, "r" );
    }
    if (groupFilePtr == NULL)
    {
        LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(passwdFilePtr);
        }
        else
        {
            fclose(passwdFilePtr);
        }
        return LE_FAULT;
    }

    // Create group first, as we need the gid to create a user.
    // First check if the group exists.
    uid_t uid;
    gid_t gid;
    le_result_t result = GetGid(usernamePtr, &gid);
    switch (result)
    {
        case LE_OK:
            // Group already exists
            break;
        case LE_NOT_FOUND:
            // Group does not exist, create it.
            result = GetAvailGid(&gid);
            if (result != LE_OK)
            {
                goto cleanup;
            }

            result = CreateGroup(usernamePtr, gid, groupFilePtr);
            if (LE_OK != result)
            {
                goto cleanup;
            }

            isDuplicate = false;

            break;
        default:
            LE_CRIT("Error %d checking if group '%s' exists", result, usernamePtr);
            goto cleanup;
    }

    // Now check if the user already exists
    result = GetUid(usernamePtr, &uid);
    switch (result)
    {
        case LE_OK:
            // User already exists;
            break;
        case LE_NOT_FOUND:
            // User does not exist, create it.
            result = GetAvailUid(&uid);
            if (LE_OK != result)
            {
                goto cleanup;
            }

            result = CreateUser(usernamePtr, uid, gid, passwdFilePtr);
            if (LE_OK != result)
            {
                goto cleanup;
            }

            isDuplicate = false;

            break;

        default:
            // Error checking if user exists
            LE_CRIT("Error %d checking if user '%s' exists", result, usernamePtr);
            goto cleanup;
    }

    if (IsEtcWritable)
    {
        result = le_atomFile_CloseStream(groupFilePtr);
    }
    else
    {
        fclose(groupFilePtr);
        result = LE_OK;
    }

    if (result != LE_OK)
    {
        DeleteFile(BACKUP_GROUP_FILE);
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(passwdFilePtr);
        }
        else
        {
            fclose(passwdFilePtr);
        }
        return result;
    }

    if (IsEtcWritable)
    {
        result = le_atomFile_CloseStream(passwdFilePtr);
    }
    else
    {
        fclose(passwdFilePtr);
        result = LE_OK;
    }

    if (result != LE_OK)
    {
        // Restore group file. If restoration succeed, it will automatically delete the backup file.
        LE_CRIT_IF(RestoreBackup(GROUP_FILE, BACKUP_GROUP_FILE) != LE_OK,
                    "Can't restore group file from backup.");
        return result;
    }
    else
    {
        DeleteFile(BACKUP_GROUP_FILE);
        LE_INFO("Created user '%s' with uid %d and gid %d.", usernamePtr, uid, gid);

        if (uidPtr != NULL)
        {
            *uidPtr = uid;
        }

        if (gidPtr != NULL)
        {
            *gidPtr = gid;
        }

        result = (isDuplicate ? LE_DUPLICATE : LE_OK);
        return result;
    }

cleanup:
    DeleteFile(BACKUP_GROUP_FILE);
    if (IsEtcWritable)
    {
        le_atomFile_CancelStream(passwdFilePtr);
        le_atomFile_CancelStream(groupFilePtr);
    }
    else
    {
        fclose(passwdFilePtr);
        fclose(groupFilePtr);
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a group with the specified name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the group already exists.  If the group already exists the gid of the group
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
    FILE* groupFilePtr;

    if (IsEtcWritable)
    {
        groupFilePtr = le_atomFile_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_APPEND, NULL);
    }
    else
    {
        groupFilePtr = fopen(GROUP_FILE, "r");
    }

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
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
        else
        {
            fclose(groupFilePtr);
        }

        *gidPtr = gid;
        return LE_DUPLICATE;
    }
    else if (result == LE_FAULT)
    {
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
        else
        {
            fclose(groupFilePtr);
        }
        return LE_FAULT;
    }

    // Get available gid.
    result = GetAvailGid(&gid);

    if (result != LE_OK)
    {
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
        else
        {
            fclose(groupFilePtr);
        }
        return LE_FAULT;
    }

    result = CreateGroup(groupNamePtr, gid, groupFilePtr);
    if (!IsEtcWritable)
    {
        fclose(groupFilePtr);
    }

    if (result == LE_OK)
    {
        if (IsEtcWritable)
        {
            result = le_atomFile_CloseStream(groupFilePtr);
        }

        if (result == LE_OK)
        {
            LE_INFO("Created group '%s' with gid %d.", groupNamePtr, gid);
            *gidPtr = gid;
        }
    }
    else
    {
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
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

    rewind(groupFilePtr);

    long readPos = ftell(groupFilePtr);

    if (readPos == -1)
    {
        LE_ERROR("Failed to get current position of group file. %m");
        return LE_FAULT;
    }

    long writePos = readPos;

    // Read each entry in the backup file.
    int result;
    bool skippedEntry = false;

    while ((result = fgetgrent_r(groupFilePtr, &groupEntry, groupBuf, sizeof(groupBuf), &groupEntryPtr)) == 0)
    {
        if (strcmp(groupEntry.gr_name, namePtr) == 0)
        {
            skippedEntry = true;
        }
        else if (skippedEntry)
        {
            if ((readPos = ftell(groupFilePtr)) == -1)
            {
                LE_ERROR("Failed to position of group file. %m");
                return LE_FAULT;
            }

            if (fseek(groupFilePtr, writePos, SEEK_SET) == -1)
            {
                LE_ERROR("Can't set position in group file. %m");
                return LE_FAULT;
            }

            // Write the entry into the group file.
            if (putgrent(&groupEntry, groupFilePtr) == -1)
            {
                LE_ERROR("Could not write into group file.  %m.");
                return LE_FAULT;
            }

            if ((writePos = ftell(groupFilePtr)) == -1)
            {
                LE_ERROR("Failed to position of group file. %m");
                return LE_FAULT;
            }

            if (fseek(groupFilePtr, readPos, SEEK_SET) == -1)
            {
                LE_ERROR("Can't set position in group file. %m");
                return LE_FAULT;
            }
        }

        if (!skippedEntry)
        {
            // No matched entry found yet, so update write position to current position in file.
            if ((writePos = ftell(groupFilePtr)) == -1)
            {
                LE_ERROR("Failed to position of passwd file. %m");
                return LE_FAULT;
            }
        }

    }

    if (result == ERANGE)
    {
        LE_ERROR("Could not read group file buffer size (%zd) is too small.", sizeof(groupBuf));
        return LE_FAULT;
    }

    return SetFileLength(groupFilePtr, writePos);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a user.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteUser
(
    const char* namePtr,        ///< [IN] Pointer to the name of the user and group to delete.
    FILE* passwdFilePtr         ///< [IN] Pointer to the passwd file.
)
{
    if (IsEtcWritable)
    {
        struct passwd passwdEntry;
        char buf[MaxPasswdEntrySize];
        struct passwd *passwdEntryPtr;

        rewind(passwdFilePtr);

        long readPos = ftell(passwdFilePtr);

        if (readPos == -1)
        {
            LE_ERROR("Failed to get current position of group file. %m");
            return LE_FAULT;
        }

        long writePos = readPos;

        // Read each entry in the backup file.
        int result;
        bool skippedEntry = false;

        while ((result = fgetpwent_r(passwdFilePtr, &passwdEntry, buf, sizeof(buf), &passwdEntryPtr)) == 0)
        {

            if (strcmp(passwdEntry.pw_name, namePtr) == 0)
            {
                skippedEntry = true;

            }
            else if (skippedEntry)
            {
                if ((readPos = ftell(passwdFilePtr)) == -1)
                {
                    LE_ERROR("Failed to position of passwd file. %m");
                    return LE_FAULT;
                }

                if (fseek(passwdFilePtr, writePos, SEEK_SET) == -1)
                {
                    LE_ERROR("Can't set position in passwd file. %m");
                    return LE_FAULT;
                }

                // Write the entry into the passwd file.
                if (putpwent(&passwdEntry, passwdFilePtr) == -1)
                {
                    LE_ERROR("Could not write into passwd file.  %m.");
                    return LE_FAULT;
                }

                if ((writePos = ftell(passwdFilePtr)) == -1)
                {
                    LE_ERROR("Failed to position of passwd file. %m");
                    return LE_FAULT;
                }

                if (fseek(passwdFilePtr, readPos, SEEK_SET) == -1)
                {
                    LE_ERROR("Can't set position in passwd file. %m");
                    return LE_FAULT;
                }
            }

            if (!skippedEntry)
            {
                // No matched entry found yet, so update write position to current position in file.
                if ((writePos = ftell(passwdFilePtr)) == -1)
                {
                    LE_ERROR("Failed to position of passwd file. %m");
                    return LE_FAULT;
                }
            }
        }

        if (result == ERANGE)
        {
            LE_ERROR("Could not read passwd file buffer size (%zd) is too small.", sizeof(buf));
            return LE_FAULT;
        }

        result = SetFileLength(passwdFilePtr, writePos);

        if (result != LE_OK)
        {
            LE_ERROR("Could not update password file. %m");
            return LE_FAULT;
        }

        return result;
    }
    else
    {
        uint32_t uid;
        FILE* fd = fopen(APPS_TRANSLATION_FILE, "r");
        if (fd)
        {
            size_t rc;
            rc = fread(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
            fclose(fd);
            if (NbAppsInTranslationTable != rc)
            {
                LE_ERROR("Read of apps translation table failed (rc %zu != %u)",
                         rc, NbAppsInTranslationTable);
                return LE_FAULT;
            }
        }

        for (uid = 0; uid < NbAppsInTranslationTable; uid++)
        {
            if (0 == strcmp(AppsTab[uid].name, namePtr))
            {
                size_t rc;

                memset(AppsTab[uid].name, 0, sizeof(appTab_t));
                fd = fopen(APPS_TRANSLATION_FILE, "w");
                if (fd)
                {
                    rc = fwrite(AppsTab, sizeof(appTab_t), NbAppsInTranslationTable, fd);
                    fclose(fd);
                    if (NbAppsInTranslationTable != rc)
                    {
                        LE_ERROR("Write of apps translation table failed (rc %zu != %u)",
                                 rc, NbAppsInTranslationTable);
                        return LE_FAULT;
                    }
                }
                return LE_OK;
            }
        }
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
    const char* namePtr     ///< [IN] Pointer to the name of the user to delete.
)
{
    FILE* passwdFilePtr;
    FILE* groupFilePtr;

    if (IsEtcWritable)
    {
        // Create a backup file for the group file.
        if (MakeBackup(GROUP_FILE, BACKUP_GROUP_FILE) != LE_OK)
        {
            return LE_FAULT;
        }

        // Lock the passwd file for reading and writing.
        passwdFilePtr = le_atomFile_OpenStream(PASSWORD_FILE, LE_FLOCK_READ_AND_WRITE, NULL);
        if (NULL == passwdFilePtr)
        {
            LE_ERROR("Could not open file %s.  %m.", PASSWORD_FILE);
            return LE_FAULT;
        }

        // Lock the group file for reading and writing.
        groupFilePtr = le_atomFile_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_WRITE, NULL);
        if (NULL == groupFilePtr)
        {
            LE_ERROR("Could not open file %s.  %m.", GROUP_FILE);
            le_atomFile_CancelStream(passwdFilePtr);
            return LE_FAULT;
        }
    }
    else
    {
        passwdFilePtr = fopen(PASSWORD_FILE, "r");
        if (NULL == passwdFilePtr)
        {
            return LE_FAULT;
        }
        groupFilePtr = fopen(GROUP_FILE, "r");
        if (NULL == groupFilePtr)
        {
            fclose(passwdFilePtr);
            return LE_FAULT;
        }
    }

    uid_t uid;
    gid_t gid;
    bool isDeleted = false;

    le_result_t result = GetUid(namePtr, &uid);

    switch (result)
    {
        case LE_OK:
            // User exists.
            result = DeleteUser(namePtr, passwdFilePtr);

            if(LE_OK != result)
            {
                LE_CRIT("Error (%s) while deleting user '%s'", LE_RESULT_TXT(result), namePtr);
                goto cleanup;
            }
            isDeleted = true;
            break;

        case LE_NOT_FOUND:
            // Group user doesn't exist, Log a warning message.
            LE_WARN("User '%s' doesn't exist", namePtr);
            break;

        default:
            LE_CRIT("Error (%s) checking if user '%s' exists", LE_RESULT_TXT(result), namePtr);
            goto cleanup;
    }

    // Group name is same as username
    result = GetGid(namePtr, &gid);

    switch (result)
    {
        case LE_OK:
            // Group exists.
            result = DeleteGroup(namePtr, groupFilePtr);

            if(LE_OK != result)
            {
                LE_CRIT("Error (%s) while deleting group '%s'", LE_RESULT_TXT(result), namePtr);
                goto cleanup;
            }
            isDeleted = true;
            break;

        case LE_NOT_FOUND:
            // Group does not exist, Log a warning message.
            LE_WARN("Group '%s' doesn't exist", namePtr);
            break;

        default:
            LE_CRIT("Error (%s) checking if group '%s' exists", LE_RESULT_TXT(result), namePtr);
            goto cleanup;
    }

    if (IsEtcWritable)
    {
        result = le_atomFile_CloseStream(groupFilePtr);

        if (result != LE_OK)
        {
            DeleteFile(BACKUP_GROUP_FILE);
            le_atomFile_CancelStream(passwdFilePtr);
            return result;
        }

        result = le_atomFile_CloseStream(passwdFilePtr);

        if (result != LE_OK)
        {
            // Restore group file. If restoration succeed, it will automatically delete the backup file.
            LE_CRIT_IF(RestoreBackup(GROUP_FILE, BACKUP_GROUP_FILE) != LE_OK,
                        "Can't restore group file from backup.");
            return result;
        }
        else
        {
            DeleteFile(BACKUP_GROUP_FILE);

            if (isDeleted)
            {
                LE_INFO("Deleted user '%s'.", namePtr);
                return LE_OK;
            }
            else
            {
                return LE_NOT_FOUND;
            }
        }
    }
    else
    {
        fclose(groupFilePtr);
        fclose(passwdFilePtr);
        return (isDeleted ? LE_OK : LE_NOT_FOUND);
    }

cleanup:
    if (IsEtcWritable)
    {
        DeleteFile(BACKUP_GROUP_FILE);
        le_atomFile_CancelStream(passwdFilePtr);
        le_atomFile_CancelStream(groupFilePtr);
    }
    else
    {
        fclose(passwdFilePtr);
        fclose(groupFilePtr);
    }
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
    FILE* groupFilePtr;

    if (IsEtcWritable)
    {
        // Lock the group file for reading and writing.
        groupFilePtr = le_atomFile_OpenStream(GROUP_FILE, LE_FLOCK_READ_AND_WRITE, NULL);
    }
    else
    {
        groupFilePtr = fopen(GROUP_FILE, "r");
    }
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
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
        else
        {
            fclose(groupFilePtr);
        }
        return result;
    }

    result = DeleteGroup(groupNamePtr, groupFilePtr);

    if (result != LE_OK)
    {
        if (IsEtcWritable)
        {
            le_atomFile_CancelStream(groupFilePtr);
        }
        else
        {
            fclose(groupFilePtr);
        }

        return result;
    }

    if (IsEtcWritable)
    {
        result = le_atomFile_CloseStream(groupFilePtr);
    }
    else
    {
        fclose(groupFilePtr);
        result = LE_OK;
    }

    if (result == LE_OK)
    {
        LE_INFO("Successfully deleted group '%s'.", groupNamePtr);
    }
    else
    {
        LE_ERROR("Failed to delete group: '%s'", groupNamePtr);
    }

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


//--------------------------------------------------------------------------------------------------
/**
 * Get's an application's group ID.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the application does not exist.
 *      LE_OVERFLOW if the application name is too long.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t user_GetAppGid
(
    const char* appName,        ///< [IN] Name of the application to get the gid for.
    gid_t* gidPtr               ///< [OUT] GID of the application.
)
{
    char userName[LIMIT_MAX_APP_NAME_BYTES];

    if (user_AppNameToUserName(appName, userName, sizeof(userName)) == LE_OVERFLOW)
    {
        return LE_OVERFLOW;
    }

    return user_GetGid(userName, gidPtr);
}
