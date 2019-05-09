//--------------------------------------------------------------------------------------------------
/** @file supervisor.c
 *
 * The Legato Supervisor is the first Legato framework process to start and is responsible for
 * starting and monitoring all other framework processes as well as applications.  The Supervisor
 * has root privileges and administrative MAC rights.
 *
 *  - @ref c_sup_kernelModules
 *  - @ref c_sup_frameworkDaemons
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
 * @section c_sup_kernelModules Kernel Modules
 *
 * Prior to starting any executables, Supervisor inserts kernel modules bundled with Legato app.
 * Legato-supplied modules are considered to be self-contained and independent from each-other.
 * They are inserted in alphabetical order, i.e. in the order in which they are listed in Legato's
 * system/modules directory.
 *
 * @section c_sup_frameworkDaemons Framework Daemons
 *
 * Besides the Supervisor, the Legato runtime system consists of a number of framework daemons that
 * must be started before any apps are started.
 *
 * The framework daemons must be started in a specific order and must be given time to initialize
 * properly.
 *
 * After starting each framework daemon, the Supervisor waits for the daemon to signal that it's
 * ready before continuing to the next daemon. Only after all framework daemons have been started
 * and initialized, will apps be started.
 *
 * @section c_sup_appStarts Starting Applications
 *
 * Installed apps may be configured to start automatically or manually. If configured to start
 * automatically, the Supervisor starts the app on start-up, after all framework daemons have been
 * started.
 *
 * All apps can be stopped and started manually by sending a request to the Supervisor.  Only one
 * instance of the app may be running at a time.
 *
 * @section c_sup_sandboxedApps Sandboxed Applications
 *
 * An app can be configured to be either sandboxed or non-sandboxed.
 *
 * Sandboxed apps run in a chrooted environment and have no visibility to the rest of the
 * system.  Sandboxed apps also have strict resource limits.
 *
 * @section c_sup_nonSandboxedApps Non-Sandboxed Applications
 *
 * A non-sandboxed app is one that runs in the main file system.
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
 * @section c_sup_faultRecovery Fault Recovery
 *
 * The Supervisor monitors all running app processes for faults. A fault is when a process
 * terminates without returning EXIT_SUCCESS.  When the Supervisor detects a fault, it will perform
 * the configured fault recovery action.
 *
 *
 * @section c_sup_faultLimits Fault Limits
 *
 * To prevent a process that is continually faulting from continually consuming resources, the
 * Supervisor imposes a fault limit on all processes in the system.  The fault limit is the minimum
 * time interval between two faults; if more than one fault occurs within the fault limit time
 * interval, the fault limit is reached.
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
 * The Supervisor refers to the "apps" branch of the "system" config tree to determine what
 * apps exist, how they should be started, and which ones should be started automatically when
 * the framework comes up.
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
 * all file systems we currently use with one key feature missing: when a new file is created, the
 * file should inherit the SMACK label of the creator. Because this feature is missing, our
 * current implementation of SMACK has the following limitations:
 *
 * - Mqueue file system will always set new files to "_" label.  This means we can't control
 *   access between apps that use MQueues.
 *
 * - Tmpfs always sets new files to "*" label. This means we can't totally control access to files
 *   created in sandboxes because sandboxes use tmpfs. It's only an issue when file descriptors
 *   for the created files are passed over IPC to another app. The other app can then pass that
 *   fd onto a third app and so on.
 *
 * - Yaffs2 does not set any label for newly created files. This causes an issue with the
 *   config daemon that has the label "framework", but its created files don't have any labels. To
 *   work around this, the config daemon must run as root and the 'onlycap' SMACK file must
 *   not be set. This means there is limited protection because all root processes have the
 *   ability to change SMACK labels on files.
 *   Note that UBIFS no longer has this issue.
 *
 * - QMI sockets are currently set to "*" because some apps need to write to them.
 *   Ideally, the QMI socket file would be given a label such as "qmi" and a rule would be created
 *   to only allow access to the app that requires it.  However, there currently isn't a
 *   way to specify this in the xdef file.  This is not a limitation of SMACK or the file system but
 *   the xdef files.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "user.h"
#include "kernelModules.h"
#include "frameworkDaemons.h"
#include "cgroups.h"
#include "smack.h"
#include "supervisor.h"
#include "sysPaths.h"
#include "daemon.h"
#include "apps.h"
#include "wait.h"
#include "fileSystem.h"
#include "sysStatus.h"
#include "fileDescriptor.h"
#include "start.h"
#include "sysPaths.h"
#include "sysStatus.h"
#include "ima.h"
#include "fs.h"


//--------------------------------------------------------------------------------------------------
/**
 * The file the Supervisor uses to ensure that only a single instance of the Supervisor is running.
 */
//--------------------------------------------------------------------------------------------------
#define SUPERVISOR_INSTANCE_FILE            LE_CONFIG_RUNTIME_DIR "/supervisorInst"


//--------------------------------------------------------------------------------------------------
/**
 * Boot configuration path
 */
//--------------------------------------------------------------------------------------------------
#define BOOT_CFG_PATH "/"


//--------------------------------------------------------------------------------------------------
/**
 * Location in boot configuration path to store user defined value for minimum allowable time (in
 * milliseconds) between boots. If system restarts less than this time, it is treated as a boot
 * loop.
 */
//--------------------------------------------------------------------------------------------------
#define BOOT_TIMEOUT_PATH "bootTimeout"


//--------------------------------------------------------------------------------------------------
/**
 * Default value of minimum allowable time between boots. This value is taken if there is no user
 * defined value.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_BOOT_PERIOD                 60000


//--------------------------------------------------------------------------------------------------
/**
 * Reboot timer
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t RebootTimer;


//--------------------------------------------------------------------------------------------------
/**
 * Enumerates the different application start options that can be provided on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static enum
{
    APP_START_AUTO,     ///< Start all apps that are marked for automatic start.
    APP_START_NONE      ///< Don't start any apps until told to do so through the App Control API.
}
AppStartMode = APP_START_AUTO;   // Default is to start apps.


//--------------------------------------------------------------------------------------------------
/**
 * Command reference for asynchronous API commands le_framework_Stop(), le_framework_Restart().
 */
//--------------------------------------------------------------------------------------------------
static le_framework_ServerCmdRef_t StopApiCmdRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Flag to indicate whether the deprecated API is being used.  Delete this flag when the deprecated
 * API is deleted.
 */
//--------------------------------------------------------------------------------------------------
static bool UsingDeprecatedSupCtrlApi = false;


//--------------------------------------------------------------------------------------------------
/**
 * Operating states.
 **/
//--------------------------------------------------------------------------------------------------
static enum
{
    STATE_STARTING,           ///< Starting the framework. No apps running yet.
    STATE_NORMAL,             ///< Normal operation. Fully initialized. All framework daemons running.
    STATE_STOPPING,           ///< Controlled shutdown of framework underway.
    STATE_RESTARTING,         ///< Controlled shutdown and restart of framework underway.
    STATE_RESTARTING_MANUAL,  ///< Manual shutdown and restart of framework underway.
    STATE_RESTARTING_START,   ///< Controlled shutdown of framework and run current start.
}
State = STATE_STARTING;


//--------------------------------------------------------------------------------------------------
/**
 * true if the process should NOT daemonize itself (i.e., it should remain attached to its
 * controlling terminal and parent process)
 **/
//--------------------------------------------------------------------------------------------------
static bool ShouldNotDaemonize = false;


//--------------------------------------------------------------------------------------------------
/**
 * Indicates to supervisor which start program is being used.
 **/
//--------------------------------------------------------------------------------------------------
static const char* CurrentStartVersion = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Prints man page style usage help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    fprintf(stderr, "Printing help...\n");

    const char* programName = le_arg_GetProgramName();

    printf( "NAME\n"
            "        %s - Starts the Legato framework.\n"
            "\n"
            "SYNOPSIS\n"
            "        %s [OPTION]\n"
            "\n"
            "DESCRIPTION\n"
            "        Start up the Legato application framework daemon processes.\n"
            "\n"
            "        Options:\n"
            "\n"
            "        -a, --start-apps=MODE\n"
            "                If MODE is 'auto', start all apps marked for auto start\n"
            "                (this is the default).  If MODE is 'none', don't start\n"
            "                any apps until told to do so through the App Control API.\n"
            "\n"
            "        -n, --no-daemonize\n"
            "                The Supervisor does not daemonize itself.\n"
            "\n"
            "        -v, --version\n"
            "                The version of the start program being used.\n"
            "\n"
            "        -h --help\n"
            "                Print this help text to standard output stream and exit.\n",
            programName,
            programName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments for options.
 */
//--------------------------------------------------------------------------------------------------
static void ParseCommandLine
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    bool printHelp = false;
    const char* appStartModeArgPtr = NULL;

    le_arg_SetStringVar(&appStartModeArgPtr, "a", "start-apps");
    le_arg_SetFlagVar(&printHelp, "h", "help");
    le_arg_SetFlagVar(&ShouldNotDaemonize, "n", "no-daemonize");
    le_arg_SetStringVar(&CurrentStartVersion, "v", "version");

    // Run the argument scanner.
    le_arg_Scan();

    // Check for the help flag first.  It overrides everything else.
    if (printHelp)
    {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    // If the -a (--start-apps) option was provided,
    if (appStartModeArgPtr != NULL)
    {
        if (strcmp(appStartModeArgPtr, "none") == 0)
        {
            AppStartMode = APP_START_NONE;
        }
        else if (strcmp(appStartModeArgPtr, "auto") != 0)
        {
            fprintf(stderr,
                    "Invalid --start-apps (-a) option '%s'.  Must be 'auto' or 'none'.\n",
                    appStartModeArgPtr);
            exit(EXIT_FAILURE);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to read the current Legato version string from the file system.
 */
//--------------------------------------------------------------------------------------------------
static void GetCurrentLegatoVersion
(
    char* versionBuffer,  ///< Buffer to hold the string.
    size_t versionSize    ///< Size of buffer
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("Read the Legato version string.");

    FILE* versionFile = fopen("/legato/systems/current/version", "r");

    if (versionFile == NULL)
    {
        LE_ERROR("Could not open Legato version file.");
        return;
    }

    if (fgets(versionBuffer, versionSize, versionFile) != NULL)
    {
        char* newLine = strchr(versionBuffer, '\n');

        if (newLine != NULL)
        {
            *newLine = 0;
        }

        LE_DEBUG("The current Legato framework version is, '%s'.", versionBuffer);
    }
    else
    {
        LE_ERROR("Could not read Legato version.");
    }

    fclose(versionFile);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Check if supervisor was launched by the startSystem executable. There are cases where supervisor
 *  is executed from external sources.
 */
//--------------------------------------------------------------------------------------------------
static bool IsParentStart
(
    void
)
{
    char procName[LE_LIMIT_PROC_NAME_LEN + 1] = "";
    char procPidPath[LE_LIMIT_PROC_NAME_LEN + 1] = "";

    snprintf(procPidPath, sizeof(procPidPath), "/proc/%d/cmdline", getppid());

    int fd = open(procPidPath, O_RDONLY);
    if (0 <= fd)
    {
        ssize_t result = read(fd, procName, LE_LIMIT_PROC_NAME_LEN);
        if (result <= 0)
        {
            LE_ERROR("Unable to read '%s': %m", procName);
            close(fd);
            return false;
        }
        close(fd);
    }

    if (strstr(procName, "startSystem") != NULL)
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts all framework daemons and apps.
 *
 * Closes stdin (reopens to /dev/null) when finished to signal any parent process that cares that
 * the framework is started.
 */
//--------------------------------------------------------------------------------------------------
static void StartFramework
(
    void
)
{
    // Start a daemon start-up watchdog timer.
    // If we don't cancel this timer within 30 seconds, a SIGALRM will be generated, which will
    // kill the Supervisor.
    alarm(30);

    // Start all framework daemons.
    fwDaemons_Start();

    // Connect to the services we need from the framework daemons.
    LE_DEBUG("---- Connecting to services ----");
    le_cfg_ConnectService();
    logFd_ConnectService();
    le_instStat_ConnectService();

    // Cancel the start-up watchdog timer.
    alarm(0);

    // Insert kernel modules
    kernelModules_Insert();

    // Advertise services.
    LE_DEBUG("---- Advertising the Supervisor's APIs ----");
    le_sup_ctrl_AdvertiseService();
    le_appCtrl_AdvertiseService();
    le_framework_AdvertiseService();
    wdog_AdvertiseService();
    supervisorWdog_AdvertiseService();
    le_appInfo_AdvertiseService();
    le_appProc_AdvertiseService();
    le_ima_AdvertiseService();
    le_kernelModule_AdvertiseService();

    // Close stdin (and reopen to /dev/null to be safe).
    // This signals to the parent process that it is now safe to start using the framework.
    // NOTE: Do this after advertising services in case anyone uses a "Try" version of an
    //       IPC connection function to connect to one of these services (which would report
    //       that the service is unavailable if it is not yet advertised).
    LE_FATAL_IF(freopen("/dev/null", "r", stdin) == NULL,
                "Failed to redirect stdin to /dev/null.  %m.");

    // Initialize the apps sub system.
    apps_Init();
    apps_VerifyAppWriteableDeviceFiles();

    State = STATE_NORMAL;

    if (AppStartMode == APP_START_AUTO)
    {
        // Launch all user apps in the config tree that should be launched on system startup.
        LE_INFO("Auto-starting apps.");
        apps_AutoStart();
    }
    else
    {
        LE_INFO("Skipping app auto-start.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the reboot count file
 */
//--------------------------------------------------------------------------------------------------
static void DeleteRebootCount
(
   void
)
{
    // Just delete the file.  It should exist here, but if it doesn't there's no problem.
    (void)unlink(BOOT_COUNT_PATH);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the boot timer period. Should be called after all framework daemons are running.
 *
 * @return
 *     The timer period, in milliseconds.
 **/
//--------------------------------------------------------------------------------------------------
static size_t GetBootExpirePeriod
(
    void
)
{
    int period;

    // Read the user defined timeout from config tree
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(BOOT_CFG_PATH);
    period = le_cfg_GetInt(iterRef, BOOT_TIMEOUT_PATH, DEFAULT_BOOT_PERIOD);
    le_cfg_CancelTxn(iterRef);

    LE_INFO("Boot timeout period = %d ms (~%d seconds)", period, period/1000);

    return (size_t)period;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle fast reboot detect timer expiring.
 *
 * Deletes the reboot count file
 */
//--------------------------------------------------------------------------------------------------
static void HandleRebootExpiry
(
    le_timer_Ref_t timer
)
{
    LE_INFO("Expired reboot timer");
    DeleteRebootCount();
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the Supervisor.  This should only be called after all user apps and framework daemons are
 * shutdown.
 */
//--------------------------------------------------------------------------------------------------
static void StopSupervisor
(
    void
)
{
    // Older start programs need us to do this as they cannot do it for themselves!
    fs_TryLazyUmount(CURRENT_SYSTEM_PATH);

    // At the current time - each of these is a controlled shutdown of some type
    // that indicate that a try has not failed but was ended for some deliberate
    // reason before probation was completed. Back out the last try from the
    // status - it doesn't count towards failed tries.
    if (State == STATE_RESTARTING)
    {
        // Initiated by updateDaemon requesting restart
        LE_INFO("Legato framework shut down complete. Restarting...");
        sysStatus_DecrementTryCount();
        exit(LE_START_EXIT_RESTART);
    }
    else if (State == STATE_RESTARTING_MANUAL)
    {
        // Initiated by user command restartLegato
        LE_INFO("Legato framework manual shut down complete. Restarting...");
        sysStatus_DecrementTryCount();
        exit(LE_START_EXIT_MANUAL_RESTART);
    }
    else if (State == STATE_STOPPING)
    {
        // Initiated by user command stopLegato
        LE_INFO("Legato framework shut down.");
        sysStatus_DecrementTryCount();
        // Exit the Supervisor.
        exit(EXIT_SUCCESS);
    }
    else if (State == STATE_RESTARTING_START)
    {
        // Initiated by user command stopLegato
        LE_INFO("Legato framework shut down. Restarting with current start.");
        sysStatus_DecrementTryCount();
        sysStatus_DecrementBootCount();
        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_FATAL("Unexpected state %d.", State);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prepares for a full shutdown of the framework by responding to the Stop Legato command telling
 * the requesting process the framework has shutdown and closing all services that the Supervisor
 * has advertised.
 *
 * This should be called only when all user apps and all framework daemons, except the Service
 * Directory, are shutdown but before the Service Directory and Supervisor are shutdown.
 */
//--------------------------------------------------------------------------------------------------
static void PrepareFullShutdown
(
    void
)
{
    if (StopApiCmdRef != NULL)
    {
        if (State == STATE_STOPPING)
        {
            // Respond to the requesting process to tell it that the Legato framework has stopped.
            if (UsingDeprecatedSupCtrlApi)
            {
                le_sup_ctrl_StopLegatoRespond((le_sup_ctrl_ServerCmdRef_t)StopApiCmdRef, LE_OK);
            }
            else
            {
                le_framework_StopRespond(StopApiCmdRef, LE_OK);
            }
        }
        else if (State == STATE_RESTARTING || State == STATE_RESTARTING_MANUAL)
        {
            // Respond to the requesting process to tell it that the Legato framework has stopped.
            if (UsingDeprecatedSupCtrlApi)
            {
                le_sup_ctrl_RestartLegatoRespond((le_sup_ctrl_ServerCmdRef_t)StopApiCmdRef, LE_OK);
            }
            else
            {
                le_framework_RestartRespond(StopApiCmdRef, LE_OK);
            }
        }
        else
        {
            LE_CRIT("Unexpected state %d.", State);
        }
    }

    // Close services that we've advertised before the Service Directory dies.
    le_msg_HideService(le_sup_ctrl_GetServiceRef());
    le_msg_HideService(le_appCtrl_GetServiceRef());
    le_msg_HideService(le_framework_GetServiceRef());
    le_msg_HideService(wdog_GetServiceRef());
    le_msg_HideService(supervisorWdog_GetServiceRef());
    le_msg_HideService(le_appInfo_GetServiceRef());
    le_msg_HideService(le_appProc_GetServiceRef());
    le_msg_HideService(le_ima_GetServiceRef());
    le_msg_HideService(le_kernelModule_GetServiceRef());
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops framework daemons.  This function kicks off the chain of handlers that will stop all
 * framework daemons.
 */
//--------------------------------------------------------------------------------------------------
static void ShutdownFramework
(
    void
)
{
    // Disconnect ourselves from services we use so when we kill the servers it does not cause us
    // to die too.
    le_cfg_DisconnectService();
    logFd_DisconnectService();
    le_instStat_DisconnectService();

    // Set the framework daemon shutdown handlers.
    fwDaemons_SetIntermediateShutdownHandler(PrepareFullShutdown);
    fwDaemons_SetShutdownHandler(StopSupervisor);

    // Stop the framework daemons.
    fwDaemons_Shutdown();

    // Remove kernel modules.
    kernelModules_Remove();
}


//--------------------------------------------------------------------------------------------------
/**
 * Shuts down all apps and all framework daemons.  The shutdown process is asynchronous  and this
 * function kicks off the chain of handlers that will shutdown all apps and framework daemons.
 */
//--------------------------------------------------------------------------------------------------
static void BeginShutdown
(
    void
)
{
    // Begin the shutdown process by shutting down all the apps.  When the apps finish shutting
    // down the apps shutdown handler will trigger the shutdown of the framework itself.
    apps_SetShutdownHandler(ShutdownFramework);

    apps_Shutdown();
}


//--------------------------------------------------------------------------------------------------
/**
 * Reboot the system.
 */
//--------------------------------------------------------------------------------------------------
void framework_Reboot
(
    void
)
{
    LE_FATAL("Supervisor going down to trigger reboot.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Called to capture any extra data that may help indicate what contributed to the fault that
 * caused the framework to fail.
 *
 * This function calls a shell script that will save a dump of the system log and any core files
 * that have been generated into a known location.
 */
//--------------------------------------------------------------------------------------------------
static void CaptureDebugData
(
    void
)
{
    int r = system("/legato/systems/current/bin/saveLogs framework unknown REBOOT");

    if (!WIFEXITED(r) || (WEXITSTATUS(r) != EXIT_SUCCESS))
    {
        LE_ERROR("Could not save log and core file.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGCHLD called from the Legato event loop.
 *
 * This is called for all framework daemon processes as well as most application processes.
 * Application processes that were started by the Supervisor are children of the Supervisor and
 * naturally generate a SIGCHILD to the Supervisor when they die.  Application processes
 * that were started by other processes in the same app would generate SIGCHILDs to their parent not
 * the Supervisor.  However, these lower level processes are still descendents of the Supervisor and
 * if their parent were to die they would be reparented to the Supervisor.  This is because the
 * Supervisor is a sub-reaper.
 *
 * Because SIGCHILD signals may come from either apps or framework daemons they are caught here
 * first.  In this function we do a wait_Peek() to get the PID of the process that generated the
 * SIGCHILD without reaping the child.  The PID is passed down to the apps SIGCHILD handler and
 * framework daemon SIGCHILD handler for identification and processing.  The lower layer handlers
 * are assumed to reap the child only if it is going to handle the process death.  If neither the
 * apps or framework daemons recognize the child then we must reap it here.
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
        pid_t pid = wait_Peek();

        if (pid == 0)
        {
            // No more children have terminated.
            break;
        }

        // Send the pid to the apps SIGCHILD handler for processing.
        le_result_t result = apps_SigChildHandler(pid);

        if (result == LE_FAULT)
        {
            // There was an app fault that could not be handled so restart the framework.
            framework_Reboot();
        }

        if (result == LE_NOT_FOUND)
        {
            // Send the pid to the framework daemon's SIGCHILD handler for processing.
            le_result_t r = fwDaemons_SigChildHandler(pid);

            if (r == LE_FAULT)
            {
                CaptureDebugData();
                framework_Reboot();
            }
            else if (r == LE_NOT_FOUND)
            {
                // The child is neither an application process nor a framework daemon.
                // Reap the child now.
                LE_INFO("Reaping unconfigured child process %d.", pid);

                wait_ReapChild(pid);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively update the SMACK labels (ignore EROFS or ENOENT errors).
 */
//--------------------------------------------------------------------------------------------------
static void UpdateSmackLabelRecursive
(
    const char* pathNamePtr,          ///< [IN] The path to the directory to update.
    const char* smackLegatoLabelPtr   ///< [IN] The SMACK label for Legato objects.
)
{
    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char*)pathNamePtr, NULL};

    errno = 0;
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL | FTS_NOSTAT, NULL);

    if (ftsPtr == NULL)
    {
        LE_CRIT("Cannot open path '%s': %m.", pathNamePtr);
        return;
    }

    // Step through the directory tree.
    FTSENT* entPtr;
    while ((entPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
            case FTS_F:
            case FTS_SL:
            case FTS_NSOK:
                LE_DEBUG("Type (%d): %s", entPtr->fts_info, entPtr->fts_path);
                if ((-1 == setxattr(entPtr->fts_path, "security.SMACK64",
                                    smackLegatoLabelPtr, strlen(smackLegatoLabelPtr), 0)) &&
                    ((EROFS != errno) && (ENOENT != errno)))
                {
                    LE_CRIT("Could not set SMACK label for '%s': %m.", entPtr->fts_path);
                }
                break;

            case FTS_SLNONE:
                LE_DEBUG("Skipping (%d): %s", entPtr->fts_info, entPtr->fts_path);
                break;
        }
    }

    fts_close(ftsPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Setup all the smack rules before enabling smack onlycap
 */
//--------------------------------------------------------------------------------------------------
static void SetupSmackOnlyCap
(
    void
)
{
    // Set correct smack permissions for admin (Bug where onlycap label does not have CAP_MAC_OVERRIDE)
    smack_SetRule("admin", "rwx", "_");

    // Set correct smack permissions for app.tools and framework
    smack_SetRule("admin", "rwx", "app.tools");
    smack_SetRule("admin", "rwx", "framework");

    // Set correct smack permissions for syslog
    smack_SetRule("_", "rw", "syslog");
    smack_SetRule("admin", "rw", "syslog");
    smack_SetRule("framework", "rw", "syslog");

    // Set correct smack label for /home directory
    smack_SetLabel("/home", "_");

    // Framework needs write access to '_' label. e.g. configEcm needs write permission to /etc/legato
    // framework needs wx access to tmpfs
    smack_SetRule("framework", "rwx", "_");

    // Set correct smack label for /data
    smack_SetLabel("/data", "_");

    // ToDo: Workaround to get le_fs have "framework" label.
    // Set correct smack label for /data
    fs_Init();

    // Set correct smack label for /data/le_fs
    smack_SetLabel("/data/le_fs", "framework");

    // Remove previously set rule if cached from an update
    smack_SetRule("_", "-", "admin");
    smack_SetRule("_", "-", "framework");

    // logDaemon needs read access to admin (fds)
    smack_SetRule("framework", "r", "admin");

    // Set correct permissions for qmuxd
    smack_SetRule("qmuxd", "rwx", "_");
    smack_SetRule("_", "rwx", "qmuxd");

    // Set admin label for the supervisor.
    smack_SetMyLabel("admin");

#if LE_CONFIG_SMACK_ONLYCAP
    // Set onlycap with 'admin' label
    smack_SetOnlyCap("admin");
    LE_INFO("SMACK onlycap enabled");
#else /* not LE_CONFIG_SMACK_ONLYCAP */
    LE_INFO("SMACK onlycap disabled");
#endif /* end not LE_CONFIG_SMACK_ONLYCAP */
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the Legato framework.
 *
 * Async API function.  Calls le_framework_StopRespond() to report results.
 */
//--------------------------------------------------------------------------------------------------
void le_framework_Stop
(
    le_framework_ServerCmdRef_t cmdRef
)
{
    UsingDeprecatedSupCtrlApi = false;

    LE_DEBUG("Received request to stop Legato.");

    if (State != STATE_NORMAL)
    {
        le_framework_StopRespond(cmdRef, LE_DUPLICATE);
    }
    else
    {
        // Save the command reference to use in the response later.
        StopApiCmdRef = cmdRef;

        State = STATE_STOPPING;

        // Start the process of shutting down the framework.
        BeginShutdown();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Restarts the Legato framework.
 *
 * Async API function.  Calls le_framework_RestartRespond() to report results.
 */
//--------------------------------------------------------------------------------------------------
void le_framework_Restart
(
    le_framework_ServerCmdRef_t cmdRef,
    bool manualRestart
)
{
    UsingDeprecatedSupCtrlApi = false;

    LE_DEBUG("Received request to restart Legato.");

    if (State == STATE_NORMAL)
    {
        // Save the command reference to use in the response later.
        StopApiCmdRef = cmdRef;

        if (manualRestart)
        {
            State = STATE_RESTARTING_MANUAL;
        }
        else
        {
            State = STATE_RESTARTING;
        }

        // Start the process of shutting down the framework.
        BeginShutdown();
    }
    else
    {
        LE_DEBUG("Ignoring request to restart Legato in state %d.", State);

        le_framework_RestartRespond(cmdRef, LE_DUPLICATE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Import a public certificate in linux keyring.
 *
 * @return
 *     - LE_OK if successful
 *     - LE_FAULT if there is a failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ima_ImportCert
(
    const char* certPath
)
{
    if ((NULL == certPath) || (0 == strcmp(certPath, "")))
    {
        LE_KILL_CLIENT("Certificate path cannot be empty.");
        return LE_FAULT;
    }

    return ima_ImportPublicCert(certPath);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reports if the Legato framework is stopping.
 *
 * @return
 *     true if the framework is stopping or rebooting
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool framework_IsStopping
(
    void
)
{
    switch (State)
    {
        case STATE_STARTING:
        case STATE_NORMAL:
            return false;
        default:
            return true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Reports if the Legato framework is stopping.
 *
 * API implementation function.
 *
 * @return
 *     true if the framework is stopping or rebooting
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
void le_framework_IsStopping
(
    le_framework_ServerCmdRef_t _cmdRef
)
{
    le_framework_IsStoppingRespond(_cmdRef, framework_IsStopping());
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether legato framework is Read-Only or not.
 *
 * @return
 *     true if the framework is Read-Only
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
void le_framework_IsReadOnly
(
    le_framework_ServerCmdRef_t _cmdRef
)
{
    le_framework_IsReadOnlyRespond(_cmdRef, sysStatus_IsReadOnly());
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the next reboot as expected. Should be called by short lived app that shutdown platform
 * after a small wakeup. This prevents system not to rollback on expected reboot.
 */
//--------------------------------------------------------------------------------------------------
void le_framework_NotifyExpectedReboot
(
    le_framework_ServerCmdRef_t _cmdRef
)
{
    if (false == sysStatus_IsReadOnly())
    {
        DeleteRebootCount();
    }
    le_framework_NotifyExpectedRebootRespond(_cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * The supervisor's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    ParseCommandLine();

    // Block Signals that we are going to use.
    le_sig_Block(SIGPIPE);

    // Set our nice level.
    errno = 0;
    LE_FATAL_IF( (nice(LE_CONFIG_SUPERV_NICE_LEVEL) == -1) && (errno != 0),
                "Could not set the nice level.  %m.");

    // Unless we have been asked not to, daemonize ourself.
    if (!ShouldNotDaemonize)
    {
        daemon_Daemonize(-1);   /// -1 = Never timeout.
    }
    else
    {
        // Make sure our umask is always cleared so that the framework created files are given
        // proper permissions.
        umask(0);
    }

    // Get the current legato version
    char versionBuffer[255] = "";
    GetCurrentLegatoVersion(versionBuffer, sizeof(versionBuffer));

    // There are two cases where supervisor will need to fork and exec the latest start program.
    // 1) When no start version has been specified (implies it is old start program)
    // 2) When the start program version mismatches with the current legato version
    if (((CurrentStartVersion == NULL) && IsParentStart()) ||
       ((CurrentStartVersion != NULL) && (strcmp(CurrentStartVersion, versionBuffer) != 0)))
    {
        // Need to fork; otherwise exec the new start will cause the tried counter to increment.
        pid_t pid = fork();

        if (pid == 0)
        {
            LE_INFO("Version mismatch. Fork and exec'ing latest start program.");
            LE_DEBUG("[Current legato version: %s]", versionBuffer);
            LE_DEBUG("[Current start version: %s]", CurrentStartVersion);

            // Exec the latest start program
            const char* startPath = "/legato/systems/current/bin/startSystem";
            (void)execl(startPath, startPath, "-v", versionBuffer, NULL);
        }
        else
        {
            // Exit current supervisor and shutdown old start
            State = STATE_RESTARTING_START;
            StopSupervisor();
        }
    }

    // Create the Legato runtime directory if it doesn't already exist.
    LE_ASSERT(le_dir_Make(LE_CONFIG_RUNTIME_DIR, S_IRWXU | S_IXOTH) != LE_FAULT);

    // Properly label objects in tmpfs that are required by apps
    smack_SetLabel(LE_CONFIG_RUNTIME_DIR, "framework");
    smack_SetLabel("/tmp/ld.so.cache", "_");

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
    LE_CRIT("Set Child Subreaper not supported. Applications with forked processes may not shutdown properly.");
#endif


    // Initialize sub systems.
    user_Init();
    kernelModules_Init();
    smack_Init();

    SetupSmackOnlyCap();

    cgrp_Init();

    if (!fs_IsMountPoint(CURRENT_SYSTEM_PATH))
    {
        // Bind mount the root of the system unto itself so that we just lazy umount this when we
        // need to clean up.
        LE_CRIT_IF(mount(CURRENT_SYSTEM_PATH, CURRENT_SYSTEM_PATH, NULL, MS_BIND, NULL) != 0,
                    "Couldn't bind mount '%s' unto itself. %m", CURRENT_SYSTEM_PATH);
    }

    bool isReadOnly = sysStatus_IsReadOnly();

    // Check whether we are in Read-Only system. If system is Read-Only, then mount the overlay
    // over appsWriteable to work with Legato
    if (isReadOnly)
    {
        LE_INFO("System is read-only. Configuring 'appsWriteable' directory");
        // Create the directories to deploy the R/W upper layer
        (void)mkdir( "/tmp/appsWriteable", 0755 );
        (void)mkdir( "/tmp/appsWriteable_wk", 0755 );
        // Check if the upper layer is already mounted
        if (!fs_IsMountPoint(CURRENT_SYSTEM_PATH "/appsWriteable"))
        {
            // mount an R/W overlay
            if (mount("overlay", CURRENT_SYSTEM_PATH "/appsWriteable", "overlay", MS_SILENT,
                             "upperdir=/tmp/appsWriteable,"
                             "lowerdir=" CURRENT_SYSTEM_PATH "/appsWriteable,"
                             "workdir=/tmp/appsWriteable_wk") != 0)
            {
                LE_ERROR("Couldn't mount overlay R/W to '%s'. %m",
                         CURRENT_SYSTEM_PATH "/appsWriteable");
            }
        }
    }
    else
    {
        // Update the SMACK label for /legato depending if SMACK is enabled or disabled
        char* smackLegatoLabelPtr = (smack_IsEnabled() ? "framework" : "_");
        char smackCurrentLabel[PATH_MAX];
        bool isSmackLabelToSet = false;

        if (-1 == getxattr("/legato", "security.SMACK64",
                           smackCurrentLabel, sizeof(smackCurrentLabel)))
        {
            isSmackLabelToSet = (ENODATA == errno || ERANGE == errno);
        }
        else
        {
            isSmackLabelToSet = (0 != strcmp(smackCurrentLabel, smackLegatoLabelPtr));
        }

        LE_DEBUG("isSmackLabelToSet %d smackCurrentLabel = \"%s\" smackLegatoLabelPtr = \"%s\"",
                 isSmackLabelToSet, smackCurrentLabel, smackLegatoLabelPtr);
        if (isSmackLabelToSet)
        {
            LE_INFO("Updating SMACK label to \"%s\"", smackLegatoLabelPtr);
            if (-1 == setxattr("/legato", "security.SMACK64",
                               smackLegatoLabelPtr, strlen(smackLegatoLabelPtr), 0))
            {
                LE_CRIT("Could not set SMACK label for '/legato': %m.");
            }

            UpdateSmackLabelRecursive("/legato/systems", smackLegatoLabelPtr);
            UpdateSmackLabelRecursive("/legato/apps", smackLegatoLabelPtr);
        }
        else
        {
            LE_INFO("SMACK label \"%s\" is up to date", smackLegatoLabelPtr);
        }
    }

    if (!smack_IsEnabled())
    {
        // Try to umount /legato/smack as we do no use SMACK anymore at this point.
        umount("/legato/smack");
    }

    // Register a signal event handler for SIGCHLD so we know when processes die.
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    StartFramework();

    le_sig_Block(SIGCHLD);

    // All the framework daemons are active now. Now set the reboot expiry timer if it is not a
    // RO system.
    if (!isReadOnly)
    {
        LE_INFO("Not a read-only system. Configuring boot expire timer.");
        // If we have a writable system, create and start the quick-reboot timer.
        RebootTimer = le_timer_Create("Reboot");
        le_timer_SetHandler(RebootTimer, HandleRebootExpiry);
        le_timer_SetMsInterval(RebootTimer, GetBootExpirePeriod());
        le_timer_SetWakeup(RebootTimer, false);
        le_timer_Start(RebootTimer);
    }

    // Create or remove the SMACK_DISABLED file, which is used by the init scripts to determine to
    // set SMACK labels or not.
    // Ignore the EROFS in case of Legato is Read-Only
#if LE_CONFIG_ENABLE_SMACK
    // Remove SMACK_DISABLED.
    LE_FATAL_IF((unlink("/legato/SMACK_DISABLED") == -1) &&
                     ((errno != ENOENT) && (errno != EROFS)),
                "Cannot remove /legato/SMACK_DISABLED. %m.");
#else
    // Create SMACK_DISABLED.
    int fd;

    do
    {
        fd = open("/legato/SMACK_DISABLED", O_CREAT | O_EXCL | O_WRONLY, 0);
    }
    while ((fd == -1) && (errno == EINTR));

    LE_FATAL_IF((fd == -1) && ((errno != EEXIST) && (errno != EROFS)),
                "failed to create /legato/SMACK_DISABLED. %m.");

    if (fd != -1)
    {
        fd_Close(fd);
    }
#endif
}


//----------------------- Deprecated Functions -----------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Stops the Legato framework.
 *
 * Async API function.  Calls le_sup_ctrl_StopLegatoRespond() to report results.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StopLegato
(
    le_sup_ctrl_ServerCmdRef_t cmdRef
)
{
    LE_WARN("This API is deprecated.  Please use le_framework.api instead.");
    UsingDeprecatedSupCtrlApi = true;

    LE_DEBUG("Received request to stop Legato.");

    if (State != STATE_NORMAL)
    {
        le_sup_ctrl_StopLegatoRespond(cmdRef, LE_DUPLICATE);
    }
    else
    {
        // Save the command reference to use in the response later.
        StopApiCmdRef = (le_framework_ServerCmdRef_t)cmdRef;

        State = STATE_STOPPING;

        // Start the process of shutting down the framework.
        BeginShutdown();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Restarts the Legato framework.
 *
 * Async API function.  Calls le_sup_ctrl_RestartLegatoRespond() to report results.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_RestartLegato
(
    le_sup_ctrl_ServerCmdRef_t cmdRef,
    bool manualRestart
)
{
    LE_WARN("This API is deprecated.  Please use le_framework.api instead.");
    UsingDeprecatedSupCtrlApi = true;

    LE_DEBUG("Received request to restart Legato.");

    if (State == STATE_NORMAL)
    {
        // Save the command reference to use in the response later.
        StopApiCmdRef = (le_framework_ServerCmdRef_t)cmdRef;

        if (manualRestart)
        {
            State = STATE_RESTARTING_MANUAL;
        }
        else
        {
            State = STATE_RESTARTING;
        }

        // Start the process of shutting down the framework.
        BeginShutdown();
    }
    else
    {
        LE_DEBUG("Ignoring request to restart Legato in state %d.", State);

        le_sup_ctrl_RestartLegatoRespond(cmdRef, LE_DUPLICATE);
    }
}
