/** @file supervisor.c
 *
 *  - @ref c_sup_intro
 *  - @ref c_sup_sysProcs
 *  - @ref c_sup_appStarts
 *  - @ref c_sup_sandboxedApps
 *  - @ref c_sup_nonSandboxedApps
 *  - @ref c_sup_appUsers
 *  - @ref c_sup_faultRecovery
 *  - @ref c_sup_faultLimits
 *  - @ref c_sup_singleInstance
 *  - @ref c_sup_configLayout
 *
 * @section c_sup_intro Supervisor
 *
 * The Legato Supervisor is a daemonized process that has root privileges.  It is the first Legato
 * process to start and is responsible for starting and monitoring the rest of the Legato runtime
 * system.
 *
 * @section c_sup_startup System Processes
 *
 * Besides the Supervisor the Legato runtime system consists of a number of system processes that
 * must be started before any applications are started.
 *
 * The system processes must be started in a specific order and must be given time to initialize
 * properly.
 *
 * After starting each system process the Supervisor waits for the system process to signal that it
 * is ready before continuing on to the next system process.  Only after all system processes have
 * been started and initialized will applications be started.  The assumption is made that system
 * processes are trusted and reliable.
 *
 * The system processes must be started in this order: Service Directory, Log Control Daemon,
 * Configuration Database.
 *
 * @todo Currently the list of system processes is stored in the file SYS_PROCS_CONFIG.  This list
 *       contains other system processes in addition to the Service Directory, Log Control Daemon
 *       and Configuration Database.  The additional system processes should probably be removed
 *       from this list and made into pre-installed unsandboxed apps.
 *
 *
 * @section c_sup_appStarts Starting Applications
 *
 * Installed applications may be configured to be started automatically or manually.  If an
 * application is configured to be started automatically the Supervisor starts the application on
 * start-up, after all system processes have been started.
 *
 * All applications can be stopped and started manually by requesting sending a request to the
 * Supervisor.  Note that only one instance of the application may be running at a time.
 *
 *
 * @section c_sup_sandboxedApps Sandboxed Applications
 *
 * An application can be configured to be either sandboxed or non-sandboxed.
 *
 * Sandboxed applications run in a chrooted environment and have no visiblity to the rest of the
 * system.  The procedure the Supervisor uses for starting a sandboxed app is:
 *
 *      1) Create the directory /tmp/Legato/sandboxes/appName.  This is the root of the sandbox.
 *      2) Mount a ramfs with a fixed size at the root of the sandbox.
 *      3) Create standard directories in the sandbox, such as /tmp, /home/appName, /dev, etc.
 *      4) Bind mount in standard files and devices into the sandbox, such as /dev/null, the Service
 *         Directory sockets, etc.
 *      5) Bind mount in all other required files into the sandbox specific to the app.
 *      6) Start all the application processes chrooted to the sandbox root and chdir to
 *         /tmp/Legato/sandboxes/appName/home/appName.
 *
 * @Note All sandboxes are created in /tmp so that nothing is persistent.
 *
 * When a sandboxed application is stopped:
 *
 *      1) All application processes are killed.
 *      2) All mounts are undone.
 *      3) Created directories are deleted.
 *
 * @todo Allow some way for sandboxed applications to write/read persistent information.
 *
 *
 * @section c_sup_nonSandboxedApps Non-Sandboxed Applications
 *
 * A non-sandboxed application is an application that runs in the main file system.  The Supervisor
 * uses this procedure to start a non-sandboxed application:
 *
 *      1) Create the directory /home/appName.
 *      2) Run application processes chdir to /home/appName.
 *
 * When a non-sandboxed application is stopped:
 *
 *      1) All application processes are killed.
 *
 * @Note The /home/appName directory is not cleaned up because there may be persistent files left in
 *       this directory that the app will need next time it starts.
 *
 * @todo Add capabilities to non-sandboxed applications.
 *
 *
 * @section c_sup_appUsers Application Users and Groups
 *
 * When an application is installed it is assigned a user name, user ID, primary group name and
 * primary group ID.  The user and primary group names are the same and are derived from the
 * application name.
 *
 * Additionally, non-sandboxed applications may have a list of supplementary groups.  If a
 * supplementary group does not already exist in the system the group is created.
 *
 * @Note An application's supplementary groups list is not stored in the system's /etc/group file
 *       because the supplementary groups are already stored in the config DB and added to the
 *       application's processes when the processes are started.
 *
 * When an application starts all the application's processes are given the application's user ID,
 * primary group ID and, if applicable, supplementary groups ID.
 *
 * @Note Currently an application's user and group(s) are not deleted when an application is
 *       uninstalled.  This is a security issue with non-sandboxed apps because if a different
 *       application is installed that has the same name as a previously installed application the
 *       new application will inherit all the file permissions of the previous application.  On the
 *       other hand if the user and group(s) are deleted a new application may reclaim the same UID
 *       and inherit permissions to files not intended for it.  So, we must give a warning if an
 *       application is installed with a user name that already exists.
 *
 * @todo Currently the Supervisor attempts to create the user each time an application is started.
 *       This task should be moved to the installer so that users and groups are created only during
 *       installation.
 *
 *
 * @section c_sup_faultRecovery Fault Recovery
 *
 * The Supervisor monitors all running application processes for faults.  A fault is when a process
 * terminates without returning EXIT_SUCCESS.  When the Supervisor detects a fault it will perform
 * the configured fault recovery action.
 *
 * The Supervisor does not monitor processes that it does not start.  Parent processes are
 * responsible for monitoring their children.  However, when the Supervisor terminates an
 * application the Supervisor will kill off all processes in the application whether it is a child
 * of the Supervisor or not.
 *
 *
 * @section c_sup_faultLimits Fault Limits
 *
 * To prevent a process that is continually faulting from continually consuming resources the
 * Supervisor imposes a fault limit on all processes in the system.  The fault limit is the minimum
 * time interval between two faults, ie. if more than one fault occurs within the fault limit time
 * interval then the fault limit is reached.  The fault limit may be different for each fault
 * action but they are applied to all application processes.
 *
 * If a process reaches the fault limit a critical message is logged and the application the process
 * belongs to is shutdown and no further fault recovery action is taken.
 *
 * The fault limits only prevent automatic recovery by the Supervisor, it does not prevent
 * applications from being restarted manually even after the fault limit is exceeded.
 *
 *
 * @section c_sup_singleInstance Single Instance
 *
 * The Supervisor uses a locked file to ensure that there is only one instance of the Supervisor
 * running.
 *
 *
 * @section c_sup_configLayout Application Configuration
 *
 * All application configuration settings are stored in the Legato Configuration Database.  The
 * database is structured as follows:
 *
 * apps
 *      appName1
 *          debug (true, false)
 *          sandboxed (true, false)
 *          defer launch (true, false)
 *          fileSystemSizeLimit (integer)
 *          totalPosixMsgQueueSizeLimit (integer)
 *          numProcessesLimit (integer)
 *          rtSignalQueueSizeLimit (integer)
 *          groups
 *              groupName0
 *              groupName1
 *              ...
 *              groupNameN
 *          files
 *              0
 *                  src (string)
 *                  dest (string)
 *              1
 *                  src (string)
 *                  dest (string)
 *              ...
 *              N
 *          procs
 *              procName1
 *                  virtualMemoryLimit (integer)
 *                  coreDumpFileSizeLimit (integer)
 *                  maxFileSizeLimit (integer)
 *                  memLockSizeLimit (integer)
 *                  numFileDescriptorsLimit (integer)
 *              priority (string)
 *              faultAction (string)
 *              args
 *                  0 (string) -> must contain the executable path relative to the sandbox root.
 *                  1 (string)
 *                  ...
 *                  N (string)
 *              envVars
 *                  varName0
 *                      varValue0 (string)
 *                  varName1
 *                      varValue1 (string)
 *                  ...
 *                  varNameN
 *                      varValueN (string)
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
#include "legato.h"
#include "limit.h"
#include "le_cfg_interface.h"
#include "user.h"
#include "app.h"
#include "proc.h"
#include "fileDescriptor.h"
#include "le_sup_server.h"
#include "config.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of all applications.
 *
 * If this entry in the config tree is missing or empty then no apps will be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_APPS_LIST                  "apps"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the applications defer launch value, used
 * to determine whether the application should be launched on system startup or if it should be
 * deferred for manual launch later.
 *
 * The defer value is either "yes" or "no".  If "yes" the application will be deferred and will not
 * be launched on startup.
 *
 * If this entry in the config tree is missing or is empty, "no" will be taken as the default
 * deferLaunch value.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_DEFER_LAUNCH                   "deferLaunch"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the configuration file that stores all system processes that the Supervisor must
 * start before any user applications.
 */
//--------------------------------------------------------------------------------------------------
#define SYS_PROCS_CONFIG                    "/tmp/LegatoConfigTree/sysProcs"


//--------------------------------------------------------------------------------------------------
/**
 * The file the Supervisor uses to ensure that only a single instance of the Supervisor is running.
 */
//--------------------------------------------------------------------------------------------------
#define SUPERVISOR_INSTANCE_FILE            LE_RUNTIME_DIR "supervisorInst"


//--------------------------------------------------------------------------------------------------
/**
 * Application object reference.  Incomplete type so that we can have the application object
 * reference the AppStopHandler_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _appObjRef* AppObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for application stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*AppStopHandler_t)
(
    AppObjRef_t appObjRef               // The app that stopped.
);


//--------------------------------------------------------------------------------------------------
/**
 * The application object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _appObjRef
{
    app_Ref_t              appRef;         // Reference to the application.
    AppStopHandler_t       stopHandler;    // Handler function that gets called when the app stops.
    le_sup_ServerCmdRef_t  stopCmdRef;     // Stores the reference to the command that requested
                                           // this app be stopped.  This reference must be sent in the
                                           // response to the stop app command.
    le_dls_Link_t          link;           // Link in the list of applications.
}
AppObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * System process object reference.  Incomplete type so that we can have the system process object
 * reference the SysProcStopHandler_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _sysProcObjRef* SysProcObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for system process stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*SysProcStopHandler_t)
(
    SysProcObjRef_t sysProcObjRef       // The system process that stopped.
);


//--------------------------------------------------------------------------------------------------
/**
 * The system process object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _sysProcObjRef
{
    char            name[LIMIT_MAX_PROCESS_NAME_BYTES];  // The name of the process.
    pid_t           pid;                // The pid of the process.
    SysProcStopHandler_t stopHandler;   // The handler to call when this system process stops.
    le_dls_Link_t   link;               // The link in the system process list.
}
SysProcObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for application objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for system process objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SysProcObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * List of all applications.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t AppsList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * List of all system processes.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t SysProcsList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * The command reference for the Stop Legato command.
 */
//--------------------------------------------------------------------------------------------------
static le_sup_ServerCmdRef_t StopLegatoCmdRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Local function Prototypes.
 */
//--------------------------------------------------------------------------------------------------
static void StopFramework(void);
static void StopSysProcs(void);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a string is empty.
 */
//--------------------------------------------------------------------------------------------------
static bool IsEmptyString
(
    const char* strPtr
)
{
    if (strPtr == NULL)
    {
        return true;
    }

    int i = 0;
    for (; strPtr[i] != '\0'; i++)
    {
        if (isspace(strPtr[i]) == 0)
        {
            return false;
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Daemonizes the calling process.
 */
//--------------------------------------------------------------------------------------------------
static void Daemonize(void)
{
    if (getppid() == 1)
    {
        // Already a daemon.
        return;
    }

    // Fork off the parent process.
    pid_t pid = fork();

    LE_FATAL_IF(pid < 0, "Failed to fork when daemonizing the supervisor.  %m.");

    // If we got a good PID, are the parent process.
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // Only the child gets here.

    // Start a new session and become the session leader, the process group leader which will free
    // us from any controlling terminals.
    LE_FATAL_IF(setsid() == -1, "Could not start a new session.  %m.");

    // Reset the file mode mask.
    umask(0);

    // Change the current working directory to the root filesystem, to ensure that it doesn't tie
    // up another filesystem and prevent it from being unmounted.
    LE_FATAL_IF(chdir("/") < 0, "Failed to set supervisor's working directory to root.  %m.");

    // Redirect standard fds to /dev/null.
    LE_FATAL_IF( (freopen("/dev/null", "w", stdout) == NULL) ||
                 (freopen("/dev/null", "w", stderr) == NULL) ||
                 (freopen("/dev/null", "r", stdin) == NULL),
                "Failed to redirect standard files to /dev/null.  %m.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete the application object from our list and free the memory.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAppObj
(
    AppObjRef_t appObjRef               // The app to delete.
)
{
    app_Delete(appObjRef->appRef);

    le_dls_Remove(&AppsList, &(appObjRef->link));
    le_mem_Release(appObjRef);

    LE_INFO("Application '%s' has stopped.", app_GetName(appObjRef->appRef));
}


//--------------------------------------------------------------------------------------------------
/**
 * Restarts the application.
 */
//--------------------------------------------------------------------------------------------------
static void RestartApp
(
    AppObjRef_t appObjRef               // The app to restart.
)
{
    // Always re-initialize the stop handler to just delete the app so that when a process dies in
    // the app that does not require a restart it will be handled properly.
    appObjRef->stopHandler = DeleteAppObj;

    // Restart the app.
    if (app_Start(appObjRef->appRef) == LE_OK)
    {
        LE_INFO("Application '%s' restarted.", app_GetName(appObjRef->appRef));
    }
    else
    {
        LE_CRIT("Could not restart application '%s'.", app_GetName(appObjRef->appRef));

        DeleteAppObj(appObjRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Responds to the stop app command.  Also deletes the application object for the application that
 * just stopped.
 */
//--------------------------------------------------------------------------------------------------
static void RespondToStopAppCmd
(
    AppObjRef_t appObjRef               // The app that stopped.
)
{
    // Save command reference for later use.
    void* cmdRef = appObjRef->stopCmdRef;

    // Perform the deletion.
    DeleteAppObj(appObjRef);

    // Respond to the requesting process.
    le_sup_StopAppRespond(cmdRef, LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the next running application.
 *
 * Deletes the current application object.  If no other applications are running stop the first
 * system process.
 */
//--------------------------------------------------------------------------------------------------
static void StopNextApp
(
    AppObjRef_t appObjRef               // The app that just stopped.
)
{
    // Perform the deletion.
    DeleteAppObj(appObjRef);

    // Continue the shutdown process.
    StopFramework();
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the system process object and logs an error message.
 */
//--------------------------------------------------------------------------------------------------
void DeleteSysProc
(
    SysProcObjRef_t sysProcObjRef           // The system process that stopped.
)
{
    // @todo Restart the framework instead of just giving a warning.
    LE_EMERG("System process '%s' has died.  Some services may not function correctly.",
             sysProcObjRef->name);

    // Delete the sys proc object.
    le_dls_Remove(&SysProcsList, &(sysProcObjRef->link));
    le_mem_Release(sysProcObjRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the next system process.
 *
 * Deletes the system process object that just stopped.
 */
//--------------------------------------------------------------------------------------------------
static void StopNextSysProc
(
    SysProcObjRef_t sysProcObjRef           // The system process that just stopped.
)
{
    // Delete the sys proc object.
    le_dls_Remove(&SysProcsList, &(sysProcObjRef->link));
    le_mem_Release(sysProcObjRef);

    // Continue to stop all other system processes.
    StopSysProcs();
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application object by name.
 *
 * @return
 *      A pointer to the application object if successful.
 *      NULL if the application is not found.
 */
//--------------------------------------------------------------------------------------------------
static AppObj_t* GetApp
(
    const char* appName     // The name of the application to get.
)
{
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

    while (appLinkPtr != NULL)
    {
        AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);

        if (strcmp(app_GetName(appPtr->appRef), appName) == 0)
        {
            return appPtr;
        }

        appLinkPtr = le_dls_PeekNext(&AppsList, appLinkPtr);
    }

    return NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * Launch an application.  Create the application object and starts all its processes.
 *
 * @return
 *      LE_OK if successfully launched the application.
 *      LE_DUPLICATE if the application is already running.
 *      LE_NOT_FOUND if the application is not installed.
 *      LE_FAULT if the application could not be launched.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LaunchApp
(
    const char* appNamePtr      // The name of the application to launch.
)
{
    // Check if the app already exists.
    if (GetApp(appNamePtr) != NULL)
    {
        LE_ERROR("Application '%s' is already running.", appNamePtr);
        return LE_DUPLICATE;
    }

    // Get the configuration path for this app.
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_NODE_APPS_LIST, appNamePtr);

    // Check that the app has a configuration value.
    le_cfg_iteratorRef_t appCfg = le_cfg_CreateReadTxn(configPath);

    if (le_cfg_IsEmpty(appCfg, ""))
    {
        LE_ERROR("Application '%s' is not installed and cannot run.", appNamePtr);
        le_cfg_DeleteIterator(appCfg);
        return LE_NOT_FOUND;
    }

    // Create the app object.
    app_Ref_t appRef = app_Create(configPath);

    if (appRef == NULL)
    {
        le_cfg_DeleteIterator(appCfg);
        return LE_FAULT;
    }

    AppObj_t* appPtr = le_mem_ForceAlloc(AppObjPool);

    appPtr->appRef = appRef;
    appPtr->link = LE_DLS_LINK_INIT;
    appPtr->stopHandler = DeleteAppObj;

    // Start the app.
    if (app_Start(appPtr->appRef) != LE_OK)
    {
        app_Delete(appPtr->appRef);
        le_mem_Release(appPtr);

        return LE_FAULT;
    }

    // @Note: We hang on to the the application config iterator till here to ensure the application
    // configuration does not change during the creation and starting of the application.
    le_cfg_DeleteIterator(appCfg);

    // Add the app to the list.
    le_dls_Queue(&AppsList, &(appPtr->link));

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called on system startup to launch all the applications found in the config tree that do not
 * specify that the Supervisor should defer their launch.
 */
//--------------------------------------------------------------------------------------------------
static void LaunchAllStartupApps
(
    void
)
{
    // Read the list of applications from the config tree.
    le_cfg_iteratorRef_t appCfg = le_cfg_CreateReadTxn(CFG_NODE_APPS_LIST);

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_WARN("No applications installed.");

        le_cfg_DeleteIterator(appCfg);
        return;
    }

    do
    {
        // Check the defer launch for this application.
        if (!le_cfg_GetBool(appCfg, CFG_NODE_DEFER_LAUNCH))
        {
            // Get the app name.
            char appName[LIMIT_MAX_APP_NAME_BYTES];
            le_cfg_GetNodeName(appCfg, appName, sizeof(appName));

            // Launch the application now.  No need to check the return code because there is
            // nothing we can do about errors.
            LaunchApp(appName);
        }
    }
    while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

    le_cfg_DeleteIterator(appCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the environment variable for a process from the list of environment variables in the
 * sysproc config.
 *
 * @todo This uses the old config tree but maybe this won't be needed at all in the future when the
 *       agent is no longer a sysproc
 */
//--------------------------------------------------------------------------------------------------
static void SetEnvironmentVariables
(
    const char* processName,        /// [IN] The name of the process to start.
    const char* procCfgPath         /// [IN] The processes path in the config tree.
)
{
    // Setup the user defined environment variables.
    char** envListPtr = cfg_GetRelative(procCfgPath, "envVars");

    if (envListPtr == NULL)
    {
        LE_WARN("Could not read environment variables for process '%s'.", processName);
    }
    else
    {
        size_t i = 0;
        for (i = 0; envListPtr[i] != NULL; i++)
        {
            // Get the enviroment variable's name and value from the environment list's name=value
            // pair string.
            char* varNamePtr = envListPtr[i];
            char* varValuePtr = strchr(envListPtr[i], '=');

            if (varValuePtr != NULL)
            {
                // Split the string into separate name and value strings.
                *varValuePtr = '\0';
                varValuePtr++;

                // Set the environment variable, overwriting anything that was previously there.
                LE_ASSERT(setenv(varNamePtr, varValuePtr, 1) == 0);
            }
            else
            {
                LE_WARN("Environment variable string '%s' is malformed.  It should be a name=value pair.",
                        envListPtr[i]);
            }
        }

        cfg_Free(envListPtr);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Launches all system processes in the order they appear in the SYS_PROCS_CONFIG file.  The
 * Supervisor waits for each system process to signal that it has successfully initialized before
 * going on to start the next process.
 *
 * @note System processes run as root and outside of sandboxes.
 */
//--------------------------------------------------------------------------------------------------
static void LaunchAllSystemProcs
(
    void
)
{
    // Open the config file.
    FILE* sysProcFilePtr = fopen(SYS_PROCS_CONFIG, "r");

    LE_FATAL_IF(sysProcFilePtr == NULL,
                "Could not read system configuration file '%s'.  %m.", SYS_PROCS_CONFIG);

    // Read each line in the file.
    char programPath[LIMIT_MAX_PATH_BYTES];

    while (fgets(programPath, LIMIT_MAX_PATH_BYTES, sysProcFilePtr) != NULL)
    {
        // Strip the line feed.
        programPath[le_utf8_NumBytes(programPath) - 1] = '\0';

        if (IsEmptyString(programPath))
        {
            LE_ERROR("Empty value for system process.");
            continue;
        }

        char* processNamePtr = le_path_GetBasenamePtr(programPath, "/");

        // Kill all other instances of this process just in case.
        char killCmd[LIMIT_MAX_PATH_BYTES];
        int s = snprintf(killCmd, sizeof(killCmd), "killall -q %s", processNamePtr);

        if (s >= sizeof(killCmd))
        {
            LE_FATAL("Could not create 'killall' cmd, buffer is too small.  Buffer is %zd bytes but \
needs to be %d bytes.", sizeof(killCmd), s);
        }

        int r = system(killCmd);

        if (!WIFEXITED(r))
        {
            LE_ERROR("Could not send killall cmd.");
        }

        // Create the process object.
        SysProcObj_t* procPtr = le_mem_ForceAlloc(SysProcObjPool);
        LE_ASSERT(le_utf8_Copy(procPtr->name, processNamePtr, sizeof(procPtr->name), NULL) == LE_OK);

        // Initialize the sys proc object.
        procPtr->link = LE_DLS_LINK_INIT;
        procPtr->stopHandler = DeleteSysProc;

        // Create a synchronization pipe.
        int syncPipeFd[2];
        LE_FATAL_IF(pipe(syncPipeFd) != 0, "Could not create synchronization pipe.  %m.");

        // Fork a process.
        pid_t pID = fork();
        LE_FATAL_IF(pID < 0, "Failed to fork child process.  %m.");

        if (pID == 0)
        {
            // Clear the signal mask so the child does not inherit our signal mask.
            sigset_t sigSet;
            LE_ASSERT(sigfillset(&sigSet) == 0);
            LE_ASSERT(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) == 0);

            // The child does not need the read end of the pipe so close it.
            fd_Close(syncPipeFd[0]);

            // Duplicate the write end of the pipe on standard in so the execed program will know
            // where it is.
            if (syncPipeFd[1] != STDIN_FILENO)
            {
                int r;
                do
                {
                    r = dup2(syncPipeFd[1], STDIN_FILENO);
                }
                while ( (r == -1)  && (errno == EINTR) );
                LE_FATAL_IF(r == -1, "Failed to duplicate fd.  %m.");

                // Close the duplicate fd.
                fd_Close(syncPipeFd[1]);
            }

            // Close all other fds.
            fd_CloseAll();

            // @todo:  Run all sysprocs as non-root.  Nobody really needs to be root except the
            //         Supervisor and the Installer (because it needs to create the user).  Also,
            //         the config path for the sysProcs should not be here (maybe it should just be
            //         hardcoded instead).  This is done this way for now so that the Air Vantage
            //         connector can set environment variables for itself but this all needs to be
            //         cleaned up later.
            SetEnvironmentVariables(processNamePtr, processNamePtr);

            // Launch the child program.  This should not return unless there was an error.
            execl(programPath, programPath, (char*)NULL);

            // The program could not be started.
            LE_FATAL("'%s' could not be started: %m", programPath);
        }

        // Close the write end of the pipe because the parent does not need it.
        fd_Close(syncPipeFd[1]);

        // Wait for the child process to close the read end of the pipe.
        // @todo: Add a timeout here.
        ssize_t numBytesRead;
        int dummyBuf;
        do
        {
            numBytesRead = read(syncPipeFd[0], &dummyBuf, 1);
        }
        while ( ((numBytesRead == -1)  && (errno == EINTR)) || (numBytesRead != 0) );

        LE_FATAL_IF(numBytesRead == -1, "Could not read synchronization pipe.  %m.");

        // Close the read end of the pipe because it is no longer used.
        fd_Close(syncPipeFd[0]);

        // Add the process to the list of system processes.
        procPtr->pid = pID;
        le_dls_Queue(&SysProcsList, &(procPtr->link));

        LE_INFO("Started system process '%s' with PID: %d.",
                processNamePtr, pID);
    }

    LE_ASSERT(fclose(sysProcFilePtr) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts all system processes and user apps.
 */
//--------------------------------------------------------------------------------------------------
static void StartFramework
(
    void
)
{
    // Launch all system processes.
    LaunchAllSystemProcs();
    LE_INFO("All sys procs ready.");

    LE_DEBUG("---- Initializing the configuration API ----");
    le_cfg_Initialize();

    LE_DEBUG("---- Initializing the Supervisor's API ----");
    le_sup_StartServer(LE_SUPERVISOR_API);

    // Launch all user apps in the config tree that should be launched on system startup.
    LaunchAllStartupApps();
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops all system processes.  This function kicks off the chain of handlers that will stop all
 * system processes.
 */
//--------------------------------------------------------------------------------------------------
static void StopSysProcs
(
    void
)
{
    // Stop the system processes in the reverse order they were created.
    le_dls_Link_t* sysProcLinkPtr = le_dls_PeekTail(&SysProcsList);

    if (sysProcLinkPtr != NULL)
    {
        SysProcObj_t* sysProcPtr = CONTAINER_OF(sysProcLinkPtr, SysProcObj_t, link);

        // Do not shutdown the service directory yet.
        if (strcmp(sysProcPtr->name, "serviceDirectory") != 0)
        {
            // Set the stop handler that will stop the next system process.
            sysProcPtr->stopHandler = StopNextSysProc;

            LE_INFO("Killing system process '%s' (PID: %d)", sysProcPtr->name, sysProcPtr->pid);

            // Kill the system process.
            LE_ASSERT(kill(sysProcPtr->pid, SIGKILL) == 0);

            return;
        }
    }

    // The only system process that should be running at this point is the service directory which
    // we need to send back the response.
    // @NOTE We assume the serviceDirectory was the first system process started.
    sysProcLinkPtr = le_dls_Peek(&SysProcsList);

    if (sysProcLinkPtr != NULL)
    {
        if (StopLegatoCmdRef != NULL)
        {
            // Respond to the requesting process to tell it that the Legato framework has stopped.
            le_sup_StopLegatoRespond(StopLegatoCmdRef, LE_OK);
        }

        SysProcObj_t* sysProcPtr = CONTAINER_OF(sysProcLinkPtr, SysProcObj_t, link);

        // Kill the serviceDirectory now.
        LE_ASSERT(kill(sysProcPtr->pid, SIGKILL) == 0);
    }

    LE_INFO("Legato framework shut down.");

    // Exit the Supervisor.
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops all system processes and user apps.  This function kicks off the chain of handlers that
 * will stop all user apps and system processes.
 */
//--------------------------------------------------------------------------------------------------
static void StopFramework
(
    void
)
{
    // Get the first app to stop.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

    if (appLinkPtr != NULL)
    {
        AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);

        // Set the stop handler that will continue to stop all apps and then stop the system processes.
        appPtr->stopHandler = StopNextApp;

        // Stop the first app.  This will kick off the chain of callback handlers that will stop
        // all processes and then stop all system processes.
        app_Stop(appPtr->appRef);
    }
    else
    {
        // There are no apps running.  Stop the systems processes.
        StopSysProcs();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Reboot the system.
 */
//--------------------------------------------------------------------------------------------------
static void Reboot
(
    void
)
{
#ifdef LEGATO_EMBEDDED

    // @todo Copy syslog to persistent file.

    sync();

    if (reboot(RB_AUTOBOOT) == -1);
    {
        LE_EMERG("Failed to reboot the system.  %m.  Attempting to shutdown Legato instead.");

        // @todo gracefully shutdown the framework.

        exit(EXIT_FAILURE);
    }

#else

    // @todo Instead of just exiting we can shutdown and restart the entire framework.
    LE_FATAL("Should reboot the system now but since this is not an embedded system just exit.");

#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a system process object by pid.
 *
 * @return
 *      A pointer to the system process if successful.
 *      NULL if the system process is not found.
 */
//--------------------------------------------------------------------------------------------------
static SysProcObj_t* GetSysProcObj
(
    pid_t pid           // The pid of the system process.
)
{
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&SysProcsList);

    while (procLinkPtr != NULL)
    {
        SysProcObj_t* procPtr = CONTAINER_OF(procLinkPtr, SysProcObj_t, link);

        if (procPtr->pid == pid)
        {
            return procPtr;
        }

        procLinkPtr = le_dls_PeekNext(&SysProcsList, procLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGCHLD called from the Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
static void SigChildHandler
(
    int sigNum
)
{
    // More than one child may have changed state so keep checking until we get all of them.
    while (1)
    {
        // Wait for a terminated child.
        int status;
        pid_t pid;

        do
        {
            pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        }
        while ( (pid < 0) && (errno == EINTR) );

        LE_FATAL_IF((pid == -1) && (errno == EINVAL), "%m.")

        if (pid <= 0)
        {
            // No more children have terminated.
            break;
        }

        // Search the list of System processes.
        SysProcObj_t* sysProcPtr = GetSysProcObj(pid);

        if ( (sysProcPtr != NULL) && (sysProcPtr->stopHandler != NULL) )
        {
            sysProcPtr->stopHandler(sysProcPtr);
        }

        // Search for the process in the list of apps.
        le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

        while (appLinkPtr != NULL)
        {
            AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);

            app_FaultAction_t faultAction;

            if (app_SigChildHandler(appPtr->appRef, pid, status, &faultAction) == LE_OK)
            {
                // Handle the fault.
                switch (faultAction)
                {
                    case APP_FAULT_ACTION_IGNORE:
                        // Do nothing.
                        break;

                    case APP_FAULT_ACTION_RESTART_APP:
                        if (app_GetState(appPtr->appRef) != APP_STATE_STOPPED)
                        {
                            // Stop the app if it hasn't already stopped.
                            app_Stop(appPtr->appRef);
                        }

                        // Set the handler to restart the app when the app stops.
                        appPtr->stopHandler = RestartApp;
                        break;

                    case APP_FAULT_ACTION_STOP_APP:
                        if (app_GetState(appPtr->appRef) != APP_STATE_STOPPED)
                        {
                            // Stop the app if it hasn't already stopped.
                            app_Stop(appPtr->appRef);
                        }
                        break;

                    case APP_FAULT_ACTION_REBOOT:
                        Reboot();

                    default:
                        LE_FATAL("Unknown fault action %d.", faultAction);
                }

                // Check if the app has stopped.
                if ( (app_GetState(appPtr->appRef) == APP_STATE_STOPPED) &&
                     (appPtr->stopHandler != NULL) )
                {
                    // The application has stopped.  Call the app stop handler.
                    appPtr->stopHandler(appPtr);
                }

                // Stop searching the other apps.
                break;
            }

            appLinkPtr = le_dls_PeekNext(&AppsList, appLinkPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an application.  This function is called automatically by the event loop when a separate
 * process requests to start an application.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_StartAppRespond().  The possible result codes are:
 *
 *      LE_OK if the application is successfully started.
 *      LE_DUPLICATE if the application is already running.
 *      LE_NOT_FOUND if the application is not installed.
 *      LE_FAULT if there was an error and the application could not be launched.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_StartApp
(
    le_sup_ServerCmdRef_t _cmdRef,  ///< [IN] The command reference that must be passed to this
                                    ///       command's response function.
    const char* appName             ///< [IN] The name of the application to start.
)
{
    LE_DEBUG("Received request to start application '%s'.", appName);

    le_sup_StartAppRespond(_cmdRef, LaunchApp(appName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an application.  This function is called automatically by the event loop when a separate
 * process requests to stop an application.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_StopAppRespond().  The possible result codes are:
 *
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the application could not be found.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_StopApp
(
    le_sup_ServerCmdRef_t _cmdRef,  ///< [IN] The command reference that must be passed to this
                                    ///       command's response function.
    const char* appName             ///< [IN] The name of the application to stop.
)
{
    LE_DEBUG("Received request to stop application '%s'.", appName);

    // Get the app object.
    AppObj_t* appPtr = GetApp(appName);

    if (appPtr == NULL)
    {
        LE_WARN("Application '%s' is not running and cannot be stopped.", appName);

        le_sup_StopAppRespond(_cmdRef, LE_NOT_FOUND);
        return;
    }

    // Save this commands reference in this app.
    appPtr->stopCmdRef = _cmdRef;

    // Set the handler to be called when this app stops.  This handler will also respond to the
    // process that requested this app be stopped.
    appPtr->stopHandler = RespondToStopAppCmd;

    // Stop the process.  This is an asynchronous call that returns right away.  When the app
    // actually stops the appPtr->stopHandler() will be called.
    app_Stop(appPtr->appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the Legato framework.  This function is called automatically by the event loop when a
 * separate process requests to stop the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_StopLegato
(
    le_sup_ServerCmdRef_t _cmdRef
)
{
    LE_DEBUG("Received request to stop Legato.");

    if (StopLegatoCmdRef != NULL)
    {
        // Someone else has already requested that the framework should be stopped so we should just
        // return right away.
        le_sup_StopLegatoRespond(_cmdRef, LE_DUPLICATE);
    }
    else
    {
        // Disconnect ourselves from the config db.
        le_cfg_StopClient();

        // Save the command reference to use in the response later.
        StopLegatoCmdRef = _cmdRef;

        // Start the process of shutting down the framework.
        StopFramework();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The supervisor's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Block Signals that we are going to use.
    // @todo: This can be done in main by the code generator later.  This could also be a function
    //        in the signals API.
    sigset_t sigSet;
    LE_ASSERT(sigemptyset(&sigSet) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGCHLD) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGPIPE) == 0);
    LE_ASSERT(pthread_sigmask(SIG_BLOCK, &sigSet, NULL) == 0);

    // Set our nice level.
    errno = 0;
    LE_FATAL_IF( (nice(LEGATO_FRAMEWORK_NICE_LEVEL) == -1) && (errno != 0),
                "Could not set the nice level.  %m.");

    // Daemonize ourself.
    Daemonize();

    // Create the Legato runtime directory if it doesn't already exists.
    LE_ASSERT(le_dir_Make(LE_RUNTIME_DIR, S_IRWXU | S_IXOTH) != LE_FAULT);

    // Create and lock a dummy file used to ensure that only a single instance of the Supervisor
    // will run.  If we cannot lock the file than another instance of the Supervisor must be running
    // so exit.
    if (le_flock_TryCreate(SUPERVISOR_INSTANCE_FILE, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST, S_IRWXU) < 0)
    {
        LE_FATAL("Another instance of the Supervisor is already running.  Terminating this instance.");
    }

    // Initialize sub systems.
    cfg_Init();
    user_Init();
    user_RestoreBackup();
    app_Init();

    // Create memory pools.
    AppObjPool = le_mem_CreatePool("Apps", sizeof(AppObj_t));
    SysProcObjPool = le_mem_CreatePool("SysProcs", sizeof(SysProcObj_t));

    // Register a signal event handler for SIGCHLD so we know when processes die.
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    StartFramework();
}
