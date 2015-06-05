/** @file supervisor.c
 *
 * The Legato Supervisor is a daemonized process that has root privileges. It's the first Legato
 * process to start, and is responsible for starting and monitoring the rest of the Legato runtime
 * system.

 *  - @ref c_sup_sysProcs
 *  - @ref c_sup_appStarts
 *  - @ref c_sup_sandboxedApps
 *  - @ref c_sup_nonSandboxedApps
 *  - @ref c_sup_appUsers
 *  - @ref c_sup_faultRecovery
 *  - @ref c_sup_faultLimits
 *  - @ref c_sup_singleInstance
 *  - @ref c_sup_configLayout
 *  - @ref c_sup_smack
 *
 * @section c_sup_sysProcs System Processes
 *
 * Besides the Supervisor, the Legato runtime system consists of a number of system processes that
 * must be started before any apps are started.
 *
 * The system processes must be started in a specific order and must be given time to initialize
 * properly.
 *
 * After starting each system process, the Supervisor waits for the system process to signal that
 * it's ready before continuing to the next system process. Only after all system processes have
 * been started and initialized, will apps be started.  The assumption is made that system
 * processes are trusted and reliable.
 *
 * The system processes must be started in this order:
 * - Service Director
 * - Log Control Daemon
 * - Configuration Database.
 *
 * @todo Currently the list of system processes is stored in the file SYS_PROCS_CONFIG.  This list
 *       contains other system processes in addition to the Service Directory, Log Control Daemon
 *       and Configuration Database.  The additional system processes should probably be removed
 *       from this list and made into pre-installed unsandboxed apps.
 *
 *
 * @section c_sup_appStarts Starting Applications
 *
 * Installed apps may be configured to start automatically or manually. If
 * configured to start automatically, the Supervisor starts the app on
 * start-up, after all system processes have been started.
 *
 * All apps can be stopped and started manually by sending a request to the
 * Supervisor.  Only one instance of the app may be running at a time.
 *
 *
 * @section c_sup_sandboxedApps Sandboxed Applications
 *
 * An app can be configured to be either sandboxed or non-sandboxed.
 *
 * Sandboxed apps run in a chrooted environment and have no visiblity to the rest of the
 * system.  The procedure the Supervisor uses for starting a sandboxed app is:
 *
 *      - Create the directory /tmp/Legato/sandboxes/appName. This is the root of the sandbox.
 *      - Mount a ramfs with a fixed size at the root of the sandbox.
 *      - Create standard directories in the sandbox, like /tmp, /home/appName, /dev, etc.
 *      - Bind mount in standard files and devices into the sandbox, like /dev/null, the Service
 *         Directory sockets, etc.
 *      - Bind mount in all other required files into the sandbox specific to the app.
 *      - Start all the app processes chrooted to the sandbox root and chdir to
 *         /tmp/Legato/sandboxes/appName/home/appName.
 *
 * @Note All sandboxes are created in /tmp so that nothing is persistent.
 *
 * When a sandboxed app is stopped:
 *
 *      - All app processes are killed.
 *      - All mounts are undone.
 *      - Created directories are deleted.
 *
 * @todo Allow some way for sandboxed apps to write/read persistent information.
 *
 *
 * @section c_sup_nonSandboxedApps Non-Sandboxed Applications
 *
 * A non-sandboxed app is one that runs in the main file system.  The Supervisor
 * uses this procedure to start a non-sandboxed app:
 *
 *      - Create the directory /home/appName.
 *      - Run app processes chdir to /home/appName.
 *
 * When a non-sandboxed app is stopped:
 *
 *      - All app processes are killed.
 *
 * @Note The /home/appName directory is not cleaned up because there may be persistent files left in
 *       this directory that the app will need next time it starts.
 *
 * @todo Add capabilities to non-sandboxed apps.
 *
 *
 * @section c_sup_appUsers Application Users and Groups
 *
 * When an app is installed it is assigned a user name, user ID, primary group name and
 * primary group ID.  The user and primary group names are the same and are derived from the
 * app name.
 *
 * Also, non-sandboxed apps may have a list of supplementary groups. If a
 * supplementary group doesn't already exist in the system, the group is created.
 *
 * @Note An app's supplementary groups list isn't stored in the system's /etc/group file
 *       because the supplementary groups are already stored in the config DB added to the
 *       app's processes when the processes are started.
 *
 * When an app starts, all the app's processes are given the app's user ID,
 * primary group ID and, if applicable, supplementary groups ID.
 *
 * @Note Currently an app's user and group(s) aren't deleted when an app is
 *       uninstalled. This is a security issue with non-sandboxed apps because if a different
 *       app is installed with the same name as a previously installed app, the
 *       new app will inherit all the file permissions of the previous app. On the
 *       other hand if the user and group(s) are deleted, a new app may reclaim the same UID
 *       and inherit permissions to files not intended for it. We must give a warning if an
 *       app is installed with a user name that already exists.
 *
 * @todo Currently the Supervisor attempts to create the user each time an app is started.
 *       This task should be moved to the installer so that users and groups are created only during
 *       installation.
 *
 *
 * @section c_sup_faultRecovery Fault Recovery
 *
 * The Supervisor monitors all running app processes for faults. A fault is when a process
 * terminates without returning EXIT_SUCCESS.  When the Supervisor detects a fault, it will perform
 * the configured fault recovery action.
 *
 * The Supervisor doesn't monitor processes that it doesn't start.  Parent processes are
 * responsible for monitoring their children.  However, when the Supervisor terminates an
 * app, the Supervisor will kill off all processes in the app whether it is a child
 * of the Supervisor or not.
 *
 *
 * @section c_sup_faultLimits Fault Limits
 *
 * To prevent a process that is continually faulting from continually consuming resources, the
 * Supervisor imposes a fault limit on all processes in the system.  The fault limit is the minimum
 * time interval between two faults; if more than one fault occurs within the fault limit time
 * interval, the fault limit is reached. The fault limit may be different for each fault
 * action, but they are applied to all app processes.
 *
 * If a process reaches the fault limit, a critical message is logged, the app the process
 * belongs to is shutdown, and no further fault recovery action is taken.
 *
 * The fault limits only prevent automatic recovery by the Supervisor, it doesn't prevent
 * apps from being restarted manually even after the fault limit is exceeded.
 *
 *
 * @section c_sup_singleInstance Single Instance
 *
 * The Supervisor uses a locked file to ensure there is only one instance of the Supervisor
 * running.
 *
 *
 * @section c_sup_configLayout Application Configuration
 *
 * All app configuration settings are stored in the Legato Configuration Database.
 * See @ref frameworkDB.
 *
 *
 * @section c_sup_smack SMACK
 *
 * SMACK policies are set by the Legato startup scripts, the Legato Installer, and the Legato
 * Supervisor.
 *
 * By default system files have the "_" SMACK label meaning everyone has read and execute
 * access to them.  The Legato startup scripts are responsible for setting SMACK labels for system
 * files that require special permission handling (e.g., @c /dev/null file is given the
 * label "*" by the start up scripts so the file is fully accessible to everyone. The Legato startup
 * scripts also ensure the Legato Supervisor and Installer have the 'admin' SMACK
 * label.
 *
 * The Legato Installer sets SMACK labels for all app bundled files.  The SMACK label for
 * each app is unique to the app.
 *
 * The Supervisor sets SMACK labels for framework daemons, processes for apps, sandbox
 * directories and SMACK rules for IPC bindings.
 *
 * Framework daemons are given the SMACK label "framework".
 *
 * All processes are given the same SMACK label as their app. All app labels are unique.
 *
 * SMACK rules are set so IPC bindings between apps work. Here's a code sample of rules to set if a
 * client app needs to access a server app:
 *
 * @code
 * 'clientAppLabel' rw 'serverAppLabel'     // client has read-write access to server.
 * 'serverAppLabel' rw 'clientAppLabel'     // server has read-write access to client.
 * @endcode
 *
 * Sandboxed directories are given labels corresponding to the app's access rights to those
 * directory. Generally, an app only has read and execute permission to its
 * sandboxes /bin directory. Its properties look like this:
 *
 * owner = root
 * group = root
 * DAC permissions = ------r-x
 * SMACK label = 'AppLabelrx'
 *
 * The Supervisor also sets up the SMACK rule so the app has the proper access to the directory:
 *
 * 'AppLabel' rx 'AppLabelrx'
 *
 * App's directories are given different labels than the app itself so that if an IPC binding
 * is present, the remote app has access to the local app but doesn't have direct
 * access to the local app's files.
 *
 * All bundled files within an app's sandbox are given the app's SMACK label. This
 * supports passing file descriptors from one app to another. However, the
 * file descriptor can't be passed onto a third app.
 *
 *
 * @section c_sup_smack_limitations SMACK Limitations
 *
 * Extended attributes used to store the SMACK label are available on
 * all file systems we currently use with one key feature is missing: when a new file is created, the
 * file should inherit the SMACK label of the creator. Because this feature is missing, our
 * current implementation of SMACK has the following limitations:
 *
 * - Mqueue file system will always set new files to "_" label.  This means we can't control
 *    access between apps that use MQueues.
 *
 * - Tmpfs always sets new files to "*" label. This means we can't totally control access to files
 *    created in sandboxes because sandboxes use tmpfs. It's only an issue when file descriptors
 *    for the created files are passed over IPC to another app. The other app can then pass that
 *    fd onto a third app and so on.
 *
 * - Yaffs2/UBIFS do not set any label for newly created files. This causes an issue with the
 *    config daemon that has the label "framework", but its created files don't have any labels. To
 *    work around this, the config daemon must run as root and the 'onlycap' SMACK file must
 *    not be set. This means there is limited protection because all root processes have the
 *    ability to change SMACK labels on files.
 *
 * - QMI sockets are currently set to "*" because some apps need to write to them.
 *    Ideally, the QMI socket file would be given a label such as "qmi" and a rule would be created
 *    to only allow access to the app that requires it.  However, there currently isn't a
 *    way to specify this in the xdef file.
 *
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "user.h"
#include "app.h"
#include "fileDescriptor.h"
#include "config.h"
#include "cgroups.h"
#include "smack.h"
#include "appSmack.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of all apps.
 *
 * If this entry in the config tree is missing or empty then no apps will be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_APPS_LIST                  "apps"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the apps startManual value, used
 * to determine whether the app should be launched on system startup or if it should be
 * deferred for manual launch later.
 *
 * The startManual value is either true or false.  If true the app will not be launched on
 * startup.
 *
 * If this entry in the config tree is missing or is empty, automatic start will be used as the
 * default.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_START_MANUAL               "startManual"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the configuration file that stores all system processes that the Supervisor must
 * start before any user apps.
 */
//--------------------------------------------------------------------------------------------------
#define SYS_PROCS_CONFIG                    "/tmp/LegatoConfigTree/sysProcs"


//--------------------------------------------------------------------------------------------------
/**
 * The file the Supervisor uses to ensure that only a single instance of the Supervisor is running.
 */
//--------------------------------------------------------------------------------------------------
#define SUPERVISOR_INSTANCE_FILE            STRINGIZE(LE_RUNTIME_DIR) "supervisorInst"


//--------------------------------------------------------------------------------------------------
/**
 * app object reference.  Incomplete type so that we can have the app object
 * reference the AppStopHandler_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _appObjRef* AppObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for app stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*AppStopHandler_t)
(
    AppObjRef_t appObjRef               // The app that stopped.
);


//--------------------------------------------------------------------------------------------------
/**
 * App object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct _appObjRef
{
    app_Ref_t              appRef;         // Reference to the app.
    AppStopHandler_t       stopHandler;    // Handler function that gets called when the app stops.
    le_sup_ctrl_ServerCmdRef_t  stopCmdRef;// Stores the reference to the command that requested
                                           // this app be stopped.  This reference must be sent in
                                           // the response to the stop app command.
    le_dls_Link_t          link;           // Link in the list of apps.
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
 * System process object.
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
 * Memory pool for app objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for system process objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SysProcObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * List of all apps.
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
 * Command reference for the Stop Legato command.
 */
//--------------------------------------------------------------------------------------------------
static le_sup_ctrl_ServerCmdRef_t StopLegatoCmdRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Timeout value used to send a SIGKILL
 */
//--------------------------------------------------------------------------------------------------
static const le_clk_Time_t KillTimeout = {
        .sec = 0,
        .usec = 300000,
};


//--------------------------------------------------------------------------------------------------
/**
 * Timer used to stop system processes.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t KillTimerRef;

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
 *
 * @note    This function only returns in the child process. In the parent, it waits until the
 *          child process closes the pipe between the processes, then terminates itself with
 *          a 0 (EXIT_SUCCESS) exit code.
 *
 * @return  File descriptor for pipe to be closed when the framework is ready to use.
 */
//--------------------------------------------------------------------------------------------------
static int Daemonize(void)
{
    // Create a pipe to use to synchronize the parent and the child.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) != 0, "Could not create synchronization pipe.  %m.");

    if (getppid() == 1)
    {
        // Already a daemon.

        // Close the read end of the pipe and return the write end to be closed later.
        fd_Close(syncPipeFd[0]);

        return syncPipeFd[1];
    }

    // Fork off the parent process.
    pid_t pid = fork();

    LE_FATAL_IF(pid < 0, "Failed to fork when daemonizing the supervisor.  %m.");

    // If we got a good PID, are the parent process.
    if (pid > 0)
    {
        // The parent does not need the write end of the pipe so close it.
        fd_Close(syncPipeFd[1]);

        // Do a blocking read on the read end of the pipe.
        int readResult;
        do
        {
            int junk;
            readResult = read(syncPipeFd[0], &junk, sizeof(junk));

        } while ((readResult == -1) && (errno == EINTR));

        exit(EXIT_SUCCESS);
    }

    // Only the child gets here.

    // The child does not need the read end of the pipe so close it.
    fd_Close(syncPipeFd[0]);

    // Start a new session and become the session leader, the process group leader which will free
    // us from any controlling terminals.
    LE_FATAL_IF(setsid() == -1, "Could not start a new session.  %m.");

    // Reset the file mode mask.
    umask(0);

    // Change the current working directory to the root filesystem, to ensure that it doesn't tie
    // up another filesystem and prevent it from being unmounted.
    LE_FATAL_IF(chdir("/") < 0, "Failed to set supervisor's working directory to root.  %m.");

    // Redirect standard fds to /dev/null except for stderr which goes to /dev/console.
    if (freopen("/dev/console", "w", stderr) == NULL)
    {
        LE_WARN("Could not redirect stderr to /dev/console, redirecting it to /dev/null instead.");

        LE_FATAL_IF(freopen("/dev/null", "w", stderr) == NULL,
                    "Failed to redirect stderr to /dev/null.  %m.");
    }

    LE_FATAL_IF( (freopen("/dev/null", "w", stdout) == NULL) ||
                 (freopen("/dev/null", "r", stdin) == NULL),
                "Failed to redirect stdout and stdin to /dev/null.  %m.");

    // Return the write end of the pipe to be closed when the framework is ready for use.
    return syncPipeFd[1];
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete the app object from our list and free the memory.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAppObj
(
    AppObjRef_t appObjRef               // App to delete.
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
    AppObjRef_t appObjRef               // App to restart.
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
 * Responds to the stop app command. Also deletes the app object for the app that
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
    le_sup_ctrl_StopAppRespond(cmdRef, LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the next running app.
 *
 * Deletes the current app object. If no other apps are running stop the first
 * system process.
 */
//--------------------------------------------------------------------------------------------------
static void StopNextApp
(
    AppObjRef_t appObjRef               // App that just stopped.
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
    SysProcObjRef_t sysProcObjRef           // System process that stopped.
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
    SysProcObjRef_t sysProcObjRef           // System process that just stopped.
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
 * Handle termination of the last system process.
 */
//--------------------------------------------------------------------------------------------------
static void HandleLastSysProcStopped
(
    SysProcObjRef_t sysProcObjRef           // System process that just stopped.
)
{
    LE_INFO("Legato framework shut down.");

    // Exit the Supervisor.
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets an app object by name.
 *
 * @return
 *      A pointer to the app object if successful.
 *      NULL if the app is not found.
 */
//--------------------------------------------------------------------------------------------------
static AppObj_t* GetApp
(
    const char* appName     // Name of the application to get.
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
 * Launch an app. Create the app object and starts all its processes.
 *
 * @return
 *      LE_OK if successfully launched the app.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LaunchApp
(
    const char* appNamePtr      // Name of the application to launch.
)
{
    // Check if the app already exists.
    if (GetApp(appNamePtr) != NULL)
    {
        LE_ERROR("Application '%s' is already running.", appNamePtr);
        return LE_DUPLICATE;
    }

    // Get the configuration path for this app.
    char configPath[LIMIT_MAX_PATH_BYTES] = { 0 };

    if (le_path_Concat("/", configPath, LIMIT_MAX_PATH_BYTES,
                       CFG_NODE_APPS_LIST, appNamePtr, (char*)NULL) == LE_OVERFLOW)
    {
        LE_ERROR("App name configuration path '%s/%s' too large for internal buffers!  "
                 "Application '%s' is not installed and cannot run.",
                 CFG_NODE_APPS_LIST, appNamePtr, appNamePtr);
        return LE_FAULT;
    }

    // Check that the app has a configuration value.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(configPath);

    if (le_cfg_IsEmpty(appCfg, ""))
    {
        LE_ERROR("Application '%s' is not installed and cannot run.", appNamePtr);
        le_cfg_CancelTxn(appCfg);
        return LE_NOT_FOUND;
    }

    // Create the app object.
    app_Ref_t appRef = app_Create(configPath);

    if (appRef == NULL)
    {
        le_cfg_CancelTxn(appCfg);
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
        le_cfg_CancelTxn(appCfg);

        return LE_FAULT;
    }

    // @Note: We hang on to the the application config iterator till here to ensure the application
    // configuration does not change during the creation and starting of the application.
    le_cfg_CancelTxn(appCfg);

    // Add the app to the list.
    le_dls_Queue(&AppsList, &(appPtr->link));

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called on system startup to launch all the apps found in the config tree that don't
 * specify the Supervisor should defer their launch.
 */
//--------------------------------------------------------------------------------------------------
static void LaunchAllStartupApps
(
    void
)
{
    // Read the list of applications from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(CFG_NODE_APPS_LIST);

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_WARN("No applications installed.");

        le_cfg_CancelTxn(appCfg);
        return;
    }

    do
    {
        // Check the defer launch for this application.
        if (!le_cfg_GetBool(appCfg, CFG_NODE_START_MANUAL, false))
        {
            // Get the app name.
            char appName[LIMIT_MAX_APP_NAME_BYTES];

            if (le_cfg_GetNodeName(appCfg, "", appName, sizeof(appName)) == LE_OVERFLOW)
            {
                LE_ERROR("AppName buffer was too small, name truncated to '%s'.  "
                         "Max app name in bytes, %d.  Application not launched.",
                         appName, LIMIT_MAX_APP_NAME_BYTES);
            }
            else
            {
                // Launch the application now.  No need to check the return code because there is
                // nothing we can do about errors.
                LaunchApp(appName);
            }
        }
    }
    while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

    le_cfg_CancelTxn(appCfg);
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

            // Close all non-standard fds.
            fd_CloseAllNonStd();

            // @todo:  Run all sysprocs as non-root.  Nobody really needs to be root except the
            //         Supervisor and the Installer (because it needs to create the user).  Also,
            //         the config path for the sysProcs should not be here (maybe it should just be
            //         hardcoded instead).  This is done this way for now so that the Air Vantage
            //         connector can set environment variables for itself but this all needs to be
            //         cleaned up later.
            SetEnvironmentVariables(processNamePtr, processNamePtr);

            smack_SetMyLabel("framework");

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
 * Load the current IPC binding configuration into the Service Directory.
 **/
//--------------------------------------------------------------------------------------------------
static void LoadIpcBindingConfig
(
    void
)
{
    int result = system("sdir load");

    if (result == -1)
    {
        LE_FATAL("Failed to fork child process. (%m)");
    }
    else if (WIFEXITED(result))
    {
        int exitCode = WEXITSTATUS(result);

        if (exitCode != 0)
        {
            LE_FATAL("Couldn't load IPC binding config. `sdir load` exit code: %d.", exitCode);
        }
    }
    else if (WIFSIGNALED(result))
    {
        int sigNum = WTERMSIG(result);

        LE_FATAL("Couldn't load IPC binding config. `sdir load` received signal: %d.", sigNum);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts all system processes and user apps.
 */
//--------------------------------------------------------------------------------------------------
static void StartFramework
(
    int syncFd ///< File descriptor for pipe to be closed when the framework is ready to use.
)
{
    // Launch all system processes.
    LaunchAllSystemProcs();
    LE_INFO("All sys procs ready.");

    // Load the current IPC binding configuration into the Service Directory.
    LoadIpcBindingConfig();

    // Close the synchronization pipe that is connected to the parent process.
    // This signals to the parent process that it is now safe to start using the framework.
    fd_Close(syncFd);

    LE_DEBUG("---- Initializing the configuration API ----");
    le_cfg_ConnectService();

    LE_DEBUG("---- Initializing the Supervisor's APIs ----");
    le_sup_ctrl_AdvertiseService();
    le_sup_wdog_AdvertiseService();
    le_appInfo_AdvertiseService();

    // Initial sub-components that require other services.
    appSmack_AdvertiseService();

    // Launch all user apps in the config tree that should be launched on system startup.
    LaunchAllStartupApps();
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to kill a system processes.
 */
//--------------------------------------------------------------------------------------------------
static void KillSysProc
(
    SysProcObj_t* sysProcPtr
)
{
    LE_INFO("Killing system process '%s' (PID: %d)", sysProcPtr->name, sysProcPtr->pid);

    // Soft Kill the system process.
    LE_ASSERT(kill(sysProcPtr->pid, SIGTERM) == 0);

    // Start a timer in case process does not comply
    le_timer_SetContextPtr(KillTimerRef, sysProcPtr);
    le_timer_Start(KillTimerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops all system processes. This function kicks off the chain of handlers that will stop all
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

            KillSysProc(sysProcPtr);

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
            le_sup_ctrl_StopLegatoRespond(StopLegatoCmdRef, LE_OK);
        }

        SysProcObj_t* sysProcPtr = CONTAINER_OF(sysProcLinkPtr, SysProcObj_t, link);

        // Set a handler
        sysProcPtr->stopHandler = HandleLastSysProcStopped;

        KillSysProc(sysProcPtr);
    }
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

        // If the application has already stopped then call its stop handler here.  Otherwise the
        // stop handler will be called from the SigChildHandler() when the app actually stops.
        if (app_GetState(appPtr->appRef) == APP_STATE_STOPPED)
        {
            appPtr->stopHandler(appPtr);
        }
    }
    else
    {
        // There are no apps running.

        // Disconnect ourselves from the config db so that when we kill the config it does cause us
        // to die too.
        le_cfg_DisconnectService();

        // Stop the systems processes.
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
 * Gets the pid of any child that is in a waitable state without reaping the child process.
 *
 * @return
 *      The pid of the waitable process if successful.
 *      0 if there are currently no waitable children.
 */
//--------------------------------------------------------------------------------------------------
static pid_t WaitPeek
(
    void
)
{
    siginfo_t childInfo = {.si_pid = 0};

    int result;

    do
    {
        result = waitid(P_ALL, 0, &childInfo, WEXITED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT);
    }
    while ( (result == -1) && (errno == EINTR) );

    LE_FATAL_IF(result == -1, "%m.")

    return childInfo.si_pid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reap a specific child.  The child must be in a waitable state.
 *
 * @note This function does not return on error.
 *
 * @return
 *      The status of the reaped child.
 */
//--------------------------------------------------------------------------------------------------
static int WaitReapChild
(
    pid_t pid                       ///< [IN] Pid of the child to reap.
)
{
    pid_t resultPid;
    int status;

    do
    {
        resultPid = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    }
    while ( (resultPid == -1) && (errno == EINTR) );

    LE_FATAL_IF(resultPid == -1, "%m.");

    LE_FATAL_IF(resultPid == 0, "Could not reap child %d.", pid);

    return status;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle application fault.  Gets the application fault action for the process that terminated
 * and handle the fault.
 */
//--------------------------------------------------------------------------------------------------
static void HandleAppFault
(
    AppObj_t* appObjPtr,            ///< [IN] Application object reference.
    pid_t procPid,                  ///< [IN] The pid of the process that changed state.
    int procExitStatus              ///< [IN] The return status of the process given by wait().
)
{
    // Get the fault action.
    app_FaultAction_t faultAction = APP_FAULT_ACTION_IGNORE;

    app_SigChildHandler(appObjPtr->appRef, procPid, procExitStatus, &faultAction);

    // Handle the fault.
    switch (faultAction)
    {
        case APP_FAULT_ACTION_IGNORE:
            // Do nothing.
            break;

        case APP_FAULT_ACTION_RESTART_APP:
            if (app_GetState(appObjPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the app if it hasn't already stopped.
                app_Stop(appObjPtr->appRef);
            }

            // Set the handler to restart the app when the app stops.
            appObjPtr->stopHandler = RestartApp;
            break;

        case APP_FAULT_ACTION_STOP_APP:
            if (app_GetState(appObjPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the app if it hasn't already stopped.
                app_Stop(appObjPtr->appRef);
            }
            break;

        case APP_FAULT_ACTION_REBOOT:
            Reboot();

        default:
            LE_FATAL("Unknown fault action %d.", faultAction);
    }

    // Check if the app has stopped.
    if ( (app_GetState(appObjPtr->appRef) == APP_STATE_STOPPED) &&
         (appObjPtr->stopHandler != NULL) )
    {
        // The application has stopped.  Call the app stop handler.
        appObjPtr->stopHandler(appObjPtr);
    }
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
        // Get the pid of the child process that changed state but do not reap the child so that we
        // can look at the child process's info.
        pid_t pid = WaitPeek();

        if (pid == 0)
        {
            // No more children have terminated.
            break;
        }

        // Search the list of System processes.
        SysProcObj_t* sysProcPtr = GetSysProcObj(pid);

        if (sysProcPtr != NULL)
        {
            // This process is a system process. Reap the child now.
            WaitReapChild(pid);

            if ( le_timer_IsRunning(KillTimerRef) &&
                 (le_timer_GetContextPtr(KillTimerRef) == sysProcPtr) )
            {
                le_timer_Stop(KillTimerRef);
            }

            if (sysProcPtr->stopHandler != NULL)
            {
                sysProcPtr->stopHandler(sysProcPtr);
            }
        }
        else
        {
            // Find the application this process belongs to.
            char appName[LIMIT_MAX_APP_NAME_BYTES];
            le_result_t result = appSmack_GetName(pid, appName, sizeof(appName));

            // Regardless of whether this process is an application process or not we must reap the
            // child process now.
            int status = WaitReapChild(pid);

            switch (result)
            {
                case LE_OK:
                {
                    // Got the app name for the process.  Now get the app object by name.
                    AppObj_t* appObjPtr = GetApp(appName);

                    if (appObjPtr != NULL)
                    {
                        // Handle any faults that the child process state change my have caused.
                        HandleAppFault(appObjPtr, pid, status);
                    }
                    else
                    {
                        LE_CRIT("Could not find running app %s.", appName);
                    }
                    break;
                }

                case LE_OVERFLOW:
                    LE_FATAL("App name '%s...' is too long.", appName);

                case LE_NOT_FOUND:
                    LE_ERROR("Unknown child process %d.", pid);
                    break;

                default:
                    LE_CRIT("Could not get app name for child process %d.", pid);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when a process has not died due to a soft kill signal within the timeout period.
 * Handles soft kill timeout by performing a hard kill.
 */
//--------------------------------------------------------------------------------------------------
static void SysProcsSoftKillExpiryHandler
(
    le_timer_Ref_t timerRef
)
{
    SysProcObj_t* sysProcPtr = le_timer_GetContextPtr(timerRef);

    if (sysProcPtr->pid == -1)
    {
        LE_WARN("Process has already exited");
        return;
    }

    LE_WARN("Hard killing %d", sysProcPtr->pid);

    if (kill(sysProcPtr->pid, SIGKILL) == -1)
    {
        // Process could have exited while we haven't received the SIGCHLD yet
        // Determine if it's still alive
        LE_FATAL_IF( (kill(sysProcPtr->pid, 0) == 0),
                    "Could not send SIGKILL to process '%s' (PID: %d).  %m.",
                    sysProcPtr->name,
                    sysProcPtr->pid);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.  This function is called automatically by the event loop when a separate
 * process requests to start an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_ctrl_StartAppRespond().  The possible result codes are:
 *
 *      LE_OK if the app is successfully started.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if there was an error and the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StartApp
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef, ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    const char* appName                 ///< [IN] Name of the application to start.
)
{
    LE_DEBUG("Received request to start application '%s'.", appName);

    le_sup_ctrl_StartAppRespond(_cmdRef, LaunchApp(appName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an app. This function is called automatically by the event loop when a separate
 * process requests to stop an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_ctrl_StopAppRespond(). The possible result codes are:
 *
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the app could not be found.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StopApp
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef, ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    const char* appName                 ///< [IN] Name of the application to stop.
)
{
    LE_DEBUG("Received request to stop application '%s'.", appName);

    // Get the app object.
    AppObj_t* appPtr = GetApp(appName);

    if (appPtr == NULL)
    {
        LE_WARN("Application '%s' is not running and cannot be stopped.", appName);

        le_sup_ctrl_StopAppRespond(_cmdRef, LE_NOT_FOUND);
        return;
    }

    // Save this commands reference in this app.
    appPtr->stopCmdRef = _cmdRef;

    // Set the handler to be called when this app stops.  This handler will also respond to the
    // process that requested this app be stopped.
    appPtr->stopHandler = RespondToStopAppCmd;

    // Stop the process.  This is an asynchronous call that returns right away.
    app_Stop(appPtr->appRef);

    // If the application has already stopped then call its stop handler here.  Otherwise the stop
    // handler will be called from the SigChildHandler() when the app actually stops.
    if (app_GetState(appPtr->appRef) == APP_STATE_STOPPED)
    {
        appPtr->stopHandler(appPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the Legato framework. This function is called automatically by the event loop when a
 * separate process requests to stop the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StopLegato
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef
)
{
    LE_DEBUG("Received request to stop Legato.");

    if (StopLegatoCmdRef != NULL)
    {
        // Someone else has already requested that the framework should be stopped so we should just
        // return right away.
        le_sup_ctrl_StopLegatoRespond(_cmdRef, LE_DUPLICATE);
    }
    else
    {
        // Save the command reference to use in the response later.
        StopLegatoCmdRef = _cmdRef;

        // Start the process of shutting down the framework.
        StopFramework();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * A watchdog has timed out. This function determines the watchdogAction to take and applies it.
 * The action to take is first delegated to the app (and proc layers) and actions not handled by
 * or not appropriate for lower layers are handled here.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_wdog_WatchdogTimedOut
(
    le_sup_wdog_ServerCmdRef_t _cmdRef,
    uint32_t userId,
    uint32_t procId
)
{
    le_sup_wdog_WatchdogTimedOutRespond(_cmdRef);
    LE_INFO("Handling watchdog expiry for: userId %d, procId %d", userId, procId);

    // Search for the process in the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

    while (appLinkPtr != NULL)
    {
        AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);
        wdog_action_WatchdogAction_t watchdogAction;
        LE_FATAL_IF(appPtr==NULL,"I got a NULL AppPtr from CONTAINER_OF");
        if (app_WatchdogTimeoutHandler(appPtr->appRef, procId, &watchdogAction) == LE_OK)
        {
            // Handle the fault.
            switch (watchdogAction)
            {
                case WATCHDOG_ACTION_NOT_FOUND:
                    // This case should already have been dealt with in lower layers, should never
                    // get here.
                    LE_FATAL("Unhandled watchdog action not found caught by supervisor.");
                case WATCHDOG_ACTION_IGNORE:
                case WATCHDOG_ACTION_HANDLED:
                    // Do nothing.
                    break;

                case WATCHDOG_ACTION_RESTART_APP:
                    if (app_GetState(appPtr->appRef) != APP_STATE_STOPPED)
                    {
                        // Stop the app if it hasn't already stopped.
                        app_Stop(appPtr->appRef);
                    }

                    // Set the handler to restart the app when the app stops.
                    appPtr->stopHandler = RestartApp;
                    break;

                case WATCHDOG_ACTION_STOP_APP:
                    if (app_GetState(appPtr->appRef) != APP_STATE_STOPPED)
                    {
                        // Stop the app if it hasn't already stopped.
                        app_Stop(appPtr->appRef);
                    }
                    break;

                case WATCHDOG_ACTION_REBOOT:
                    Reboot();

                // This should never happen
                case WATCHDOG_ACTION_ERROR:
                    LE_FATAL("Unhandled watchdog action error caught by supervisor.");
                    break;

                // this should never happen
                default:
                    LE_FATAL("Unknown watchdog action %d.", watchdogAction);
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

    if (appLinkPtr == NULL)
    {
        // We exhausted the app list without taking any action for this process
        LE_CRIT("Process pid:%d was not started by the framework. No watchdog action can be taken", procId);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of the specified application.  The state of unknown applications is STOPPED.
 *
 * @return
 *      The state of the specified application.
 */
//--------------------------------------------------------------------------------------------------
le_appInfo_State_t le_appInfo_GetState
(
    const char* appName
        ///< [IN]
        ///< Name of the application.
)
{
    // Search the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

    while (appLinkPtr != NULL)
    {
        AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);

        if (strcmp(app_GetName(appPtr->appRef), appName) == 0)
        {
            switch (app_GetState(appPtr->appRef))
            {
                case APP_STATE_STOPPED:
                    return LE_APPINFO_STOPPED;

                case APP_STATE_RUNNING:
                    return LE_APPINFO_RUNNING;

                default:
                    LE_FATAL("Unrecognized app state.");
            }
        }

        appLinkPtr = le_dls_PeekNext(&AppsList, appLinkPtr);
    }

    return LE_APPINFO_STOPPED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of the specified process in an application.  This function only works for
 * configured processes that the Supervisor starts directly.
 *
 * @return
 *      The state of the specified process.
 */
//--------------------------------------------------------------------------------------------------
le_appInfo_ProcState_t le_appInfo_GetProcState
(
    const char* appName,
        ///< [IN]
        ///< Name of the application.

    const char* procName
        ///< [IN]
        ///< Name of the process.
)
{
    // Search the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&AppsList);

    while (appLinkPtr != NULL)
    {
        AppObj_t* appPtr = CONTAINER_OF(appLinkPtr, AppObj_t, link);

        if (strcmp(app_GetName(appPtr->appRef), appName) == 0)
        {
            switch (app_GetProcState(appPtr->appRef, procName))
            {
                case APP_PROC_STATE_STOPPED:
                    return LE_APPINFO_PROC_STOPPED;

                case APP_PROC_STATE_RUNNING:
                    return LE_APPINFO_PROC_RUNNING;

                case APP_PROC_STATE_PAUSED:
                    return LE_APPINFO_PROC_PAUSED;

                default:
                    LE_FATAL("Unrecognized proc state.");
            }
        }

        appLinkPtr = le_dls_PeekNext(&AppsList, appLinkPtr);
    }

    return LE_APPINFO_PROC_STOPPED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_appInfo_GetName
(
    int32_t pid,
        ///< [IN]
        ///< PID of the process.

    char* appName,
        ///< [OUT]
        ///< Application name

    size_t appNameNumElements
        ///< [IN]
)
{
    return appSmack_GetName (pid, appName, appNameNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * The supervisor's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Block Signals that we are going to use.
    le_sig_Block(SIGCHLD);
    le_sig_Block(SIGPIPE);

    // Set our nice level.
    errno = 0;
    LE_FATAL_IF( (nice(LEGATO_FRAMEWORK_NICE_LEVEL) == -1) && (errno != 0),
                "Could not set the nice level.  %m.");

    // Daemonize ourself.
    int syncFd = Daemonize();

    // Create timer to handle graceful shutdown
    KillTimerRef = le_timer_Create("SupervisorKill");
    le_timer_SetInterval(KillTimerRef, KillTimeout);

    le_timer_SetHandler(KillTimerRef, SysProcsSoftKillExpiryHandler);

    // Create the Legato runtime directory if it doesn't already exist.
    LE_ASSERT(le_dir_Make(STRINGIZE(LE_RUNTIME_DIR), S_IRWXU | S_IXOTH) != LE_FAULT);

    // Create and lock a dummy file used to ensure that only a single instance of the Supervisor
    // will run.  If we cannot lock the file than another instance of the Supervisor must be running
    // so exit.
    if (le_flock_TryCreate(SUPERVISOR_INSTANCE_FILE, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST, S_IRWXU) < 0)
    {
        LE_FATAL("Another instance of the Supervisor is already running.  Terminating this instance.");
    }


#ifdef PR_SET_CHILD_SUBREAPER
    // Set the Supervisor as a sub-reaper so that all descendents of the Superivsor get re-parented
    // to the Supervisor when their parent dies.
    prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0);
#else
    LE_WARN("Set Child Subreaper not supported. Applications with forked processes may not shutdown properly.");
#endif


    // Initialize sub systems.
    cfg_Init();
    user_Init();
    user_RestoreBackup();
    app_Init();
    smack_Init();
    cgrp_Init();

    // Create memory pools.
    AppObjPool = le_mem_CreatePool("apps", sizeof(AppObj_t));
    SysProcObjPool = le_mem_CreatePool("sysProcs", sizeof(SysProcObj_t));

    // Register a signal event handler for SIGCHLD so we know when processes die.
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    StartFramework(syncFd);
}
