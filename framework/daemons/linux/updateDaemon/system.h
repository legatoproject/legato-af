//--------------------------------------------------------------------------------------------------
/**
 * @file system.h
 *
 * Functions exported by the Update Daemon's "system" module, which encapsulates detailed knowledge
 * of how to operate on systems.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_UPDATE_DAEMON_SYSTEM_H_INCLUDE_GUARD
#define LEGATO_UPDATE_DAEMON_SYSTEM_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to the directory that systems get unpacked into.
 */
//--------------------------------------------------------------------------------------------------
extern const char* system_UnpackPath;


//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to directory containing apps in current running system.
 **/
//--------------------------------------------------------------------------------------------------
extern const char* system_CurrentAppsDir;


//--------------------------------------------------------------------------------------------------
/**
 * Prepare the unpack directory for use (delete any old one and create a fresh empty one).
 */
//--------------------------------------------------------------------------------------------------
void system_PrepUnpackDir
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove the systems unpack directory.
 */
//--------------------------------------------------------------------------------------------------
void system_KillUnpackDir
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the current systems index.
 *
 * @return The index ID of the current system.
 */
//--------------------------------------------------------------------------------------------------
int system_Index
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the index for the previous system in the chain, using the current system as a starting point.
 *
 * @return The index to the system that's previous to the given system.  -1 is returned if the
 *         previous system was not found.
 */
//--------------------------------------------------------------------------------------------------
int system_GetPreviousSystemIndex
(
    int systemIndex  ///< [IN] Get the system that's older than this system.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the current system's version string (null-terminated).
 *
 * @return Length of the version string, in bytes, excluding the null terminator.
 *         0 if version string could not be read.
 */
//--------------------------------------------------------------------------------------------------
size_t system_Version
(
    char* versionPtr,   ///< [OUT] Buffer to write the version string too.
    size_t versionSize  ///< [IN]  Size of the buffer passed in.
);


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the given system exists.
 *
 * @return True if the system in question exists, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool system_Exists
(
    int systemIndex  ///< [IN] The system index to check.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the hash ID from a given system.
 *
 * @return
 *      - LE_OK if no problems are encountered.
 *      - LE_NOT_FOUND if the given index does not correspond to an available system.
 *      - LE_FORMAT_ERROR if there are problems reading the hash from the system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_GetSystemHash
(
    int systemIndex,                        ///< [IN]  The system to read the hash for.
    char hashBuffer[LIMIT_MD5_STR_BYTES]    ///< [OUT] Buffer to hold the system's hash ID.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a symlink to a given app's non-writeable files in a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_SymlinkApp
(
    const char* systemNamePtr,  ///< E.g., "current" or "unpack".
    const char* appMd5Ptr,
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a symlink to a given app's non-writeable files in a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_UnlinkApp
(
    const char* systemNamePtr,  ///< E.g., "current" or "unpack".
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Update a given app's writeable files in the "current" system to match what's in the app's install
 * directory (/legato/apps/<hash>).  Deletes from the current system files that are not in the app's
 * install directory.  Adds to the current system files from the apps' install directory that are
 * not in the current system.  Leaves alone files that are in both the current system and the app's
 * install directory.
 *
 * @warning Assumes the app identified by the hash is installed in /legato/apps/<hash>.
 **/
//--------------------------------------------------------------------------------------------------
void system_UpdateCurrentAppWriteableFiles
(
    const char* appMd5Ptr,
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a given app's files from the current running system.
 **/
//--------------------------------------------------------------------------------------------------
void system_RemoveApp
(
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Copy a given app's (app-specific) config file from the current running system to the system
 * unpack area.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t system_CopyAppConfig
(
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete a system update and move the system from unpack into current.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_FinishUpdate
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Take a snapshot of the current system.
 *
 * @return LE_OK if successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t system_Snapshot
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system as being modified.
 */
//--------------------------------------------------------------------------------------------------
void system_MarkModified
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the current system has been marked as modified.
 *
 * @return true if the system has been modified, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool system_IsModified
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if an application with a given name is used in the current running system.
 *
 * @return true if the app is in the system.
 */
//--------------------------------------------------------------------------------------------------
bool system_HasApp
(
    const char* appNamePtr  ///< [IN] Look for this app name.
);


//--------------------------------------------------------------------------------------------------
/**
 * Check all installed systems and see if the given application is installed in any of them.  The
 * search is done by application MD5 hash.
 *
 * @return true if there is any system with the given application installed, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool system_AppUsedInAnySystem
(
    const char* appHashPtr  ///< [IN] Look for this app hash.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete any apps that are not used by any systems (including the "unpack" system, if there is one).
 */
//--------------------------------------------------------------------------------------------------
void system_RemoveUnusedApps
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete any systems that are "bad" or older than the newest "good".
 */
//--------------------------------------------------------------------------------------------------
void system_RemoveUnneeded
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the file system path to the directory into which a given app's writeable files are
 * found inside a given system.
 **/
//--------------------------------------------------------------------------------------------------
void system_GetAppWriteableFilesDirPath
(
    char* pathBuffPtr,          ///< [out] Ptr to where the path will be put.
    size_t buffSize,            ///< Size of the buffer.
    const char* systemNamePtr,
    const char* appNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the labels of the unpacked system.
 **/
//--------------------------------------------------------------------------------------------------
void system_InitSmackLabels
(
    void
);


#endif // LEGATO_UPDATE_DAEMON_SYSTEM_H_INCLUDE_GUARD
