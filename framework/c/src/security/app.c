//--------------------------------------------------------------------------------------------------
/** @file app.c
 *
 * This is the application class that references applications the Supervisor creates/starts/etc.
 * This application class contains all the processes that belong to this application.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "watchdogAction.h"
#include "app.h"
#include "limit.h"
#include "proc.h"
#include "user.h"
#include "le_cfg_interface.h"
#include "sandbox.h"
#include "resourceLimits.h"


//--------------------------------------------------------------------------------------------------
/**
 * The location where all applications are installed.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_INSTALL_DIR                "/opt/legato/apps"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that specifies whether the app should be in a sandbox.
 *
 * If this entry in the config tree is missing or empty the application will be sandboxed.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SANDBOXED                              "sandboxed"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's supplementary groups list.
 *
 * Supplementary groups list is only available for non-sandboxed apps.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_GROUPS                                 "groups"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of processes for the application.
 *
 * If this entry in the config tree is missing or empty the application will not be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PROC_LIST                              "procs"


//--------------------------------------------------------------------------------------------------
/**
 * The application object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct app_Ref
{
    char*           name;                               // The name of the application.
    char            cfgPathRoot[LIMIT_MAX_PATH_BYTES];  // Our path in the config tree.
    bool            sandboxed;                          // true if this is a sandboxed app.
    char            installPath[LIMIT_MAX_PATH_BYTES];  // The app's install directory path.
    char            sandboxPath[LIMIT_MAX_PATH_BYTES];  // The app's sandbox path.
    char            homeDirPath[LIMIT_MAX_PATH_BYTES];  // Home directory path to start procs in.
    uid_t           uid;                // The user ID for this application.
    gid_t           gid;                // The group ID for this application.
    gid_t           supplementGids[LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS];  // List of supplementary
                                                                         // group IDs.
    size_t          numSupplementGids;  // The number of supplementary groups for this app.
    app_State_t     state;              // The applications current state.
    le_dls_List_t   procs;              // The list of processes in this application.
}
App_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for application objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppPool;

//--------------------------------------------------------------------------------------------------
/**
 * Application object reference.  Incomplete type so that we can have the application object
 * reference the AppStopHandler_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _procObjRef* ProcObjRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for process stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*ProcStopHandler_t)
(
    app_Ref_t appRef,                   ///< [IN] The application containing the stopped process
    proc_Ref_t procRef                  ///< [IN] The stopped process
);

//--------------------------------------------------------------------------------------------------
/**
 * The process object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _procObjRef
{
    proc_Ref_t      procRef;        // The process reference.
    ProcStopHandler_t stopHandler;    // Handler function that gets called when this process stops.
    le_dls_Link_t   link;           // The link in the application's list of processes.
}
ProcObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for process objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * The file that stores the application reboot fault record.  When the system reboots due to an
 * application fault the applications and process names are stored here.
 */
//--------------------------------------------------------------------------------------------------
#define REBOOT_FAULT_RECORD                 "/opt/legato/appRebootFault"


//--------------------------------------------------------------------------------------------------
/**
 * The fault limits.
 *
 * @todo Put in the config tree so that it can be configured.
 */
//--------------------------------------------------------------------------------------------------
#define FAULT_LIMIT_INTERVAL_RESTART                1    // in seconds
#define FAULT_LIMIT_INTERVAL_RESTART_APP            3    // in seconds
#define FAULT_LIMIT_INTERVAL_REBOOT                 120  // in seconds


//--------------------------------------------------------------------------------------------------
/**
 * The reboot fault timer handler.  When this expires we delete the reboot fault record so that
 * reboot faults will reach the fault limit only if there is a fault that reboots the system before
 * this timer expires.
 */
//--------------------------------------------------------------------------------------------------
static void RebootFaultTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    if ( (unlink(REBOOT_FAULT_RECORD) == -1) && (errno != ENOENT) )
    {
        LE_ERROR("Could not delete reboot fault record.  %m.  This could result in the fault limit \
being reached incorrectly when a process faults and resets the system.")
    }

    le_timer_Delete(timerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the application system.
 */
//--------------------------------------------------------------------------------------------------
void app_Init
(
    void
)
{
    AppPool = le_mem_CreatePool("Apps", sizeof(App_t));
    ProcObjPool = le_mem_CreatePool("ProcObj", sizeof(ProcObj_t));

    // Start the reboot fault timer.
    le_timer_Ref_t rebootFaultTimer = le_timer_Create("RebootFault");
    le_clk_Time_t rebootFaultInterval = {.sec = FAULT_LIMIT_INTERVAL_REBOOT};

    if ( (le_timer_SetHandler(rebootFaultTimer, RebootFaultTimerHandler) != LE_OK) ||
         (le_timer_SetInterval(rebootFaultTimer, rebootFaultInterval) != LE_OK) ||
         (le_timer_Start(rebootFaultTimer) != LE_OK) )
    {
        LE_ERROR("Could not start the reboot fault timer.  This could result in the fault limit \
being reached incorrectly when a process faults and resets the system.");
    }

    proc_Init();
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a home directory for a specific user/app.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateHomeDir
(
    app_Ref_t appRef        // The app to create the directory for.
)
{
    // Create the "/home" directory if it doesn't exist yet.
    if (le_dir_Make("/home", S_IRWXU | S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH) == LE_FAULT)
    {
        LE_ERROR("Could not create '/home' directory.");
        return LE_FAULT;
    }

    // Create the app's home directory.
    if (le_dir_Make(appRef->homeDirPath, S_IRWXU | S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH) == LE_FAULT)
    {
        LE_ERROR("Could not create home directory '%s'.  Application '%s' cannot be started.",
                 appRef->homeDirPath,
                 appRef->name);
        return LE_FAULT;
    }

    // Set the owner of this folder to the application's user.
    if (chown(appRef->homeDirPath, appRef->uid, appRef->gid) != 0)
    {
        LE_ERROR("Could not set ownership of folder '%s' to uid %d.",
                 appRef->homeDirPath,
                 appRef->uid);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the supplementary groups for an application.
 *
 * @todo Move creation of the groups to the installer.  Make this function just read the groups
 *       list into the app object.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t CreateSupplementaryGroups
(
    app_Ref_t appRef        // The app to create groups for.
)
{
    // Get an iterator to the supplementary groups list in the config.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    le_cfg_GoToNode(cfgIter, CFG_NODE_GROUPS);

    if (le_cfg_GoToFirstChild(cfgIter) != LE_OK)
    {
        LE_DEBUG("No supplementary groups for app '%s'.", appRef->name);
        le_cfg_CancelTxn(cfgIter);

        return LE_OK;
    }

    // Read the supplementary group names from the config.
    size_t i;
    for (i = 0; i < LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS; i++)
    {
        // Read the supplementary group name from the config.
        char groupName[LIMIT_MAX_USER_NAME_BYTES];
        if (le_cfg_GetNodeName(cfgIter, "", groupName, sizeof(groupName)) != LE_OK)
        {
            LE_ERROR("Could not read supplementary group for app '%s'.", appRef->name);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }

        // Create the group.
        gid_t gid;
        if (user_CreateGroup(groupName, &gid) == LE_FAULT)
        {
            LE_ERROR("Could not create supplementary group '%s'.", groupName);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }

        // Store the group id in the user's buffer.
        appRef->supplementGids[i] = gid;

        // Go to the next group.
        if (le_cfg_GoToNextSibling(cfgIter) != LE_OK)
        {
            break;
        }
        else if (i >= LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS - 1)
        {
            LE_ERROR("Too many supplementary groups for app '%s'.", appRef->name);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }
    }

    appRef->numSupplementGids = i + 1;

    le_cfg_CancelTxn(cfgIter);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates the user and groups in the /etc/passwd and /etc/groups files for an application.  This
 * function sets the uid and primary gid for the appRef and also populates the appRef's
 * supplementary groups list for non-sandboxed apps.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateUserAndGroups
(
    app_Ref_t appRef        // The app to create user and groups for.
)
{
    // Generate a unique home directory path for the application.
    if ( snprintf(appRef->homeDirPath,
                  sizeof(appRef->homeDirPath),
                  "/home/app%s",
                  appRef->name)
        >= sizeof(appRef->homeDirPath))
    {
        LE_ERROR("Home directory path too long (would be truncated to '%s'). "
                 "Application '%s' cannot be started.",
                 appRef->homeDirPath,
                 appRef->name);

        return LE_FAULT;
    }

    // For sandboxed apps,
    if (appRef->sandboxed)
    {
        // Compute the unique user name for the application.
        char username[LIMIT_MAX_USER_NAME_BYTES];

        if (user_AppNameToUserName(appRef->name, username, sizeof(username)) != LE_OK)
        {
            LE_ERROR("The user name '%s' is too long.", username);
            return LE_FAULT;
        }

        // Get the user ID and primary group ID for this app.
        if (user_GetIDs(username, &(appRef->uid), &(appRef->gid)) != LE_OK)
        {
            LE_ERROR("Could not get uid and gid for user '%s'.", username);
            return LE_FAULT;
        }

        // Create the supplementary groups...
        return CreateSupplementaryGroups(appRef);
    }
    // For unsandboxed apps,
    else
    {
        // The user and group will be "root" (0).
        appRef->uid = 0;
        appRef->gid = 0;

        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an application object.
 *
 * @note
 *      The name of the application is the node name (last part) of the cfgPathRootPtr.
 *
 * @return
 *      A reference to the application object if success.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
app_Ref_t app_Create
(
    const char* cfgPathRootPtr      ///< [IN] The path in the config tree for this application.
)
{
    // Create a new app object.
    App_t* appPtr = le_mem_ForceAlloc(AppPool);

    // Save the config path.
    if (le_utf8_Copy(appPtr->cfgPathRoot, cfgPathRootPtr, sizeof(appPtr->cfgPathRoot), NULL) != LE_OK)
    {
        LE_ERROR("Config path '%s' is too long.", cfgPathRootPtr);

        le_mem_Release(appPtr);
        return NULL;
    }

    // Store the app name.
    appPtr->name = le_path_GetBasenamePtr(appPtr->cfgPathRoot, "/");

    // Initialize the other parameters.
    appPtr->procs = LE_DLS_LIST_INIT;
    appPtr->state = APP_STATE_STOPPED;

    // Get a config iterator for this app.
    le_cfg_IteratorRef_t cfgIterator = le_cfg_CreateReadTxn(appPtr->cfgPathRoot);

    // See if this is a sandboxed app.
    appPtr->sandboxed = le_cfg_GetBool(cfgIterator, CFG_NODE_SANDBOXED, true);

    // @todo: Create the user and all the groups for this app.  This function has a side affect
    //        where it populates the app's supplementary groups list and sets the uid and the
    //        primary gid.  This behaviour will be changed when the create user functionality is
    //        moved to the app installer.
    if (CreateUserAndGroups(appPtr) != LE_OK)
    {
        le_mem_Release(appPtr);
        le_cfg_CancelTxn(cfgIterator);
        return NULL;
    }

    // Get the app's install directory path.
    appPtr->installPath[0] = '\0';
    if (LE_OK != le_path_Concat("/",
                                appPtr->installPath,
                                sizeof(appPtr->installPath),
                                APPS_INSTALL_DIR,
                                appPtr->name,
                                NULL))
    {
        LE_ERROR("Install directory path '%s' is too long.  Application '%s' cannot be started.",
                 appPtr->installPath,
                 appPtr->name);

        le_mem_Release(appPtr);
        le_cfg_CancelTxn(cfgIterator);
        return NULL;
    }

    // Get the app's sandbox path.
    if (appPtr->sandboxed)
    {
        if (sandbox_GetPath(appPtr->name, appPtr->sandboxPath, sizeof(appPtr->sandboxPath)) != LE_OK)
        {
            LE_ERROR("The application's sandbox path '%s' is too long.  Application '%s' cannot be started.",
                     appPtr->sandboxPath, appPtr->name);

            le_mem_Release(appPtr);
            le_cfg_CancelTxn(cfgIterator);
            return NULL;
        }
    }
    else
    {
        appPtr->sandboxPath[0] = '\0';
    }

    // Move the config iterator to the procs list for this app.
    le_cfg_GoToNode(cfgIterator, CFG_NODE_PROC_LIST);

    // Read the list of processes for this application from the config tree.
    if (le_cfg_GoToFirstChild(cfgIterator) == LE_OK)
    {
        do
        {
            // Get the process's config path.
            char procCfgPath[LIMIT_MAX_PATH_BYTES];

            if (le_cfg_GetPath(cfgIterator, "", procCfgPath, sizeof(procCfgPath)) == LE_OVERFLOW)
            {
                LE_ERROR("Internal path buffer too small.");
                app_Delete(appPtr);
                le_cfg_CancelTxn(cfgIterator);
                return NULL;
            }

            // Strip off the trailing '/'.
            size_t lastIndex = le_utf8_NumBytes(procCfgPath) - 1;

            if (procCfgPath[lastIndex] == '/')
            {
                procCfgPath[lastIndex] = '\0';
            }

            // Create the process.
            proc_Ref_t procPtr;
            if ((procPtr = proc_Create(procCfgPath, appPtr->name)) == NULL)
            {
                app_Delete(appPtr);
                le_cfg_CancelTxn(cfgIterator);
                return NULL;
            }

            // Add the process to the app's process list.
            ProcObj_t* procObjPtr = le_mem_ForceAlloc(ProcObjPool);
            procObjPtr->procRef = procPtr;
            procObjPtr->stopHandler = NULL;
            procObjPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(appPtr->procs), &(procObjPtr->link));
        }
        while (le_cfg_GoToNextSibling(cfgIterator) == LE_OK);
    }

    le_cfg_CancelTxn(cfgIterator);

    return appPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an application.  The application must be stopped before it is deleted.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void app_Delete
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to delete.
)
{
    // Pop all the processes off the app's list and free them.
    le_dls_Link_t* procLinkPtr = le_dls_Pop(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcObj_t* procPtr = CONTAINER_OF(procLinkPtr, ProcObj_t, link);

        proc_Delete(procPtr->procRef);
        le_mem_Release(procPtr);

        procLinkPtr = le_dls_Pop(&(appRef->procs));
    }

    // Relesase app.
    le_mem_Release(appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_Start
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to start.
)
{
    if (appRef->state == APP_STATE_RUNNING)
    {
        LE_ERROR("Application '%s' is already running.", appRef->name);

        return LE_FAULT;
    }

    // If a sandboxed app,
    if (appRef->sandboxed)
    {
        // Create the sandboxed area.
        if (sandbox_Setup(appRef) != LE_OK)
        {
            LE_ERROR("Could not create sandbox for application '%s'.  This application cannot be started.",
                     appRef->name);

            return LE_FAULT;
        }
    }
    else
    {
        // Create the app's home directory.
        if (CreateHomeDir(appRef) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    // Set the resource limit for this application.
    if (resLim_SetAppLimits(appRef) != LE_OK)
    {
        LE_ERROR("Could not set application resource limits.  Application %s cannot be started.",
                 appRef->name);
        return LE_FAULT;
    }

    // Start all the processes in the application.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcObj_t* procPtr = CONTAINER_OF(procLinkPtr, ProcObj_t, link);

        le_result_t result;

        if (appRef->sandboxed)
        {
            result = proc_StartInSandbox(procPtr->procRef,
                                         appRef->homeDirPath,
                                         appRef->uid,
                                         appRef->gid,
                                         appRef->supplementGids,
                                         appRef->numSupplementGids,
                                         appRef->sandboxPath);
        }
        else
        {
            result = proc_Start(procPtr->procRef, appRef->homeDirPath);
        }

        if (result != LE_OK)
        {
            LE_ERROR("Could not start all application processes.  Stopping the application '%s'.",
                     appRef->name);

            app_Stop(appRef);

            return LE_FAULT;
        }

        // Get the next process.
        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    appRef->state = APP_STATE_RUNNING;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Kills all the processes that have this application's user ID.
 *
 * Does nothing for unsandboxed apps (which run as root).
 *
 * @todo    Use process groups to support killing of unsandboxed apps' processes.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t KillAppProcs
(
    app_Ref_t appRef    ///< [IN] Reference to the application whose processes should be killed.
)
{
    // Don't do this if the app is running as root.
    if (appRef->uid == 0)
    {
        LE_INFO("App '%s' runs as root.  Can't kill all processes running as root.", appRef->name);
        return LE_OK;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        LE_ERROR("Failed to fork child process.  %m.");
        return LE_FAULT;
    }

    // The child will kill all the processes.
    if (pid == 0)
    {
        // Set our uid to match the uid of the user that we want to kill all processes for.
        LE_FATAL_IF(setuid(appRef->uid) == -1, "Failed to set the uid.  %m.");

        // Send a signal to terminate all processes that share our uid.
        LE_FATAL_IF(kill(-1, SIGKILL) == -1, "Failed to send kill signal.  %m.");

        // Exit our process.
        exit(EXIT_SUCCESS);
    }

    // The parent waits for the child to terminate.
    int status;
    int r;

    do
    {
        r = waitpid(pid, &status, 0);
    } while ( (r == -1) && (errno == EINTR) );

    if (r == -1)
    {
        LE_ERROR("Waiting for child process failed.  %m.");
        return LE_FAULT;
    }

    // Check the return code of the child process.
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_SUCCESS)
        {
            return LE_OK;
        }
        else
        {
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Child process exited unexpectedly.");
        return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleans up a stopped application's resources ie. sandbox, resource limits, etc.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupApp
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Remove the sanbox.
    if (appRef->sandboxed)
    {
        if (sandbox_Remove(appRef) != LE_OK)
        {
            LE_CRIT("Could not remove sandbox for application '%s'.", appRef->name);
        }
    }

    // Remove the resource limits.
    resLim_CleanupApp(appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an application.  This is an asynchronous function call that returns immediately but
 * the application may not stop right away.  Check the application's state with app_GetState() to
 * see when the application actually stops.
 */
//--------------------------------------------------------------------------------------------------
void app_Stop
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
)
{
    if (appRef->state == APP_STATE_STOPPED)
    {
        LE_ERROR("Application '%s' is already stopped.", appRef->name);
        return;
    }

    // Stop all processes in our list.
    bool hasProcess = false;
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        hasProcess = true;

        ProcObj_t* procObjPtr = CONTAINER_OF(procLinkPtr, ProcObj_t, link);

        if (proc_GetState(procObjPtr->procRef) != PROC_STATE_STOPPED)
        {
            procObjPtr->stopHandler = NULL;
            proc_Stop(procObjPtr->procRef);
        }

        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    // Kill all user apps in case there were any forked processes in the app.
    // NOTE: There is a race condition here because the processes that are killed may take some time
    //       to die but we have no way of knowing when they actually die.  This may cause problems
    //       trying to cleanup system resources such as unmounting files used by the processes,
    //       deleting cgroups, etc.
    //       It is possible to poll /proc for all processes with the same application's username but
    //       this seems fairly heavy weight and is therefore left for future enhancements.
    if (KillAppProcs(appRef) != LE_OK)
    {
        LE_ERROR("Could not kill processes for application '%s'.", appRef->name);
    }

    if (!hasProcess)
    {
        // There are no more processes that we are aware of so we can only assume the app has stopped.
        LE_INFO("app '%s' has stopped.", appRef->name);

        // Note the application is cleaned up here so if the app is restarted it will apply the new
        // config settings if the config has changed.
        CleanupApp(appRef);

        appRef->state = APP_STATE_STOPPED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's state.
 *
 * @return
 *      The application's state.
 */
//--------------------------------------------------------------------------------------------------
app_State_t app_GetState
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
)
{
    return appRef->state;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's name.
 *
 * @return
 *      The application's name.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetName
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's UID.
 *
 * @return
 *      The application's UID.
 */
//--------------------------------------------------------------------------------------------------
uid_t app_GetUid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return appRef->uid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's GID.
 *
 * @return
 *      The application's GID.
 */
//--------------------------------------------------------------------------------------------------
gid_t app_GetGid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return appRef->gid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's installation directory path.
 *
 * @return
 *      The application's install directory path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetInstallDirPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->installPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's sandbox path.
 *
 * @return
 *      The application's sandbox path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetSandboxPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->sandboxPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's home directory path.
 *
 * If the app is sandboxed, this is relative to the sandbox's root directory.
 *
 * @return A pointer to the path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetHomeDirPath
(
    app_Ref_t appRef
)
{
    return appRef->homeDirPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's configuration path.
 *
 * @return
 *      The application's configuration path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetConfigPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->cfgPathRoot;
}


//--------------------------------------------------------------------------------------------------
/**
 * Finds a process object for the app.
 *
 * @return
 *      The process object reference if successful.
 *      NULL if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static ProcObjRef_t FindProcObjectRef
(
    app_Ref_t appRef,               ///< [IN] The application to search in.
    pid_t pid                       ///< [IN] The pid to search for.
)
{
    // Find the process in the app's list.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcObj_t* procObjPtr = CONTAINER_OF(procLinkPtr, ProcObj_t, link);

        if (proc_GetPID(procObjPtr->procRef) == pid)
        {
            return procObjPtr;
        }

        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write the reboot fault record for the application/process that experienced the fault and requires
 * a system reboot.
 *
 * @todo Write the record fault into the config tree when it is available.  This is just a temporary
 *       solution because the current config tree is non-persistent.
 */
//--------------------------------------------------------------------------------------------------
static void WriteRebootFaultRec
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    proc_Ref_t procRef                  ///< [IN] The process reference.
)
{
    // @note Don't really need to lock this file as no-one else really uses.  Using the le_flock
    //       API just cause it's easier to use then open() and this is a temporary location for the
    //       record fault anyways.
    int fd = le_flock_Create(REBOOT_FAULT_RECORD, LE_FLOCK_WRITE, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);

    if (fd < 0)
    {
        LE_ERROR("Could not create reboot fault record.  The reboot fault limit will not \
be enforced correctly.");
        return;
    }

    char faultStr[LIMIT_MAX_PATH_BYTES];

    int faultStrSize = snprintf(faultStr, sizeof(faultStr), "%s/%s", appRef->name,
                                proc_GetName(procRef)) + 1;
    LE_ASSERT(faultStrSize <= sizeof(faultStr));

    int result;
    do
    {
        result = write(fd, faultStr, faultStrSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    if (result == -1)
    {
        LE_ERROR("Could not write reboot fault record.  %m.");
    }
    else if (result != faultStrSize)
    {
        LE_ERROR("Could not write reboot fault record.  The reboot fault limit will not \
be enforced correctly.");
    }

    le_flock_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the reboot fault record was created by the specified application/process.
 *
 * @return
 *      true if the reboot fault record was created by the specified app/process.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool isRebootFaultRecFor
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    proc_Ref_t procRef                  ///< [IN] The process reference.
)
{
    // This file does not really need to be locked as no one else uses it.  Also this should go into
    // the config tree when the config tree is available.
    int fd = le_flock_Open(REBOOT_FAULT_RECORD, LE_FLOCK_READ);

    if (fd == LE_NOT_FOUND)
    {
        return false;
    }
    else if (fd == LE_FAULT)
    {
        LE_ERROR("Could not open reboot fault record.  The reboot fault limit will not \
be enforced correctly.");
        return false;
    }

    // Read the record.
    char faultRec[LIMIT_MAX_PATH_BYTES] = {0};
    ssize_t count = read(fd, faultRec, sizeof(faultRec));

    le_flock_Close(fd);

    if (count == -1)
    {
        LE_ERROR("Could not read reboot fault record.  %m.  The reboot fault limit will not \
be enforced correctly.");
    }
    else if (count >= sizeof(faultRec))
    {
        LE_ERROR("Could not read reboot fault record.  The reboot fault limit will not \
be enforced correctly.");
    }
    else
    {
        // Add a null to the string read.
        faultRec[count] = '\0';

        // See if the reboot record is for this app/process.
        char faultStr[LIMIT_MAX_PATH_BYTES];
        LE_ASSERT(snprintf(faultStr, sizeof(faultStr), "%s/%s", appRef->name, proc_GetName(procRef)) <
                  sizeof(faultStr));

        if (strcmp(faultRec, faultStr) == 0)
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the fault limit for this process has been reached.  The fault limit is reached
 * when there is more than one fault within the fault limit interval.
 *
 * @return
 *      true if the fault limit has been reached.
 *      false if the fault limit has not been reached.
 */
//--------------------------------------------------------------------------------------------------
static bool ReachedFaultLimit
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    proc_Ref_t procRef,                 ///< [IN] The process reference.
    proc_FaultAction_t currFaultAction, ///< [IN] The process's current fault action.
    time_t prevFaultTime                ///< [IN] Time of the previous fault.
)
{
    switch (currFaultAction)
    {
        case PROC_FAULT_ACTION_RESTART:
            if ( (proc_GetFaultTime(procRef) != 0) &&
                 (proc_GetFaultTime(procRef) - prevFaultTime <= FAULT_LIMIT_INTERVAL_RESTART) )
            {
                return true;
            }
            break;

        case PROC_FAULT_ACTION_RESTART_APP:
            if ( (proc_GetFaultTime(procRef) != 0) &&
                 (proc_GetFaultTime(procRef) - prevFaultTime <= FAULT_LIMIT_INTERVAL_RESTART_APP) )
            {
                return true;
            }
            break;

        case PROC_FAULT_ACTION_REBOOT:
            return isRebootFaultRecFor(appRef, procRef);

        default:
            // Fault limits do not apply to the other fault actions.
            return false;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process in an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartProc
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    proc_Ref_t procRef                  ///< [IN] The process to start.
)
{
    le_result_t result;

    if (appRef->sandboxed)
    {
        result = proc_StartInSandbox(procRef,
                                     appRef->homeDirPath,
                                     appRef->uid,
                                     appRef->gid,
                                     appRef->supplementGids,
                                     appRef->numSupplementGids,
                                     appRef->sandboxPath);
    }
    else
    {
        result = proc_Start(procRef, appRef->homeDirPath);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the application has any processes running.
 *
 * @note This only applies to child processes.  Forked processes in the application are not
 *       monitored.
 *
 * @return
 *      true if there is atleast one running process for the application.
 *      false if there are no running processes for the application.
 */
//--------------------------------------------------------------------------------------------------
static bool HasRunningProc
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcObj_t* procObjPtr = CONTAINER_OF(procLinkPtr, ProcObj_t, link);

        if (proc_GetState(procObjPtr->procRef) != PROC_STATE_STOPPED)
        {
            return true;
        }

        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when the watchdog expires for a process that belongs to the
 * specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the procPid was not found for the specified app.
 *
 *      The watchdog action passed in will be set to the action that should be taken for
 *      this process or one of the following:
 *          WATCHDOG_ACTION_NOT_FOUND - no action was configured for this process
 *          WATCHDOG_ACTION_ERROR     - the action could not be read or is unknown
 *          WATCHDOG_ACTION_HANDLED   - no further action is required, it is already handled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_WatchdogTimeoutHandler
(
    app_Ref_t appRef,                                 ///< [IN] The application reference.
    pid_t procPid,                                    ///< [IN] The pid of the process that changed
                                                      ///< state.
    wdog_action_WatchdogAction_t* watchdogActionPtr   ///< [OUT] The fault action that should be
                                                      ///< taken.
)
{
    LE_FATAL_IF(appRef == NULL, "appRef is NULL");

    ProcObjRef_t procObjectRef = FindProcObjectRef(appRef, procPid);

    if (procObjectRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    proc_Ref_t procRef = procObjectRef->procRef;

    // Get the current process fault action.
    wdog_action_WatchdogAction_t watchdogAction = proc_GetWatchdogAction(procRef);

    // If WATCHDOG_ACTION_ERROR, we have reported the error already in proc. Let's give ourselves a
    // second chance and see if we can find a good value at app level.
    if (WATCHDOG_ACTION_NOT_FOUND == watchdogAction || WATCHDOG_ACTION_ERROR == watchdogAction)
    {
        // No action was defined for the proc. See if there is one for the app.
        // Read the app's watchdog action from the config tree.
        le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

        char watchdogActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
        le_result_t result = le_cfg_GetString(appCfg, wdog_action_GetConfigNode(),
                watchdogActionStr, sizeof(watchdogActionStr), "");

        le_cfg_CancelTxn(appCfg);

        // Set the watchdog action based on the watchdog action string.
        if (result == LE_OK)
        {
            LE_DEBUG("%s watchdogAction '%s' in app section", appRef->name, watchdogActionStr);
            watchdogAction = wdog_action_EnumFromString(watchdogActionStr);
            if (WATCHDOG_ACTION_ERROR == watchdogAction)
            {
                LE_WARN("%s watchdog Action %s unknown", appRef->name, watchdogActionStr);
            }
        }
        else
        {
            LE_CRIT("Watchdog action string for application '%s' is too long.",
                    appRef->name);
            watchdogAction = WATCHDOG_ACTION_ERROR;
        }
    }

    // Set the action pointer to error. If it's still error when we leave here something
    // has gone wrong!!
    *watchdogActionPtr = WATCHDOG_ACTION_ERROR;

    // TODO: do watchdog timeouts count toward this total?
    switch (watchdogAction)
    {
        case WATCHDOG_ACTION_NOT_FOUND:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out but there is no \
policy. The process will be restarted by default.", proc_GetName(procRef), appRef->name);

            // Set the process to restart when it stops then stop it.
            procObjectRef->stopHandler = StartProc;
            proc_Stop(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_IGNORE:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be ignored \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_STOP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be terminated \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);
            proc_Stop(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_RESTART:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be restarted \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            // Set the process to restart when it stops then stop it.
            procObjectRef->stopHandler = StartProc;
            proc_Stop(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_RESTART_APP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and the app will be \
restarted in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_STOP_APP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and the app will \
be stopped in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_REBOOT:
            LE_EMERG("The watchdog for process '%s' in app '%s' has timed out and the system will \
now be rebooted in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_ERROR:
            // something went wrong reading the action.
            LE_CRIT("An error occurred trying to find the watchdog action for process '%s' in \
application '%s'. Restarting app by default.", proc_GetName(procRef), appRef->name);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_HANDLED:
            *watchdogActionPtr = watchdogAction;
            break;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for a process that belongs to the
 * specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the procPid was not found for the specified app.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SigChildHandler
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    pid_t procPid,                      ///< [IN] The pid of the process that changed state.
    int procExitStatus,                 ///< [IN] The return status of the process given by wait().
    app_FaultAction_t* faultActionPtr   ///< [OUT] The fault action that should be taken.
)
{
    ProcObjRef_t procObjRef = FindProcObjectRef(appRef, procPid);

    if (procObjRef == NULL)
    {
        return LE_NOT_FOUND;
    }

    proc_Ref_t procRef = procObjRef->procRef;

    // Remember the previous fault time.
    time_t prevFaultTime = proc_GetFaultTime(procRef);

    // Get the current process fault action.
    proc_FaultAction_t procFaultAction = proc_SigChildHandler(procRef, procExitStatus);

    // Determine the fault action for the application.
    *faultActionPtr = APP_FAULT_ACTION_IGNORE;

    if (ReachedFaultLimit(appRef, procRef, procFaultAction, prevFaultTime))
    {
        LE_CRIT("The process '%s' in application '%s' has reached the fault limit so the \
application will be stopped instead of performing the configured fault action.",
                proc_GetName(procRef), appRef->name);

        *faultActionPtr = APP_FAULT_ACTION_STOP_APP;
    }
    else
    {
        switch (procFaultAction)
        {
            case PROC_FAULT_ACTION_NO_FAULT:
                // This is something that happens if we have deliberately killed the proc or if we
                // paused or resumed the proc. If the wdog stopped it then we may get here with a
                // stop handler attached ( to call StartProc).
                if (procObjRef->stopHandler)
                {
                    if (procObjRef->stopHandler(appRef, procRef) != LE_OK)
                    {
                        LE_ERROR("Watchdog could not restart process '%s' in application '%s'.",
                                proc_GetName(procRef), appRef->name);

                        *faultActionPtr = APP_FAULT_ACTION_STOP_APP;
                    }
                }
                break;

            case PROC_FAULT_ACTION_IGNORE:
                LE_CRIT("The process '%s' in app '%s' has faulted and will be ignored in \
accordance with its fault policy.", proc_GetName(procRef), appRef->name);
                break;

            case PROC_FAULT_ACTION_RESTART:
                LE_CRIT("The process '%s' in app '%s' has faulted and will be restarted in \
accordance with its fault policy.", proc_GetName(procRef), appRef->name);

                // Restart the process now.
                if (StartProc(appRef, procRef) != LE_OK)
                {
                    LE_ERROR("Could not restart process '%s' in application '%s'.",
                             proc_GetName(procRef), appRef->name);

                    *faultActionPtr = APP_FAULT_ACTION_STOP_APP;
                }
                break;

            case PROC_FAULT_ACTION_RESTART_APP:
                LE_CRIT("The process '%s' in app '%s' has faulted and the app will be restarted \
in accordance with its fault policy.", proc_GetName(procRef), appRef->name);

                *faultActionPtr = APP_FAULT_ACTION_RESTART_APP;
                break;

            case PROC_FAULT_ACTION_STOP_APP:
                LE_CRIT("The process '%s' in app '%s' has faulted and the app will be stopped \
in accordance with its fault policy.", proc_GetName(procRef), appRef->name);

                *faultActionPtr = APP_FAULT_ACTION_STOP_APP;
                break;

            case PROC_FAULT_ACTION_REBOOT:
                LE_EMERG("The process '%s' in app '%s' has faulted and the system will now be \
rebooted in accordance with its fault policy.", proc_GetName(procRef), appRef->name);

                WriteRebootFaultRec(appRef, procRef);

                *faultActionPtr = APP_FAULT_ACTION_REBOOT;
                break;
        }
    }

    if (!HasRunningProc(appRef))
    {
        LE_INFO("app '%s' has stopped.", appRef->name);

        // Note the application is cleaned up here so if the app is restarted it will apply the new
        // config settings if the config has changed.
        CleanupApp(appRef);

        appRef->state = APP_STATE_STOPPED;
    }

    return LE_OK;
}

