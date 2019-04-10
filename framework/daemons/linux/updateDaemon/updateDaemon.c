/** @file updateDaemon.c
 *
 * The Update Daemon is one of the "framework daemon" processes that is started by the Supervisor
 * outside of other apps.  It is a core part of the Legato app framework responsible for
 * software update.
 *
 * The Update Daemon has a single-threaded, event-driven internal design.  It is broken into the
 * following parts:
 *
 * - updateDaemon.c - COMPONENT_INIT and all API implementations.
 *
 * - updateUnpack.c - unpacks incoming update pack files and drives execution of the update.
 *
 * - updateExec.c - implements execution of the updates.
 *
 * @note The Update Daemon only supports a single update task at a time.  Requests to start
 *       updates will be rejected while an update is already in progress.
 *
 * At start up, the Update Daemon checks for new configuration settings that need to be imported
 * due to an unfinished system update.  It does this by looking for the files "users.cfg" and
 * "apps.cfg" in the directory in which configuration trees are stored.  If these files exist
 * they are imported into the system tree and deleted.
 *
 * If the current system is not "good", then a probation timer is started.  When that timer
 * expires, the current system is marked "good".  The le_updateCtrl API can be used to control
 * this from outside.
 *
 * While the system is in its probation period, update requests are rejected, and probation period
 * controls (marking "bad" or "good", and deferring the end of probation) are accepted.
 *
 * If the current system is "good", then update requests are honoured and fault reports are
 * ignored.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "user.h"
#include "pipeline.h"
#include "updateUnpack.h"
#include "instStat.h"
#include "app.h"
#include "system.h"
#include "sysStatus.h"
#include "sysPaths.h"
#include "supCtrl.h"
#include "updateCtrl.h"
#include "installer.h"
#include "properties.h"
#include "updateInfo.h"
#include "ima.h"
#include "file.h"
#include "smack.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of app config tree name.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CFGTREE_NAME_BYTES   LIMIT_MAX_USER_NAME_BYTES


//--------------------------------------------------------------------------------------------------
/**
 * State of the Update Daemon state machine.
 *
 *                +---------------------------------------+
 *                |                                       |
 *                |                                       V
 * IDLE ----> UNPACKING ----> SECURITY_CHECKING ----> APPLYING
 *   ^            |                   |                   |
 *   |            |                   |                   |
 *   +------------+                   |                   |
 *   |                                |                   |
 *   +--------------------------------+                   |
 *   |                                                    |
 *   +----------------------------------------------------+
 *
 * Transition from UNPACKING to APPLYING happens when security-unpack finishes before the unpacking
 * finishes.  This is common when update packs are unsigned (when security-unpack is not really
 * doing anything).
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    STATE_IDLE,         ///< No update is happening.  Check probation timer to see if in probation.
    STATE_UNPACKING,    ///< A system update is being unpacked.
    STATE_SECURITY_CHECKING, ///< A unpack finished, but security-unpack not finished yet.
    STATE_APPLYING,     ///< A system update is being applied.
}
State = STATE_IDLE;


//--------------------------------------------------------------------------------------------------
/**
 * Timer for the probation period.
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t ProbationTimer;


//--------------------------------------------------------------------------------------------------
/**
 * The IPC Session Reference for the IPC session that started the current update.
 * NULL if no update in progress.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t IpcSession = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to the security-unpack process pipeline, or NULL if the pipeline doesn't exist.
 **/
//--------------------------------------------------------------------------------------------------
pipeline_Ref_t SecurityUnpackPipeline = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Structure that can hold the details of a client's registered progress notification handler.
 *
 * These are allocated from the ClientProgressHandlerPool and kept in the ClientProgressHandlerList.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;   ///< Used to link into the ClientProgressHandlerList.
    le_update_ProgressHandlerFunc_t func;
    void* contextPtr;
    le_update_ProgressHandlerRef_t ref; ///< Reference to this thing that we gave to the client.
    le_msg_SessionRef_t sessionRef; ///< IPC session reference that this handler belongs to.
}
ClientProgressHandler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Counter used to create progress handler references.
 **/
//--------------------------------------------------------------------------------------------------
static size_t NextClientProgressHandlerRef;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which client progress handler objects are allocated.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientProgressHandlerPool;


//--------------------------------------------------------------------------------------------------
/**
 * List of client progress handlers.
 **/
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ClientProgressHandlerList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Event ID for triggering installation.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t InstallEventId;


//--------------------------------------------------------------------------------------------------
/**
 * Error code.
 */
//--------------------------------------------------------------------------------------------------
static le_update_ErrorCode_t ErrorCode = LE_UPDATE_ERR_NONE;


//--------------------------------------------------------------------------------------------------
/**
 * Flag to store when updateDaemon is ready to install new app/system. Only set to true
 * when updateDaemon send notification to client that download is successful.
 */
//--------------------------------------------------------------------------------------------------
bool InstallReady = false;


//--------------------------------------------------------------------------------------------------
/**
 * Legato R/O tree
 */
//--------------------------------------------------------------------------------------------------
static bool IsReadOnly = false;


//--------------------------------------------------------------------------------------------------
/**
 * Enter probation mode and kick off the probation timer.
 */
//--------------------------------------------------------------------------------------------------
static void StartProbation
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (le_timer_Start(ProbationTimer) == LE_BUSY)
    {
        le_timer_Restart(ProbationTimer);
    }

    LE_INFO("System on probation (timer started).");
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the update state to idle, mark the system good and clean up old system files.
 */
//--------------------------------------------------------------------------------------------------
void updateDaemon_MarkGood
(
    void
)
{
    LE_INFO("System passed probation. Marking 'good'.");

    // stop the probation timer - we may have been called from updateCtrl before expiry
    // This will return LE_FAULT if the timer is not currently running but that is OK
    le_timer_Stop(ProbationTimer);
    sysStatus_MarkGood();
    system_RemoveUnneeded();
    system_RemoveUnusedApps();
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer expiry function for the probation timer.  If this goes off, it means the system passed
 * probation and it's time to mark it "good".
 */
//--------------------------------------------------------------------------------------------------
static void HandleProbationExpiry
(
    le_timer_Ref_t timer
)
//--------------------------------------------------------------------------------------------------
{
    if (State != STATE_IDLE)
    {
        // An update is ongoing, so don't mark system good. Start the timer again
        StartProbation();
        return;
    }

    if (updateCtrl_IsProbationLocked() == false)
    {
        updateDaemon_MarkGood();
    }
    else
    {
        // Probation is Locked. Set callback for updateCtrl to call when proabtion is unlocked.
        updateCtrl_SetProbationExpiryCallBack(updateDaemon_MarkGood);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the supplied file descriptor.
 *
 * @return
 *      - true: Supplied file descriptor is valid.
 *      - false: Supplied file descriptor is invalid..
 */
//--------------------------------------------------------------------------------------------------
static bool IsValidFileDesc
(
    int fd                   ///<[IN] File descriptor to be validated.
)
//--------------------------------------------------------------------------------------------------
{
    return ((fd >= 0) && (fcntl(fd, F_GETFL) != -1));
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that runs in the security-unpack child process inside the pipeline.
 */
//--------------------------------------------------------------------------------------------------
static int SecurityUnpack
(
    void* param  // unused.
)
//--------------------------------------------------------------------------------------------------
{
    // Close all fds except stdin, stdout, stderr.
    fd_CloseAllNonStd();

    // Create a user account for the security-unpack tool (or don't if the account already exists).
    const char* userName = "SecurityUnpack";
    uid_t uid;
    gid_t gid;
    LE_FATAL_IF(user_Create(userName, &uid, &gid) == LE_FAULT, "Can't create user: %s", userName);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");

    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    // Set our process's user ID.  This sets all of our user IDs (real, effective, saved).  This
    // call also clears all capabilities.  This function in particular MUST be called after all
    // the previous system calls because once we make this call we will lose root priviledges.
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");

    // Execute the program.
    const char* exePath = "/legato/systems/current/bin/security-unpack";
    execl(exePath, exePath, (char *)NULL);
    LE_FATAL("Failed to exec '%s' (%m)", exePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether or not a file exists at a given file system path.
 *
 * @return true if the file exists and is a normal file.  false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool FileExists
(
    const char* filePath
)
//--------------------------------------------------------------------------------------------------
{
    struct stat fileStatus;

    // Use stat(2) to check for existence of the file.
    if (stat(filePath, &fileStatus) != 0)
    {
        // ENOENT = the file doesn't exist.  Anything else warrants and error report.
        if (errno != ENOENT)
        {
            LE_CRIT("Error when trying to stat '%s'. (%m)", filePath);
        }

        return false;
    }
    else
    {
        // Something exists. Make sure it's a file.
        // NOTE: stat() follows symlinks.
        if (S_ISREG(fileStatus.st_mode))
        {
            return true;
        }
        else
        {
            LE_CRIT("Unexpected file system object type (%#o) at path '%s'.",
                    fileStatus.st_mode & S_IFMT,
                    filePath);

            return false;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Call all the registered status handler functions for the IPC client whose update is currently
 * in progress.
 **/
//--------------------------------------------------------------------------------------------------
static void CallStatusHandlers
(
    le_update_State_t apiState,
    unsigned int percentDone        ///< The percentage this state is complete.
)
//--------------------------------------------------------------------------------------------------
{
    // Call the client's progress handler(s) for this session.
    le_dls_Link_t* linkPtr;
    for (linkPtr = le_dls_Peek(&ClientProgressHandlerList);
         linkPtr != NULL;
         linkPtr = le_dls_PeekNext(&ClientProgressHandlerList, linkPtr))
    {
        ClientProgressHandler_t* handlerPtr = CONTAINER_OF(linkPtr, ClientProgressHandler_t, link);

        if (handlerPtr->sessionRef == IpcSession)
        {
            handlerPtr->func(apiState, percentDone, handlerPtr->contextPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * If the update failed, go back to IDLE and report the failure to the client.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateFailed
(
    le_update_ErrorCode_t errCode
)
//--------------------------------------------------------------------------------------------------
{
    // Should notify client only once if it is at failed state.
    if (ErrorCode == LE_UPDATE_ERR_NONE)
    {
        CallStatusHandlers(LE_UPDATE_STATE_FAILED, 0);
        LE_ERROR("Update failed!!");
    }

    if (errCode != LE_UPDATE_ERR_NONE)
    {
        ErrorCode = errCode;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Report to the client that update is done.
 **/
//--------------------------------------------------------------------------------------------------
static void ReportUpdateDone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    CallStatusHandlers(LE_UPDATE_STATE_APPLYING, 100);
    CallStatusHandlers(LE_UPDATE_STATE_SUCCESS, 100);
}


//--------------------------------------------------------------------------------------------------
/**
 * Install system applications from unpack directory.
 *
 * @return
 *      - LE_OK if successful
 *      - LE_FAULT if failed
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t InstallSystemApps
(
    void
)
//--------------------------------------------------------------------------------------------------
{

    char* pathArrayPtr[] = {(char *)app_UnpackPath,
                            NULL};

    // Open the directory tree to search. We just need to traverse top level directory, so no need
    // of stat information.
    FTS* ftsPtr = fts_open(pathArrayPtr,
                           FTS_LOGICAL|FTS_NOSTAT,
                           NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'.  %m.",
                pathArrayPtr[0]);

    // Traverse through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (entPtr->fts_level == 1)
                {
                    char appPropertyPath[LIMIT_MAX_PATH_BYTES];

                    LE_ASSERT(snprintf(appPropertyPath,
                                       sizeof(appPropertyPath),
                                       "%s/info.properties",
                                       entPtr->fts_path)
                                       < sizeof(appPropertyPath));

                    char appMd5Hash[LIMIT_MD5_STR_BYTES];
                    char appName[LIMIT_MAX_APP_NAME_BYTES];
                    le_result_t result = properties_GetValueForKey(appPropertyPath,
                                                                   "app.md5",
                                                                   appMd5Hash,
                                                                   sizeof(appMd5Hash));
                    if (result != LE_OK)
                    {
                        fts_close(ftsPtr);
                        LE_CRIT("Failed to get 'app.md5' from '%s'", appPropertyPath);
                        return LE_FAULT;
                    }

                    result = properties_GetValueForKey(appPropertyPath,
                                                       "app.name",
                                                       appName,
                                                       sizeof(appName));
                    if (result != LE_OK)
                    {
                        fts_close(ftsPtr);
                        LE_CRIT("Failed to get 'app.name' from '%s'", appPropertyPath);
                        return LE_FAULT;
                    }


                    char appPath[LIMIT_MAX_PATH_BYTES];

                    LE_ASSERT(snprintf(appPath,
                                       sizeof(appPath),
                                       "/legato/apps/%s",
                                       appMd5Hash)
                                       < sizeof(appPath));

                    (void)unlink(appPath);

                    LE_DEBUG("Renaming '%s' to '%s'",
                             entPtr->fts_path,
                             appPath);

                    if (rename(entPtr->fts_path, appPath) != 0)
                    {
                        fts_close(ftsPtr);
                        LE_CRIT("Failed to rename '%s' to '%s', %m.",
                                app_UnpackPath,
                                appPath);
                        return LE_FAULT;
                    }

                    smack_SetLabel(appPath, "framework");

                    // Now setup the smack permission.
                    if (app_SetSmackPermReadOnly(appMd5Hash, appName) != LE_OK)
                    {
                        fts_close(ftsPtr);
                        LE_CRIT("Failed to setup smack permission for app '%s<%s>'",
                                appName,
                                appMd5Hash);
                        return LE_FAULT;
                    }
                    // We don't need to go into this directory.
                    fts_set(ftsPtr, entPtr, FTS_SKIP);
                }
                break;

            case FTS_DP:
                // Same directory in post order, ignore it.
                break;

            default:
                if (entPtr->fts_level != 0)
                {
                    LE_ERROR("Unexpected file type %d at '%s'",
                             entPtr->fts_info,
                             entPtr->fts_path);
                }
                break;
        }

    }

    fts_close(ftsPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Setup writable files for all system applications.
 *
 * @return
 *      - LE_OK if successful
 *      - LE_FAULT if failed
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t SetupSystemAppsWritable
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char sysUnpackAppPath[LIMIT_MAX_PATH_BYTES];
    LE_ASSERT(snprintf(sysUnpackAppPath,
                       sizeof(sysUnpackAppPath),
                       "%s/apps",
                       system_UnpackPath)
                       < sizeof(sysUnpackAppPath));

    char* pathArrayPtr[] = {(char *)sysUnpackAppPath,
                             NULL};

    // We need to get the app names and their md5 hash
    FTS* ftsPtr = fts_open(pathArrayPtr,
                           FTS_PHYSICAL,
                           NULL);

    LE_FATAL_IF(ftsPtr == NULL, "Could not access dir '%s'. %m.", pathArrayPtr[0]);

    // Traverse through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        if (entPtr->fts_info == FTS_SL)
        {
            if (entPtr->fts_level == 1)
            {

                char appMd5Buf[LIMIT_MD5_STR_BYTES];

                //Here path is constructed as /legato/system/apps/<AppName point to soft link>>.
                const char* appName = le_path_GetBasenamePtr(entPtr->fts_path, "/");

                installer_GetAppHashFromSymlink(entPtr->fts_path, appMd5Buf);

                LE_DEBUG("Path '%s' AppName '%s', MD5 '%s'", entPtr->fts_path, appName, appMd5Buf);

                // If the app is in "Preloaded Any Version" mode, then the application directory
                // must be inherited from the previous system.
                if (0 == strncmp(appMd5Buf, PRELOADED_ANY_VERSION, sizeof(appMd5Buf)))
                {
                    bool isFound = false;
                    char linkPath[PATH_MAX];
                    char installedAppPath[PATH_MAX];

                    LE_ASSERT(snprintf(linkPath,
                                       sizeof(linkPath),
                                       "/legato/systems/current/apps/%s",
                                       appName)
                                       < sizeof(linkPath));

                    // Read the content of the symlink pointing to app directory
                    ssize_t bytesRead = readlink(linkPath,
                                                 installedAppPath,
                                                 sizeof(installedAppPath) - 1);
                    if (bytesRead < 0)
                    {
                        LE_ERROR("Error resolving symlink %s", linkPath);
                    }
                    else if (bytesRead >= sizeof(installedAppPath))
                    {
                        LE_ERROR("Contents of symlink %s too long (> %zu).",
                                 linkPath, sizeof(installedAppPath) - 1);
                    }
                    else
                    {
                        // Null-terminate the string.
                        installedAppPath[bytesRead] = '\0';
                        LE_INFO("Preloaded app %s: found link %s", appName, installedAppPath);

                        // Establish the symlink
                        system_SymlinkApp("unpack",
                                          le_path_GetBasenamePtr(installedAppPath, "/"),
                                          appName);
                        isFound = true;
                    }
                    if (!isFound)
                    {
                        LE_CRIT("Preloaded app %s not found!", appName);
                    }
                }

                // Set up the app's writeable files in the new system (copying from install dir and/or
                // current system).
                if (app_SetUpAppWriteables(appMd5Buf, appName) != LE_OK)
                {
                    if (ftsPtr)
                    {
                       fts_close(ftsPtr);
                    }

                    LE_CRIT("Failed to setup writable for app '%s<%s>'",
                            appName,
                            appMd5Buf);
                    return LE_FAULT;
                }

                // We don't need to go into this directory.
                fts_set(ftsPtr, entPtr, FTS_SKIP);
            }
        }
        else if (entPtr->fts_level != 0)
        {
            LE_ERROR("Unexpected file type %d at '%s'", entPtr->fts_info, entPtr->fts_path);
        }

    }

    if (ftsPtr)
    {
       fts_close(ftsPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively traverse the directory and verify each file IMA signature against the public
 * certificate.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if no app unpack directory exists
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t  VerifyAppUnpackDir
(
    void
)
{
    char path[LIMIT_MAX_PATH_BYTES] = "";
    snprintf(path, sizeof(path), "%s/%s", app_UnpackPath, PUB_CERT_NAME);

    if (!le_dir_IsDir(app_UnpackPath))
    {
        // No unpack directory exists, this means we tried to install the same app again.
        LE_INFO("'%s' does not exists", app_UnpackPath);
        return LE_NOT_FOUND;
    }

    if (!file_Exists(path))
    {
        LE_CRIT("Bad public certificate path '%s'", path);
        return LE_FAULT;
    }

    if (LE_OK != supCtrl_ImportImaCert(path))
    {
        LE_CRIT("Failed to import public certificate '%s'", path);
        return LE_FAULT;
    }

    return ima_VerifyDir(app_UnpackPath, path);
}



//--------------------------------------------------------------------------------------------------
/**
 * Recursively traverse the directory and verify each file IMA signature against the public
 * certificate.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t  VerifyUnpackedSystem
(
    void
)
{
    char path[LIMIT_MAX_PATH_BYTES] = "";
    snprintf(path, sizeof(path), "%s/%s", system_UnpackPath, PUB_CERT_NAME);

    if (!file_Exists(path))
    {
        LE_CRIT("Bad public certificate path '%s'", path);
        return LE_FAULT;
    }

    // UpdateDaemon doesn't have privilege to import certificate to linux keyring. Ask supervisor
    // to import certificate in keyring.
    LE_DEBUG("Import certificate '%s'", path);
    le_result_t result = supCtrl_ImportImaCert(path);

    if (LE_OK != result)
    {
        LE_CRIT("Failed to import public certificate '%s'", path);
        return LE_FAULT;
    }

    LE_DEBUG("Verify dir: '%s' with certificate '%s'", system_UnpackPath, path);
    result = ima_VerifyDir(system_UnpackPath, path);

    if (LE_OK != result)
    {
        LE_CRIT("Failed to verify files  '%s' directory", system_UnpackPath);
        return LE_FAULT;
    }

    // Now traverse the system app unpack directory and verify each app files
    char* pathArrayPtr[] = {(char *)app_UnpackPath,
                                NULL};

    // Open the directory tree to traverse.
    FTS* ftsPtr = fts_open(pathArrayPtr,
                           FTS_PHYSICAL,
                           NULL);

    if (NULL == ftsPtr)
    {
        LE_ERROR("Could not access dir '%s'.  %m.", pathArrayPtr[0]);
        return LE_FAULT;
    }

    // Traverse through the directory tree.
    FTSENT* entPtr;
    char appPubCertPath[LIMIT_MAX_PATH_BYTES] = "";

    while (NULL != (entPtr = fts_read(ftsPtr)))
    {
        LE_DEBUG("Filename: %s, filePath: %s, rootPath: %s, fts_info: %d", entPtr->fts_name,
                                entPtr->fts_accpath,
                                entPtr->fts_path,
                                entPtr->fts_info);
        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (1 == entPtr->fts_level)
                {
                    snprintf(appPubCertPath,
                             sizeof(appPubCertPath),
                             "%s/%s",
                             entPtr->fts_path, PUB_CERT_NAME );

                    if (file_Exists(appPubCertPath))
                    {
                        result = supCtrl_ImportImaCert(appPubCertPath);

                        if (LE_OK != result)
                        {
                            LE_CRIT("Failed to import public certificate '%s'", appPubCertPath);
                            fts_close(ftsPtr);
                            return LE_FAULT;
                        }
                    }
                    else
                    {
                        memset(appPubCertPath, 0, sizeof(appPubCertPath));
                    }
                }
                break;

            case FTS_SL:
            case FTS_SLNONE:
                break;

            case FTS_F:
                // As directories are visiting preorder, verifying files using last received
                // certificate should be ok.
                if (file_Exists(appPubCertPath))
                {
                    result = ima_VerifyFile(entPtr->fts_accpath, appPubCertPath);
                }
                else
                {
                    result = ima_VerifyFile(entPtr->fts_accpath, path);
                }

                if (LE_OK != result)
                {
                    LE_CRIT("Failed to verify file '%s' with public certificate '%s'",
                            entPtr->fts_accpath,
                            file_Exists(appPubCertPath) ? appPubCertPath : path);
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
                break;
        }

    }

    fts_close(ftsPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply a system update.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplySystemUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (ima_IsEnabled())
    {
        if (LE_OK != VerifyUnpackedSystem())
        {
            LE_CRIT("Failed to unpacked system");
            UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
            return;
        }
    }

    if (LE_OK != InstallSystemApps())
    {
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
        return;
    }

    if (LE_OK != SetupSystemAppsWritable())
    {
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
        return;
    }

    if (LE_OK == system_FinishUpdate())
    {
        ReportUpdateDone();
        // Just ask the Supervisor to restart Legato.
        supCtrl_RestartLegato();
    }
    else
    {
        // Notify error
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply an application update.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyAppUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const char* appName = updateUnpack_GetAppName();
    const char* md5 = updateUnpack_GetAppMd5();
    le_result_t result = LE_OK;

    if (ima_IsEnabled())
    {
        result = VerifyAppUnpackDir();

        if (LE_FAULT == result)
        {
            LE_CRIT("Failed to install app '%s<%s>'.", appName, md5);
            UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
            return;
        }

    }

    // Install the app in the current running system.
    result = app_InstallIndividual(md5, appName);

    if (LE_OK == result)
    {
       LE_INFO("App '%s<%s>' installed properly.", appName, md5);
       ReportUpdateDone();
       // App is installed, now start probation
       StartProbation();
    }
    else if (LE_DUPLICATE == result)
    {
        LE_INFO("App '%s<%s>' already installed. Discarded app installation.", appName, md5);
        ReportUpdateDone();
    }
    else
    {
        LE_CRIT("Failed to install app '%s<%s>'.", appName, md5);
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Do an application remove.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyAppRemove
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    const char* appName = updateUnpack_GetAppName();
    // Install the app in the current running system.
    le_result_t result = app_RemoveIndividual(appName);

    if (result == LE_OK)
    {
        LE_INFO("App '%s' removed properly.", appName);
        ReportUpdateDone();
        // App is installed, now start probation
        StartProbation();
    }
    else if (result == LE_NOT_FOUND)
    {
        LE_ERROR("App '%s' was not found in the system.", appName);
        ReportUpdateDone();
    }
    else
    {
        LE_CRIT("Failed to remove app '%s'.", appName);
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Apply a firmware update.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyFwUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result = LE_OK;

    LE_INFO("Applying Firmware update");

    if (le_fwupdate_TryConnectService() != LE_OK)
    {
        LE_ERROR("Unable to connect to fwupdate service.");
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
        return;
    }

    // This function returns only if there was an error.
    result = le_fwupdate_Install();
    if (result != LE_OK)
    {
        LE_ERROR("Firmware update install failed: result %d", result);
        UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
    }
    le_fwupdate_DisconnectService();
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply an unpacked update that has passed security check.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplyUpdate
(
    void *unused
)
//--------------------------------------------------------------------------------------------------
{
    State = STATE_APPLYING;
    CallStatusHandlers(LE_UPDATE_STATE_APPLYING, 0);
    switch (updateUnpack_GetType())
    {
        case TYPE_SYSTEM_UPDATE:
            ApplySystemUpdate();
            break;

        case TYPE_APP_REMOVE:
            ApplyAppRemove();
            break;

        case TYPE_APP_UPDATE:
            ApplyAppUpdate();
            break;

        case TYPE_FIRMWARE_UPDATE:
            // The firmware update will trigger a reboot, report that update done.
            ReportUpdateDone();
            ApplyFwUpdate();
            break;

        default:
            LE_FATAL("Unexpected update type %d.", updateUnpack_GetType());
    }


}


//--------------------------------------------------------------------------------------------------
/**
 * Called when an unpack finishes successfully.
 **/
//--------------------------------------------------------------------------------------------------
static void UnpackDone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    CallStatusHandlers(LE_UPDATE_STATE_UNPACKING, 100);

    // If the security unpack is already finished, then notify client that download successful and
    // wait for calling le_update_Install() api. Otherwise, wait for the security-unpack program
    // to finish.
    if (SecurityUnpackPipeline == NULL)
    {
        CallStatusHandlers(LE_UPDATE_STATE_DOWNLOAD_SUCCESS, 100);
        InstallReady = true;
    }
    else
    {
        State = STATE_SECURITY_CHECKING;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback from the update unpacker to report progress on the update.
 */
//--------------------------------------------------------------------------------------------------
static void HandleUpdateProgress
(
    updateUnpack_ProgressCode_t progressCode,  ///< The update state.
    unsigned int percentDone        ///< The percentage this state is complete.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("progressCode: %d, percentDone: %u", progressCode, percentDone);

    // Report progress to the client.
    switch (progressCode)
    {
        case UPDATE_UNPACK_STATUS_UNPACKING:
            CallStatusHandlers(LE_UPDATE_STATE_UNPACKING, percentDone);
            break;

        case UPDATE_UNPACK_STATUS_DONE:
            UnpackDone();
            break;

        case UPDATE_UNPACK_STATUS_BAD_PACKAGE:
            UpdateFailed(LE_UPDATE_ERR_BAD_PACKAGE);
            break;

        case UPDATE_UNPACK_STATUS_INTERNAL_ERROR:
            UpdateFailed(LE_UPDATE_ERR_INTERNAL_ERROR);
            break;
    }

}
//--------------------------------------------------------------------------------------------------
/**
 * Terminates the current update.
 **/
//--------------------------------------------------------------------------------------------------
static void EndUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (State == STATE_UNPACKING)
    {
        updateUnpack_Stop();
    }

    // Delete the security-unpack pipeline if it is still active.
    if (SecurityUnpackPipeline != NULL)
    {
        pipeline_Delete(SecurityUnpackPipeline);
        SecurityUnpackPipeline = NULL;
    }

    // State should only be changed to idle when when update ends(i.e. le_update_End api call or
    // client session closed), otherwise it may lead to race condition(i.e. updateDaemon will
    // accept another update while there is one unfinished update).
    State = STATE_IDLE;

    // Increment the current session reference by 2 (to keep it odd) so that we can identify stale
    // references.
    IpcSession = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle a client session closing on the le_update service interface.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateServiceClosed
(
    le_msg_SessionRef_t sessionRef, ///<[IN] Client session reference.
    void*               contextPtr  ///<[IN] Client context pointer.
)
//--------------------------------------------------------------------------------------------------
{
    // If this session is currently doing an update, cancel that.
    if (sessionRef == IpcSession)
    {
        EndUpdate();
    }

    // NOTE: We don't have to remove all the registered progress handlers for this session
    //       because the generated IPC code will call le_update_RemoveProgressHandler()
    //       automatically for us.
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGCHLD, called via the Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
static void SigChildHandler
(
    int sigNum
)
//--------------------------------------------------------------------------------------------------
{
    pipeline_CheckChildren();
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file at a given path.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFile
(
    const char* filePath
)
//--------------------------------------------------------------------------------------------------
{
    if ((unlink(filePath) != 0) && (errno != ENOENT))
    {
        LE_CRIT("Failed to delete file '%s' (%m).", filePath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports configuration settings from a given file into a given node in the tree.
 */
//--------------------------------------------------------------------------------------------------
static void ImportFile
(
    le_cfg_IteratorRef_t i, ///< Write transaction iterator.
    const char* filePath,   ///< File system path of file to import from.
    const char* nodePath    ///< Path of node in tree, relative to current position of iterator.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Importing configuration file '%s' to system configuration tree node '%s'.",
             filePath,
             nodePath);

    le_result_t result = le_cfgAdmin_ImportTree(i, filePath, nodePath);

    if (result != LE_OK)
    {
        // Can't complete the system update.  This will leave the current running system in a
        // bad state from which it cannot recover, so this is a fatal error.  Terminating now
        // will allow the problem to be detected early so corrective action can be taken.
        LE_FATAL("Failed (%s) to import config file '%s' to system tree node '%s'.",
                 LE_RESULT_TXT(result),
                 filePath,
                 nodePath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if given name is a valid config tree.
 *
 * returns
 *     - true if it is a valid config tree.
 *     - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsCfgTree
(
    const char* treeName   ///< [IN] Config tree name.
)
{

    char* extension = strrchr(treeName, '.');

    if (extension == NULL)
    {
        return false;
    }

    return (strcmp(extension, ".rock") == 0) ||
           (strcmp(extension, ".paper") == 0) ||
           (strcmp(extension, ".scissors") == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if given name is a valid system config tree.
 *
 * returns
 *     - true if it is a valid system config tree.
 *     - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsSystemCfgTree
(
    const char* treeName   ///< [IN] Config tree name.
)
{
    return (strcmp(treeName, "system.rock") == 0) ||
           (strcmp(treeName, "system.paper") == 0) ||
           (strcmp(treeName, "system.scissors") == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if given directory entry is an app config tree.
 *
 * returns
 *     - true if it is a valid app config tree.
 *     - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDirEntryAppCfgTree
(
    struct dirent* dp   ///< [IN] Directory entry in config dir.
)
{
    if (dp->d_type == DT_REG)
    {
        return IsCfgTree(dp->d_name) && !IsSystemCfgTree(dp->d_name);
    }
    else if (dp->d_type == DT_UNKNOWN)
    {
        // As per man page (http://man7.org/linux/man-pages/man3/readdir.3.html), DT_UNKNOWN
        // should be handled properly for portability purpose. Use stat(2) to check file info.
        struct stat stbuf;

        if (stat(dp->d_name, &stbuf) != 0)
        {
            LE_ERROR("Error when trying to stat '%s'. (%m)", dp->d_name);
        }
        else if (S_ISREG(stbuf.st_mode))
        {
            return IsCfgTree(dp->d_name) && !IsSystemCfgTree(dp->d_name);
        }
    }

    return false;
}



//--------------------------------------------------------------------------------------------------
/**
 * Checks if given config tree belongs to given app.
 *
 * returns
 *     - true if belongs to the given app.
 *     - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsThisAppsCfgTree
(
    const char* treeName,   ///< [IN] Config tree name to check.
    const char* appName     ///< [IN] App name
)
{
    char* dotStrPtr = strrchr(treeName, '.');

    if (dotStrPtr == NULL)
    {
        return false;
    }

    char tempTreeName[MAX_CFGTREE_NAME_BYTES] = "";

    LE_ASSERT(le_utf8_CopyUpToSubStr(tempTreeName,
                                     treeName,
                                     dotStrPtr,
                                     sizeof(tempTreeName),
                                     NULL) == LE_OK);
    return strcmp(appName, tempTreeName) == 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get number of app config trees.
 *
 * returns
 *     - Number of app config trees in config directory.
 */
//--------------------------------------------------------------------------------------------------
static int GetNumAppCfgTree
(
    DIR* dirPtr                                       ///< [IN] Directory containing config trees.
)
{
    int numAppCfgTree=0;
    struct dirent* dp;

    while ((dp = readdir(dirPtr)) != NULL)
    {
        if (IsDirEntryAppCfgTree(dp))
        {
            numAppCfgTree++;
        }
    }
    return numAppCfgTree;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get list of app config trees.
 */
//--------------------------------------------------------------------------------------------------
static void GetAppCfgTreeList
(
    DIR* dirPtr,                                     ///< [IN] Directory containing config trees.
    int numAppCfgTree,                               ///< [IN] Number of app config tree in directory.
    char (*cfgTreeList)[MAX_CFGTREE_NAME_BYTES]      ///< [OUT] Array containing list of config trees
                                                     ///< in specified config directory. Caller function
                                                     ///< is responsible to allocate it properly.


)
{
    struct dirent* dp;
    int count = 0;

    while ((dp = readdir(dirPtr)) != NULL)
    {
        if (IsDirEntryAppCfgTree(dp))
        {
            LE_ASSERT(le_utf8_Copy(cfgTreeList[count],
                                   dp->d_name,
                                   MAX_CFGTREE_NAME_BYTES,
                                   NULL) == LE_OK);
            count++;
        }

        if (count >= numAppCfgTree)
        {
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the tree pointed by config tree node from obsolete tree list..
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromListTreeInCfgNode
(
    le_cfg_IteratorRef_t cfgIter,  ///< [IN] Node containing config tree name
    size_t numAppCfgTree,          ///< [IN] No. of config tree in current config directory.
    char (*obsoleteTreeList)[MAX_CFGTREE_NAME_BYTES] ///< [IN] Array containing list of config trees
                                                     ///<      in current config directory.
)
{

    char cfgTree[LIMIT_MAX_APP_NAME_BYTES] = "";
    LE_FATAL_IF(le_cfg_GetNodeName(cfgIter, "", cfgTree, sizeof(cfgTree)) != LE_OK,
                "Application name in config is too long.");

    if (strcmp(cfgTree, "system") == 0)
    {
        return;
    }

    int i = 0;

    while (i < numAppCfgTree)
    {
        if ((obsoleteTreeList[i][0] != 0) &&
            IsThisAppsCfgTree(obsoleteTreeList[i], cfgTree))
        {
            // There may be more than one config tree (e.g. helloWorld.rock, helloWorld.paper), so
            // don't break after first match.
            LE_DEBUG("Removed cfgTree '%s' from obsolete list", obsoleteTreeList[i]);
            obsoleteTreeList[i][0] = 0;
        }
        i++;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove trees that are in App Access list from obsolete tree list.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromListTreeInAppACL
(
    const char* appName,        ///< [IN] App name.
    size_t numAppCfgTree,       ///< [IN] No. of config tree in current config directory.
    char (*obsoleteTreeList)[MAX_CFGTREE_NAME_BYTES]  ///< [IN] Array containing list of config trees
                                                      ///<      in current config directory.
)
{

    char cfgTreePath[LIMIT_MAX_PATH_BYTES] = "";
    snprintf(cfgTreePath, sizeof(cfgTreePath), "system:/apps/%s/configLimits/acl",appName);
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn(cfgTreePath);

    if (le_cfg_GoToFirstChild(cfgIter) != LE_NOT_FOUND)
    {
        do
        {
            RemoveFromListTreeInCfgNode(cfgIter,
                             numAppCfgTree,
                             obsoleteTreeList);
        }
        while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);
    }
    le_cfg_CancelTxn(cfgIter);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function traverses the system config tree, find required tree and remove unnecessary trees.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupAppConfigTrees
(
    void
)
{
    // Path to the config tree directory in the linux filesystem.
    const char* configDirPath = "/legato/systems/current/config";
    DIR* dirPtr;

    LE_FATAL_IF((dirPtr = opendir(configDirPath)) == NULL, "Can't open %s (%m)", configDirPath);

    // We don't know how many apps config trees are in config directory. First count them.
    size_t numAppCfgTree=GetNumAppCfgTree(dirPtr);

    LE_DEBUG("Total app cfgTree: %zu", numAppCfgTree);
    // Now Rewind the directory
    rewinddir(dirPtr);

    char obsoleteCfgTreeList[numAppCfgTree][MAX_CFGTREE_NAME_BYTES];
    GetAppCfgTreeList(dirPtr, numAppCfgTree, obsoleteCfgTreeList);

    closedir(dirPtr);

    // Iterate over config tree and mark as zero which is needed
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("system:/apps");

    if (le_cfg_GoToFirstChild(cfgIter) != LE_NOT_FOUND)
    {
        // Iterate over the list of apps.
        do
        {
            char appName[LIMIT_MAX_APP_NAME_BYTES] = "";

            LE_FATAL_IF(le_cfg_GetNodeName(cfgIter, "", appName, sizeof(appName)-1) != LE_OK,
                        "Application name in config is too long.");
            LE_DEBUG("Removing required cfgTrees for app: '%s' from obsolete list", appName);

            // Remove the app tree(currently pointed by cfgIter) from obsolete list.
            RemoveFromListTreeInCfgNode(cfgIter, numAppCfgTree, obsoleteCfgTreeList);

            // Remove trees that are in App Access list from obsolete tree list.
            RemoveFromListTreeInAppACL(appName, numAppCfgTree, obsoleteCfgTreeList);
        }
        while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);

    }

    le_cfg_CancelTxn(cfgIter);

    // Now delete all obsolete config trees.
    int i = 0;
    while(i < numAppCfgTree)
    {
        if (obsoleteCfgTreeList[i][0] != 0)
        {
            char obsoleteCfgTree[LIMIT_MAX_PATH_BYTES];
            LE_ASSERT (snprintf(obsoleteCfgTree, sizeof(obsoleteCfgTree), "%s/%s",configDirPath,
                                obsoleteCfgTreeList[i])
                        < sizeof(obsoleteCfgTree));
            LE_DEBUG("Deleting tree '%s'", obsoleteCfgTree);
            DeleteFile(obsoleteCfgTree);
        }
        i++;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Before we advertise our services, we check to see if we need to import new system
 * configuration settings.  This happens when we start after a system update has just been
 * applied.
 */
//--------------------------------------------------------------------------------------------------
static void FinishSystemUpdate
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Set correct smack label for certain tools
    smack_SetLabelExec("/legato/systems/current/bin/_appStopClient", "admin");
    smack_SetLabelExec("/legato/systems/current/bin/sdir", "framework");

    // Path to the config tree directory in the linux filesystem.
    const char* usersFilePath = "/legato/systems/current/config/users.cfg";
    const char* appsFilePath = "/legato/systems/current/config/apps.cfg";
    const char* modulesFilePath = "/legato/systems/current/config/modules.cfg";
    const char* frameworkFilePath = "/legato/systems/current/config/framework.cfg";

    // Ensure that the cfg files are set with the correct smack labels
    smack_SetLabel(usersFilePath, "framework");
    smack_SetLabel(appsFilePath, "framework");
    smack_SetLabel(modulesFilePath, "framework");
    smack_SetLabel(frameworkFilePath, "framework");

    // If users.cfg, apps.cfg or modules.cfg exist in the directory containing the configuration data files,
    // import them into the system config tree to finish a previous update operation.
    bool usersFileExists = FileExists(usersFilePath);
    bool appsFileExists = FileExists(appsFilePath);
    bool modulesFileExists = FileExists(modulesFilePath);
    bool frameworkFileExists = FileExists(frameworkFilePath);
    if (usersFileExists || appsFileExists || modulesFileExists || frameworkFileExists)
    {
        LE_INFO("Finishing system update...");

        // To work around a bug in the "import" feature of the Config Tree, start by deleting
        // the "users", "apps" and "modules" branches of the system config tree in a separate transaction
        // before starting the "import" transaction.  If we don't do this, old contents of
        // the "users", "apps" and "modules" branches will still remain after the import operations.
        le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("");
        le_cfg_DeleteNode(i, "users");
        le_cfg_DeleteNode(i, "apps");
        le_cfg_DeleteNode(i, "modules");
        le_cfg_DeleteNode(i, "framework");
        le_cfg_CommitTxn(i);

        i = le_cfg_CreateWriteTxn("");

        if (usersFileExists)
        {
            LE_INFO("Importing file '%s' into system:/users", usersFilePath);
            ImportFile(i, usersFilePath, "users");
        }

        if (appsFileExists)
        {
            LE_INFO("Importing file '%s' into system:/apps", appsFilePath);
            ImportFile(i, appsFilePath, "apps");
        }

        if (modulesFileExists)
        {
            LE_INFO("Importing file '%s' into system:/modules", modulesFilePath);
            ImportFile(i, modulesFilePath, "modules");
        }

        if (frameworkFileExists)
        {
            LE_INFO("Importing file '%s' into system:/framework", frameworkFilePath);
            ImportFile(i, frameworkFilePath, "framework");
        }

        le_cfg_CommitTxn(i);

        // Cleanup unnecessary trees copied from old system.
        CleanupAppConfigTrees();

        // Flag that new system is installed. This function should be called before deleting the
        // 'user', 'app' and 'module' config trees. Otherwise, race condition may arise if there
        // is a power cut immediately after deletion of these config trees.
        updateInfo_FlagNewSys();

        // Ensure that the newSystem in le_fs is using the "framework" label. Otherwise apps using
        // le_fs will encounter issues.
        smack_SetLabel("/data/le_fs/newSystem", "framework");

        // Delete users.cfg and apps.cfg.
        DeleteFile(usersFilePath);
        DeleteFile(appsFilePath);
        DeleteFile(modulesFilePath);
        DeleteFile(frameworkFilePath);

        LE_INFO("System update finished.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether or not a given line from the /etc/passwd file or /etc/group file should
 * be kept.  If the user or group name at the beginning of the line does not being with "app",
 * then it should be kept.  Also, we want to keep app users and groups for apps that are installed
 * in the current (running) system.
 **/
//--------------------------------------------------------------------------------------------------
static bool ShouldKeepUserOrGroup
(
    const char* fileLine
)
//--------------------------------------------------------------------------------------------------
{
    // If the user name does not begin with "app", keep it.
    if (strncmp("app", fileLine, 3) != 0)
    {
        return true;
    }

    char appName[LIMIT_MAX_APP_NAME_BYTES];
    le_result_t result = le_utf8_CopyUpToSubStr(appName,
                                                fileLine + 3, // + 3 to skip over "app" prefix.
                                                ":",
                                                sizeof(appName),
                                                NULL);
    if (result != LE_OK)
    {
        LE_CRIT("App user name too long. Discarding.");
        return false;
    }

    // If the app exists in the current system, keep it.
    char path[PATH_MAX];
    LE_ASSERT(snprintf(path, sizeof(path), "/legato/systems/current/apps/%s", appName)
              < sizeof(path));
    return le_dir_IsDir(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy lines from an existing passwd or group file that are either non-app users or groups or
 * that correspond to apps that exist in the current system.
 **/
//--------------------------------------------------------------------------------------------------
static void CopyExistingUserOrGroupLines
(
    FILE* outputFile,   ///< File to copy lines into.
    const char* inputFilePath ///< File system path of file to copy lines from.
)
//--------------------------------------------------------------------------------------------------
{
    // Open the existing passwd file.
    FILE* inputFile = fopen(inputFilePath, "r");
    LE_FATAL_IF(inputFile == NULL, "Failed to open '%s' (%m)", inputFilePath);

    // For each line in the existing file,
    for (;;)
    {
        // Read the line.
        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), inputFile) == NULL)
        {
            if (feof(inputFile))
            {
                break;
            }
            else
            {
                LE_FATAL("Error reading from '%s'.", inputFilePath);
            }
        }

        if (ShouldKeepUserOrGroup(buffer))
        {
            LE_DEBUG("Keeping line: %s", buffer);

            LE_FATAL_IF(fputs(buffer, outputFile) == EOF, "Failed to write (%m)");
        }
        else
        {
            LE_INFO("Discarding line: %s", buffer);
        }
    }

    // Close the existing passwd file.
    LE_CRIT_IF(fclose(inputFile) != 0, "Failed to close file '%s' (%m)", inputFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Make sure the users and groups are set up correctly for the apps we have installed
 * in the current system.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateUsersAndGroups
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Check if /etc is writable. Else skip the /etc/passwd and /etc/group update
    if (0 == access("/etc/passwd", W_OK))
    {
        // Create a new passwd file and group file.
        static const char* newPasswdFilePath = "/etc/newpasswd";
        static const char* newGroupFilePath = "/etc/newgroup";
        FILE* newPasswdFile = fopen(newPasswdFilePath, "w");
        LE_FATAL_IF(newPasswdFile == NULL, "Failed to create '%s'.", newPasswdFilePath);
        FILE* newGroupFile = fopen(newGroupFilePath, "w");
        LE_FATAL_IF(newGroupFile == NULL, "Failed to create '%s'.", newGroupFilePath);

        // Set correct access permissions: u=rw,go=r
        LE_FATAL_IF(chmod(newPasswdFilePath, (S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)),
                    "Failed to set permissions 0664 to '%s'.", newPasswdFilePath);
        // Set correct access permissions: u=rw,go=r
        LE_FATAL_IF(chmod(newGroupFilePath, (S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)),
                    "Failed to set permissions 0664 to '%s'.", newGroupFilePath);

        // Copy existing passwd file and group file contents that we need for the current system.
        CopyExistingUserOrGroupLines(newPasswdFile, "/etc/passwd");
        CopyExistingUserOrGroupLines(newGroupFile, "/etc/group");

        // Close the new passwd and group files.
        LE_CRIT_IF(fclose(newPasswdFile) != 0, "Failed to close '%s' (%m).", newPasswdFilePath);
        LE_CRIT_IF(fclose(newGroupFile) != 0, "Failed to close '%s' (%m).", newGroupFilePath);

        // Rename the new passwd and group files over top of the old ones.
        LE_FATAL_IF(rename(newPasswdFilePath, "/etc/passwd") != 0,
                    "Failed to rename '%s' to '/etc/passwd' (%m).",
                    newPasswdFilePath);
        LE_FATAL_IF(rename(newGroupFilePath, "/etc/group") != 0,
                    "Failed to rename '%s' to '/etc/group' (%m).",
                    newGroupFilePath);

        // Leave these system files with '_' label; otherwise it will inherit the label of
        // updateDaemon and cause other issues e.g. liblegato user API access etc...
        smack_SetLabel("/etc/passwd", "_");
        smack_SetLabel("/etc/group", "_");
    }

    // Walk the apps directory under the current system, and for each app in the directory,
    // make sure it has a user account and primary group in the new passwd and group files.
    char* pathArrayPtr[] = { "/legato/systems/current/apps", NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL, NULL);
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        if (entPtr->fts_info == FTS_SL)
        {
            if (entPtr->fts_level == 1)
            {
                char* appNamePtr = le_path_GetBasenamePtr(entPtr->fts_path, "/");

                char userName[LIMIT_MAX_APP_NAME_BYTES + 3]; // 3 = length of "app" prefix.
                LE_ASSERT(  snprintf(userName, sizeof(userName), "app%s", appNamePtr)
                          < sizeof(userName));

                le_result_t result = user_Create(userName, NULL, NULL);
                if (result == LE_OK)
                {
                    LE_INFO("User '%s' created for app '%s'.", userName, appNamePtr);
                }
                else if (result == LE_DUPLICATE)
                {
                    LE_DEBUG("User '%s' already existed for app '%s'.", userName, appNamePtr);
                }
                else
                {
                    LE_CRIT("Failed to create user '%s' (%s)", userName, LE_RESULT_TXT(result));
                    LE_FATAL("Legato installation failure. System is unworkable");
                }

                // We don't need to go into this directory.
                fts_set(ftsPtr, entPtr, FTS_SKIP);
            }
        }
        else if (entPtr->fts_level != 0)
        {
            LE_ERROR("Unexpected file type %d at '%s'", entPtr->fts_info, entPtr->fts_path);
        }
    }

    fts_close(ftsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pipeline completion function for the security-unpack.
 *
 * @note This will usually be called AFTER the updateUnpack operation reports completion, even if
 *       the updateUnpack operation was interrupted by the security-unpack program exiting.
 */
//--------------------------------------------------------------------------------------------------
static void PipelineDone
(
    pipeline_Ref_t pipeline,
    int status
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(pipeline == SecurityUnpackPipeline);

    pipeline_Delete(SecurityUnpackPipeline);
    SecurityUnpackPipeline = NULL;
    le_update_ErrorCode_t errCode = LE_UPDATE_ERR_NONE;

    LE_FATAL_IF(State == STATE_APPLYING, "Bad state, can't apply update without security check");

    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_SUCCESS)
        {
            LE_DEBUG("security-unpack completed successfully.");
            // Only allowed state here is STATE_UNPACKING or STATE_SECURITY_CHECKING. If state is
            // STATE_UNPACKING, then we return and wait for unpacking to finish (see UnpackDone()).
            if (State == STATE_SECURITY_CHECKING)
            {
                CallStatusHandlers(LE_UPDATE_STATE_DOWNLOAD_SUCCESS, 100);
                InstallReady = true;
            }
            return;
        }
        else if (WEXITSTATUS(status) == EXIT_FAILURE)
        {
            LE_CRIT("security-unpack reported a security violation.");
            errCode = LE_UPDATE_ERR_SECURITY_FAILURE;
        }
        else
        {
            LE_CRIT("security-unpack terminated (exit code: %d).", WEXITSTATUS(status));
            errCode = LE_UPDATE_ERR_INTERNAL_ERROR;
        }
    }
    else if (WIFSIGNALED(status))
    {
        LE_CRIT("security-unpack was killed by signal %d.", WTERMSIG(status));
        errCode = LE_UPDATE_ERR_INTERNAL_ERROR;
    }
    else
    {
        LE_CRIT("security-unpack died for an unknown reason (status: %d).", status);
        errCode = LE_UPDATE_ERR_INTERNAL_ERROR;
    }

    // This is an error scenario. If the unpacker is still running, stop it.
    if (State == STATE_UNPACKING)
    {
        updateUnpack_Stop();
    }

    UpdateFailed(errCode);

}


//--------------------------------------------------------------------------------------------------
/**
 * Checks that the current IPC session is the one that started the current update.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsSessionValid
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (le_update_GetClientSessionRef() != IpcSession)
    {
        LE_KILL_CLIENT("Client tried to perform operation on update they didn't start.");

        return false;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_update_Progress'
 *
 * This event is used for showing status of ongoing update process.
 */
//--------------------------------------------------------------------------------------------------
le_update_ProgressHandlerRef_t le_update_AddProgressHandler
(
    le_update_ProgressHandlerFunc_t func,  ///<[IN] Pointer to client callback function.
    void* contextPtr                       ///<[IN] Context pointer.
)
{
    ClientProgressHandler_t* handlerPtr = le_mem_ForceAlloc(ClientProgressHandlerPool);

    handlerPtr->link = LE_DLS_LINK_INIT;
    handlerPtr->func = func;
    handlerPtr->contextPtr = contextPtr;
    handlerPtr->ref = (le_update_ProgressHandlerRef_t)NextClientProgressHandlerRef;
    NextClientProgressHandlerRef += 1;
    handlerPtr->sessionRef = le_update_GetClientSessionRef();
    le_dls_Queue(&ClientProgressHandlerList, &handlerPtr->link);

    return handlerPtr->ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_update_Progress'
 */
//--------------------------------------------------------------------------------------------------
void le_update_RemoveProgressHandler
(
    le_update_ProgressHandlerRef_t handlerRef   ///[IN] Reference to client callback function.
)
//--------------------------------------------------------------------------------------------------
{
    // Search the list of handlers until we find the one that has this reference.
    le_dls_Link_t* linkPtr;
    for (linkPtr = le_dls_Peek(&ClientProgressHandlerList);
         linkPtr != NULL;
         linkPtr = le_dls_PeekNext(&ClientProgressHandlerList, linkPtr))
    {
        ClientProgressHandler_t* handlerPtr = CONTAINER_OF(linkPtr, ClientProgressHandler_t, link);

        if (handlerPtr->ref == handlerRef)
        {
            if (handlerPtr->sessionRef != le_update_GetClientSessionRef())
            {
                LE_KILL_CLIENT("Attempt to remove someone else's progress handler!");
                return;
            }
            le_dls_Remove(&ClientProgressHandlerList, linkPtr);
            le_mem_Release(handlerPtr);
            return;
        }
    }

    LE_KILL_CLIENT("Invalid progress handler reference!");
}


//-------------------------------------------------------------------------------------------------
/**
 * Starts an update.
 *
 * @return
 *      - LE_OK if accepted.
 *      - LE_BUSY if another update is in progress.
 *      - LE_UNSUPPORTED if Legato system is R/O.
 *      - LE_UNAVAILABLE if updates are deferred.
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_update_Start
(
    int clientFd                ///<[IN] Open file descriptor from which the update can be read.
)
{
    if (IsReadOnly)
    {
        LE_ERROR("Legato is R/O");
        return LE_UNSUPPORTED;
    }

    LE_DEBUG("fd: %d", clientFd);

    if (!IsValidFileDesc(clientFd))
    {
        LE_KILL_CLIENT("Received invalid update pack file descriptor.");
        return LE_OK;    // Client is dead, so this value doesn't matter.
    }

    le_result_t result;

    // Reject updates unless IDLE.
    switch (State)
    {
        case STATE_IDLE:
            if (updateCtrl_HasDefers())
            {
                LE_WARN("Updates are deferred. Request denied.");
                result = LE_UNAVAILABLE;
            }
            else
            {
                LE_INFO("Update request accepted.");
                result = LE_OK;
            }
            break;

        default:
            LE_WARN("Update denied. Another update is already in progress.");
            result = LE_BUSY;
            break;

    }
    if (result != LE_OK)
    {
        fd_Close(clientFd);
        return result;
    }

    // Remember the IPC session reference in case the session drops.
    IpcSession = le_update_GetClientSessionRef();

    // Reset the error code.
    ErrorCode = LE_UPDATE_ERR_NONE;

    //Reset the InstallReady flag
    InstallReady = false;

    // Create a pipeline: clientfd -> security-unpack -> readFd
    SecurityUnpackPipeline = pipeline_Create();
    pipeline_SetInput(SecurityUnpackPipeline, clientFd);
    pipeline_Append(SecurityUnpackPipeline, SecurityUnpack, NULL);
    int readFd = pipeline_CreateOutputPipe(SecurityUnpackPipeline);
    pipeline_Start(SecurityUnpackPipeline, PipelineDone);

    // Close the input fd as pipeline_SetInput dups() this.
    fd_Close(clientFd);

    // Pass the readFd to the updateUnpacker module.
    LE_DEBUG("Starting unpack");
    updateUnpack_Start(readFd, HandleUpdateProgress);

    State = STATE_UNPACKING;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get error code when update fails.
 *
 * @return
 *      - Error code of encountered error.
 *      - ERR_NONE if update is in any other state.
 */
//--------------------------------------------------------------------------------------------------
le_update_ErrorCode_t le_update_GetErrorCode()
{
    IsSessionValid();   // Kills client if not valid.

    return ErrorCode;
}


//-------------------------------------------------------------------------------------------------
/**
 * Install the update
 *
 * @return
 *      - LE_OK if installation started.
 *      - LE_BUSY if package download is not finished yet.
 *      - LE_FAULT if there is an error. Check logs
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_update_Install()
{
    if (!IsSessionValid())
    {
        return LE_FAULT;
    }

    le_result_t result = LE_OK;

    switch (State)
    {
        case STATE_UNPACKING:
        case STATE_SECURITY_CHECKING:
            if (InstallReady)
            {
                le_event_Report(InstallEventId, NULL, 0);
                InstallReady = false;
                return LE_OK;
            }
            else
            {
                LE_ERROR("Not ready for install. Still downloading and verifying package");
                result = LE_BUSY;
            }
            break;

        case STATE_APPLYING:
            LE_ERROR("Already installing package");
            result = LE_FAULT;
            break;

        case STATE_IDLE:
            LE_ERROR("No pending installation. No package downloaded or it already installed");
            result = LE_FAULT;
            break;
    }

    return result;

}

//--------------------------------------------------------------------------------------------------
/**
 * Ends an update session.  If the update is not finished yet, cancels it.
 *
 * @note The update session reference becomes invalid after this.
 */
//--------------------------------------------------------------------------------------------------
void le_update_End()
{
    if (!IsSessionValid())   // Kills client if not valid.
    {
        return;
    }

    EndUpdate();
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the currently running system.
 *
 * @return The index of the running system.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_update_GetCurrentSysIndex
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    return system_Index();
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the hash ID from a given system.
 *
 * @return
 *      - LE_OK if no problems are encountered.
 *      - LE_NOT_FOUND if the given index does not correspond to an available system.
 *      - LE_OVERFLOW if the supplied buffer is too small.
 *      - LE_FORMAT_ERROR if there are problems reading the hash from the system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_update_GetSystemHash
(
    int32_t systemIndex,  ///< [IN]  The system to read the hash for.
    char* hashStr,        ///< [OUT] Buffer to hold the system hash string.
    size_t hashStrSize    ///< [IN]  Size of the hashStr buffer.
)
//--------------------------------------------------------------------------------------------------
{
    if (hashStrSize < LIMIT_MD5_STR_BYTES)
    {
        return LE_OVERFLOW;
    }

    return system_GetSystemHash(systemIndex, hashStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the index for the previous system in the chain, using the current system as a starting point.
 *
 * @return The index to the system that's previous to the given system.  -1 is returned if the
 *         previous system was not found.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_update_GetPreviousSystemIndex
(
    int32_t systemIndex  ///< [IN] Get the system that's earlier to this system.
)
//--------------------------------------------------------------------------------------------------
{
    return system_GetPreviousSystemIndex(systemIndex);
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes a given app from the target device.
 *
 * @return
 *     - LE_OK if successful
 *     - LE_BUSY if system busy.
 *     - LE_NOT_FOUND if given app is not installed.
 *     - LE_FAULT for any other failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_appRemove_Remove
(
    const char* appName
)
{
    if (IsReadOnly)
    {
        LE_ERROR("Legato is R/O");
        return LE_FAULT;
    }

    // Check whether any update is active.
    if (State != STATE_IDLE)
    {
        LE_WARN("App removal requested while an update is already in progress.");

        return LE_BUSY;
    }

    if (updateCtrl_HasDefers())
    {
        LE_WARN("App removal requested while a defer is in effect.");

        return LE_BUSY;
    }

    // Make sure there's space to make a snapshot if we need to.
    system_RemoveUnneeded();
    system_RemoveUnusedApps();

    // If the removal was successful, kick off the probation timer.
    le_result_t result = app_RemoveIndividual(appName);

    if (result == LE_OK)
    {
        StartProbation();
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the probation timer period.
 *
 * @return The timer period, in milliseconds.
 **/
//--------------------------------------------------------------------------------------------------
static size_t GetProbationPeriod
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int period;

    const char* envStrPtr = getenv("LE_PROBATION_MS");

    if ((envStrPtr == NULL) || (le_utf8_ParseInt(&period, envStrPtr) != LE_OK) || (period <= 0))
    {
        period = LE_CONFIG_PROBATION_PERIOD * 1000;
    }

    LE_INFO("System probation period = %d ms (~ %d minutes)", period, period / 60000);

    return (size_t)period;
}


//--------------------------------------------------------------------------------------------------
/**
 * The main function for the update daemon.  Listens for commands from process/components and apply
 * update accordingly.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Block signals.
    le_sig_Block(SIGCHLD);
    le_sig_Block(SIGPIPE);

    // Initialize the User module
    user_Init();

    // Initialize pools
    ClientProgressHandlerPool = le_mem_CreatePool("ProgressHandler",
                                                  sizeof(ClientProgressHandler_t));

    // Initialize the client progress handler reference counter to some random value.
    NextClientProgressHandlerRef = random();

    InstallEventId = le_event_CreateId("InstallEvent", 0);
    le_event_AddHandler("Installer", InstallEventId, ApplyUpdate);

    // Register SIGCHLD signal handler
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    // Create the Probation Timer.
    ProbationTimer = le_timer_Create("Probation");
    le_timer_SetHandler(ProbationTimer, HandleProbationExpiry);
    le_timer_SetMsInterval(ProbationTimer, GetProbationPeriod());
    le_timer_SetWakeup(ProbationTimer, false);

    // Make sure we can set file permissions properly.
    umask(0);

    IsReadOnly = sysStatus_IsReadOnly();

    if (!IsReadOnly)
    {
        // If a system update needs finishing, finish it now.
        FinishSystemUpdate();

        // If an app update needs finishing, finish it now.
        app_FinishUpdates();

        // Make sure the users and groups are set up correctly for the apps we have installed
        // in the current system.  We may have updated or rolled-back to a different system with
        // different apps than we had last time we ran.
        UpdateUsersAndGroups();

        // If the current system is "good", go into the IDLE state,
        if (sysStatus_Status() == SYS_GOOD)
        {
            LE_INFO("Current system is 'good'.");
        }
        // Otherwise, this system is on probation.
        else
        {
            StartProbation();
        }
    }
    else
    {
        // Test if /etc/passwd and /etc/group are writable to run sandboxed apps
        if ((0 == access("/etc/passwd", W_OK)) && (0 == access("/etc/group", W_OK)))
        {
            UpdateUsersAndGroups();
        }
        else
        {
            LE_CRIT("/etc/passwd and /etc/group are read-only. Sandboxes are not supported");
        }

        State = STATE_IDLE;

        if (sysStatus_Status() == SYS_GOOD)
        {
            LE_INFO("Current system is 'good'.");
        }
        else
        {
            LE_ERROR("Current R/O system is 'not good'.");
        }
    }

    // Make sure that we can report app install events.
    instStat_Init();

    updateCtrl_Initialize();

    // Register session close handler for the le_update service.
    le_msg_AddServiceCloseHandler(le_update_GetServiceRef(), UpdateServiceClosed, NULL);

    // Tell the Supervisor that we are up by closing stdin and reopening it to /dev/null.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );
}
