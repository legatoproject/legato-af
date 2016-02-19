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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
#include "supCtrl.h"


// Default probation period.
#ifndef PROBATION_PERIOD
#define PROBATION_PERIOD  (30 * 60 * 1000)  // 30 minutes.
#endif


//--------------------------------------------------------------------------------------------------
/**
 * State of the Update Daemon state machine.
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    STATE_IDLE,         ///< The current system is "good" and there's nothing to do.
    STATE_PROBATION,    ///< The current system is in its probation period.  Updates not allowed.
    STATE_UPDATING      ///< The current system is "good" and an update is in progress.
}
State;


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
 * Error code.
 */
//--------------------------------------------------------------------------------------------------
static le_update_ErrorCode_t ErrorCode = LE_UPDATE_ERR_NONE;


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
    State = STATE_PROBATION;

    if (le_timer_Start(ProbationTimer) == LE_BUSY)
    {
        le_timer_Restart(ProbationTimer);
    }
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
    State = STATE_IDLE;

    LE_INFO("System passed probation. Marking 'good'.");

    system_MarkGood();
    system_RemoveUnneeded();
    system_RemoveUnusedApps();
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
 * Callback from the update unpacker to report progress on the update.
 */
//--------------------------------------------------------------------------------------------------
static void HandleProgressReport
(
    updateUnpack_ProgressCode_t progressCode,  ///< The update state.
    unsigned int percentDone        ///< The percentage this state is complete.
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("progressCode: %d, percentDone: %u", progressCode, percentDone);

    bool failed = false;

    // Report progress to the client.
    switch (progressCode)
    {
        case UPDATE_UNPACK_STATUS_UNPACKING:
            CallStatusHandlers(LE_UPDATE_STATE_UNPACKING, percentDone);

            // ** RETURN - There's still more work to do.
            return;

        case UPDATE_UNPACK_STATUS_APPLYING:
            CallStatusHandlers(LE_UPDATE_STATE_APPLYING, percentDone);

            // ** RETURN - There's still more work to do.
            return;

        case UPDATE_UNPACK_STATUS_APP_UPDATED:
        case UPDATE_UNPACK_STATUS_SYSTEM_UPDATED:
        case UPDATE_UNPACK_STATUS_WAIT_FOR_REBOOT:
            CallStatusHandlers(LE_UPDATE_STATE_APPLYING, 100);
            CallStatusHandlers(LE_UPDATE_STATE_SUCCESS, 100);
            break;

        case UPDATE_UNPACK_STATUS_BAD_PACKAGE:
            CallStatusHandlers(LE_UPDATE_STATE_UNPACKING, percentDone);
            if (ErrorCode == LE_UPDATE_ERR_NONE)
            {
                ErrorCode = LE_UPDATE_ERR_BAD_PACKAGE;
            }
            failed = true;
            break;

        case UPDATE_UNPACK_STATUS_INTERNAL_ERROR:
            CallStatusHandlers(LE_UPDATE_STATE_UNPACKING, percentDone);
            if (ErrorCode == LE_UPDATE_ERR_NONE)
            {
                ErrorCode = LE_UPDATE_ERR_INTERNAL_ERROR;
            }
            failed = true;
            break;
    }

    // If the update failed, go back to IDLE and report the failure to the client.
    if (failed)
    {
        State = STATE_IDLE;
        CallStatusHandlers(LE_UPDATE_STATE_FAILED, percentDone);
    }
    // If a system update finished successfully,
    else if (progressCode == UPDATE_UNPACK_STATUS_SYSTEM_UPDATED)
    {
        // Ask the Supervisor to restart Legato.
        supCtrl_RestartLegato();

        // Stay in the UPDATING state while waiting to be restarted.
    }
    // If an update to one or more individual apps was successfully completed,
    else if (progressCode == UPDATE_UNPACK_STATUS_APP_UPDATED)
    {
        StartProbation();

        LE_INFO("Individual app changes applied. System on probation (timer started).");
    }
    // Note: If the update was a firmware update, there's no probation.
    //       The firmware update will trigger a reboot, so we just stay in the UPDATING state.
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
    if (State == STATE_UPDATING)
    {
        updateUnpack_Stop();
        State = STATE_IDLE;
    }

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
    // Path to the config tree directory in the linux filesystem.

    const char* usersFilePath = "/legato/systems/current/config/users.cfg";
    const char* appsFilePath = "/legato/systems/current/config/apps.cfg";

    // If users.cfg or apps.cfg exist in the directory containing the configuration data files,
    // import them into the system config tree to finish a previous update operation.
    bool usersFileExists = FileExists(usersFilePath);
    bool appsFileExists = FileExists(appsFilePath);
    if (usersFileExists || appsFileExists)
    {
        LE_INFO("Finishing system update...");

        // To work around a bug in the "import" feature of the Config Tree, start by deleting
        // the "users" and "apps" branches of the system config tree in a separate transaction
        // before starting the "import" transaction.  If we don't do this, old contents of
        // the "users" and "apps" branches will still remain after the import operations.
        le_cfg_IteratorRef_t i = le_cfg_CreateWriteTxn("");
        le_cfg_DeleteNode(i, "users");
        le_cfg_DeleteNode(i, "apps");
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

        le_cfg_CommitTxn(i);

        // Delete users.cfg and apps.cfg.
        DeleteFile(usersFilePath);
        DeleteFile(appsFilePath);

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
    LE_FATAL_IF(fclose(inputFile) != 0, "Failed to close file '%s' (%m)", inputFilePath);
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
    // Create a new passwd file and group file.
    static const char* newPasswdFilePath = "/etc/newpasswd";
    static const char* newGroupFilePath = "/etc/newgroup";
    FILE* newPasswdFile = fopen(newPasswdFilePath, "w");
    LE_FATAL_IF(newPasswdFile == NULL, "Failed to create '%s'.", newPasswdFilePath);
    FILE* newGroupFile = fopen(newGroupFilePath, "w");
    LE_FATAL_IF(newGroupFile == NULL, "Failed to create '%s'.", newGroupFilePath);

    // Copy existing passwd file and group file contents that we need for the current system.
    CopyExistingUserOrGroupLines(newPasswdFile, "/etc/passwd");
    CopyExistingUserOrGroupLines(newGroupFile, "/etc/group");

    // Close the new passwd and group files.
    LE_FATAL_IF(fclose(newPasswdFile) != 0, "Failed to close '%s' (%m).", newPasswdFilePath);
    LE_FATAL_IF(fclose(newGroupFile) != 0, "Failed to close '%s' (%m).", newGroupFilePath);

    // Rename the new passwd and group files over top of the old ones.
    LE_FATAL_IF(rename(newPasswdFilePath, "/etc/passwd") != 0,
                "Failed to rename '%s' to '/etc/passwd' (%m).",
                newPasswdFilePath);
    LE_FATAL_IF(rename(newGroupFilePath, "/etc/group") != 0,
                "Failed to rename '%s' to '/etc/group' (%m).",
                newGroupFilePath);

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
                    LE_FATAL("Failed to create user '%s' (%s)", userName, LE_RESULT_TXT(result));
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

    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_FAILURE)
        {
            LE_ERROR("security-unpack reported a security violation.");

            ErrorCode = LE_UPDATE_ERR_SECURITY_FAILURE;
        }
        else
        {
            LE_ERROR("security-unpack terminated (exit code: %d).", WEXITSTATUS(status));
        }
    }
    else if (WIFSIGNALED(status))
    {
        LE_WARN("security-unpack was killed by signal %d.", WTERMSIG(status));
    }
    else
    {
        LE_WARN("security-unpack died for an unknown reason (status: %d).", status);
    }

    pipeline_Delete(SecurityUnpackPipeline);
    SecurityUnpackPipeline = NULL;
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
 *      - LE_UNAVAILABLE if the system is still in "probation" (not marked "good" yet).
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_update_Start
(
    int clientFd                ///<[IN] Open file descriptor from which the update can be read.
)
{
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
        case STATE_UPDATING:
            LE_INFO("Update denied. Another update is already in progress.");
            result = LE_BUSY;
            break;

        case STATE_PROBATION:
        case STATE_IDLE:
            LE_INFO("Update request accepted.");
            result = LE_OK;
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

    // Create a pipeline: clientfd -> security-unpack -> readFd
    SecurityUnpackPipeline = pipeline_Create();
    pipeline_SetInput(SecurityUnpackPipeline, clientFd);
    pipeline_Append(SecurityUnpackPipeline, SecurityUnpack, NULL);
    int readFd = pipeline_CreateOutputPipe(SecurityUnpackPipeline);
    pipeline_Start(SecurityUnpackPipeline, PipelineDone);

    // Pass the readFd to the updateUnpacker module.
    LE_DEBUG("Starting unpack");
    updateUnpack_Start(readFd, HandleProgressReport);

    State = STATE_UPDATING;

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
 * @return LE_OK if successful
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_appRemove_Remove
(
    const char* appName
)
{
    // Check whether any update is active.
    if (State == STATE_UPDATING)
    {
        LE_WARN("App removal requested while an update is already in progress.");

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
        period = PROBATION_PERIOD;
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


    // Initialize pools
    ClientProgressHandlerPool = le_mem_CreatePool("ProgressHandler",
                                                  sizeof(ClientProgressHandler_t));

    // Initialize the client progress handler reference counter to some random value.
    NextClientProgressHandlerRef = random();

    // Register SIGCHLD signal handler
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    // Create the Probation Timer.
    ProbationTimer = le_timer_Create("Probation");
    le_timer_SetHandler(ProbationTimer, HandleProbationExpiry);
    le_timer_SetMsInterval(ProbationTimer, GetProbationPeriod());

    // Make sure we can set file permissions properly.
    umask(0);

    // If a system update needs finishing, finish it now.
    FinishSystemUpdate();

    // Make sure the users and groups are set up correctly for the apps we have installed
    // in the current system.  We may have updated or rolled-back to a different system with
    // different apps than we had last time we ran.
    UpdateUsersAndGroups();

    // If the current system is "good", go into the IDLE state,
    if (system_Status() == SYS_GOOD)
    {
        State = STATE_IDLE;

        LE_INFO("Current system is 'good'.");
    }
    // Otherwise, this system is on probation.
    else
    {
        StartProbation();

        LE_INFO("System on probation (timer started).");
    }

    // Make sure that we can report app install events.
    instStat_Init();

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
