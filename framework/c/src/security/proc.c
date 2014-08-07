/** @file proc.c
 *
 * This is the process class that is used to reference the Supervisor's child processes in
 * applications.  This class has methods for starting and stopping processes and keeping process
 * state information.  However, a processes state must be updated by calling the
 * proc_SigChildHandler() from within a SIGCHILD handler.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 */
#include "legato.h"
#include "watchdogAction.h"
#include "proc.h"
#include "limit.h"
#include "le_cfg_interface.h"
#include "sandbox.h"
#include "resourceLimits.h"
#include "fileDescriptor.h"
#include "user.h"
#include <sys/resource.h>
#include <grp.h>


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's command-line arguments.
 *
 * The list of arguments is the commmand-line argument list used to start the process.  The first
 * argument in the list must be the absolute path (relative to the sandbox root) of the executable
 * file.
 *
 * If this entry in the config tree is missing or is empty, the process will fail to launch.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_ARGS                               "args"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's environment variables.
 *
 * Each item in the environment variables list must be a name=value pair.
 *
 * If this entry in the config tree is missing or is empty, no environment variables will be set.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_ENV_VARS                           "envVars"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's scheduling priority level.
 *
 * Possible values for the scheduling priorty are: "idle", "low", "medium", "high", "rt1"... "rt32".
 *
 * "idle"     - intended for very low priority processes that will only get CPU time if there are
 *              no other processes waiting for the CPU.
 *
 * "low",
 * "medium",
 * "high"     - intended for normal processes that contend for the CPU.  Processes with these
 *              priorities do not preempt each other but their priorities affect how they are
 *              inserted into the schdeduling queue. ie. "high" will get higher priority than
 *              "medium" when inserted into the queue.
 *
 * "rt1" to
 * "rt32"     - intended for (soft) realtime processes.  A higher realtime priority will pre-empt a
 *              lower realtime priority (ie. "rt2" would pre-empt "rt1").  Processes with any
 *              realtime priority will pre-empt processes with "high", "medium", "low" and "idle"
 *              priorities.  Also, note that processes with these realtime priorities will pre-empt
 *              the Legato framework processes so take care to design realtime processes that
 *              relinguishes the CPU appropriately.
 *
 *
 * If this entry in the config tree is missing or is empty, "medium" priority is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PRIORITY                       "priority"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the fault action for a process.
 *
 * The fault action value must be either IGNORE, RESTART, RESTART_APP, TERMINATE_APP or REBOOT.
 *
 * If this entry in the config tree is missing or is empty, APP_PROC_IGNORE is assumed.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_FAULT_ACTION                       "faultAction"

//--------------------------------------------------------------------------------------------------
/**
 * Fault action string definitions.
 */
//--------------------------------------------------------------------------------------------------
#define IGNORE_STR                          "ignore"
#define RESTART_STR                         "restart"
#define RESTART_APP_STR                     "restartApp"
#define STOP_APP_STR                        "stopApp"
#define REBOOT_STR                          "reboot"


//--------------------------------------------------------------------------------------------------
/**
 * Minimum and maximum realtime priority levels.
 */
//--------------------------------------------------------------------------------------------------
#define MIN_RT_PRIORITY                         1
#define MAX_RT_PRIORITY                         32


//--------------------------------------------------------------------------------------------------
/**
 * The number of string pointers needed when obtaining the command line arguments from the config
 * database.
 */
//--------------------------------------------------------------------------------------------------
#define NUM_ARGS_PTRS       LIMIT_MAX_NUM_CMD_LINE_ARGS + 3     // This is to accomodate the
                                                                // executable, process name and the
                                                                // NULL-terminator.


//--------------------------------------------------------------------------------------------------
/**
 * The process object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct proc_Ref
{
    char*           name;                                   // Name of the process.
    char            cfgPathRoot[LIMIT_MAX_PATH_BYTES];      // Our path in the config tree.
    char            appName[LIMIT_MAX_APP_NAME_BYTES];      // Name of the app that we are a part of.
    bool            paused;         // true if the process is paused.
    pid_t           pid;            // The pid of the process.
    time_t          faultTime;      // The time of the last fault.
    bool            cmdKill;        // true if the process was killed by proc_Stop().
    le_timer_Ref_t  timerRef;       // Timer used to allow the application to shutdown.
}
Process_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for process objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcessPool;


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
 * Nice level definitions for the different Legato priority levels.
 */
//--------------------------------------------------------------------------------------------------
#define LOW_PRIORITY_NICE_LEVEL         10
#define MEDIUM_PRIORITY_NICE_LEVEL      0
#define HIGH_PRIORITY_NICE_LEVEL        -10


//--------------------------------------------------------------------------------------------------
/**
 * Environment variable type.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            name[LIMIT_MAX_ENV_VAR_NAME_BYTES];     // The variable name.
    char            value[LIMIT_MAX_PATH_BYTES];            // The variable value.
}
EnvVar_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the process system.
 */
//--------------------------------------------------------------------------------------------------
void proc_Init
(
    void
)
{
    ProcessPool = le_mem_CreatePool("Procs", sizeof(Process_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a process object.
 *
 * @note The name of the process is the node name (last part) of the cfgPathRootPtr.
 *
 * @return
 *      A reference to a process object if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
proc_Ref_t proc_Create
(
    const char* cfgPathRootPtr,     ///< [IN] The path in the config tree for this process.
    const char* appNamePtr          ///< [IN] The name of the app that this process is a part of.
)
{
    Process_t* procPtr = le_mem_ForceAlloc(ProcessPool);

    // Copy the config path.
    if (le_utf8_Copy(procPtr->cfgPathRoot,
                     cfgPathRootPtr,
                     sizeof(procPtr->cfgPathRoot),
                     NULL) == LE_OVERFLOW)
    {
        LE_ERROR("Config path '%s' is too long.", cfgPathRootPtr);

        le_mem_Release(procPtr);
        return NULL;
    }


    // Copy the app name.
    if (le_utf8_Copy(procPtr->appName,
                     appNamePtr,
                     sizeof(procPtr->appName),
                     NULL) == LE_OVERFLOW)
    {
        LE_ERROR("App name '%s' is too long.", appNamePtr);

        le_mem_Release(procPtr);
        return NULL;
    }

    procPtr->name = le_path_GetBasenamePtr(procPtr->cfgPathRoot, "/");

    procPtr->faultTime = 0;
    procPtr->paused = false;
    procPtr->pid = -1;  // Processes that are not running are assigned -1 as its pid.
    procPtr->cmdKill = false;
    procPtr->timerRef = NULL;

    return procPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete the process object.  The process must be stopped before it is deleted.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void proc_Delete
(
    proc_Ref_t procRef              ///< [IN] The process to start.
)
{
    le_mem_Release(procRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the priority level for the specified process.
 *
 * The priority level string can be either "idle", "low", "medium", "high", "rt1" ... "rt32".
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_SetPriority
(
    const char* priorStr,   ///< [IN] Priority level string.
    pid_t pid               ///< [IN] PID of the process to set the priority for.
)
{
    // Declare these varialbes with the default values.
    struct sched_param priority = {.sched_priority = 0};
    int policy = SCHED_OTHER;
    int niceLevel = MEDIUM_PRIORITY_NICE_LEVEL;

    if (strcmp(priorStr, "idle") == 0)
    {
         policy = SCHED_IDLE;
    }
    else if (strcmp(priorStr, "low") == 0)
    {
        niceLevel = LOW_PRIORITY_NICE_LEVEL;
    }
    else if (strcmp(priorStr, "high") == 0)
    {
        niceLevel = HIGH_PRIORITY_NICE_LEVEL;
    }
    else if ( (priorStr[0] == 'r') && (priorStr[1] == 't') )
    {
        // Get the realtime level from the characters following "rt".
        char *endPtr;
        errno = 0;
        int level = strtol(&(priorStr[2]), &endPtr, 10);

        if ( (*endPtr != '\0') || (level < MIN_RT_PRIORITY) ||
             (level > MAX_RT_PRIORITY) )
        {
            LE_WARN("Unrecognized priority level (%s) for process '%d'.  Using default priority.",
                    priorStr, pid);
        }
        else
        {
            policy = SCHED_RR;
            priority.sched_priority = level;
        }
    }
    else if (strcmp(priorStr, "medium") != 0)
    {
        LE_WARN("Unrecognized priority level for process '%d'.  Using default priority.", pid);
    }

    // Set the policy and priority.
    if (sched_setscheduler(pid, policy, &priority) == -1)
    {
        LE_ERROR("Could not set the scheduling policy.  %m.");
        return LE_FAULT;
    }

    // Set the nice level.
    errno = 0;
    if (setpriority(PRIO_PROCESS, pid, niceLevel) == -1)
    {
        LE_ERROR("Could not set the nice level.  %m.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the scheduling policy, priority and/or nice level for the specified process based on the
 * process' configuration settings in the config tree.
 *
 * @note This function kills the specified process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void SetSchedulingPriority
(
    proc_Ref_t procRef      ///< [IN] The process to set the priority for.
)
{
    // Read the priority setting from the config tree.
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);

    char priorStr[LIMIT_MAX_PRIORITY_NAME_BYTES];

    if (le_cfg_GetString(procCfg, CFG_NODE_PRIORITY, priorStr, sizeof(priorStr), "medium") != LE_OK)
    {
        LE_CRIT("Priority string for process %s is too long.  Using default priority.", procRef->name);

        LE_ASSERT(le_utf8_Copy(priorStr, "medium", sizeof(priorStr), NULL) == LE_OK);
    }

    le_cfg_CancelTxn(procCfg);

    if (proc_SetPriority(priorStr, procRef->pid) != LE_OK)
    {
        LE_ASSERT(kill(procRef->pid, SIGKILL) == 0);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the environment variable from the list of environment variables in the config tree.
 *
 * @return
 *      The number of environment variables read from the config tree if successful.
 *      LE_NOT_FOUND if there are not environment variables.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetEnvironmentVariables
(
    proc_Ref_t procRef,     ///< [IN] The process to get the environment variables for.
    EnvVar_t envVars[],     ///< [IN] The list of environment variables.
    size_t maxNumEnvVars    ///< [IN] The maximum number of items envVars can hold.
)
{
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);
    le_cfg_GoToNode(procCfg, CFG_NODE_ENV_VARS);

    if (le_cfg_GoToFirstChild(procCfg) != LE_OK)
    {
        LE_WARN("No environment variables for process '%s'.", procRef->name);

        le_cfg_CancelTxn(procCfg);
        return LE_NOT_FOUND;
    }

    int i;
    for (i = 0; i < maxNumEnvVars; i++)
    {
        if ( (le_cfg_GetNodeName(procCfg, "", envVars[i].name, LIMIT_MAX_ENV_VAR_NAME_BYTES) != LE_OK) ||
             (le_cfg_GetString(procCfg, "", envVars[i].value, LIMIT_MAX_PATH_BYTES, "") != LE_OK) )
        {
            LE_ERROR("Error reading environment variables for process '%s'.", procRef->name);

            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }

        if (le_cfg_GoToNextSibling(procCfg) != LE_OK)
        {
            break;
        }
        else if (i >= maxNumEnvVars-1)
        {
            LE_ERROR("There were too many environment variables for process '%s'.", procRef->name);

            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }
    }

    le_cfg_CancelTxn(procCfg);
    return i + 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the environment variable for the calling process.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void SetEnvironmentVariables
(
    EnvVar_t envVars[],     ///< [IN] The list of environment variables.
    int numEnvVars          ///< [IN] The number environment variables in the list.
)
{
#define OVER_WRITE_ENV_VAR      1

    // Erase entire environment list.
    LE_ASSERT(clearenv() == 0);

    // Set the environment variables list.
    int i;
    for (i = 0; i < numEnvVars; i++)
    {
        // Set the environment variable, overwriting anything that was previously there.
        LE_ASSERT(setenv(envVars[i].name, envVars[i].value, OVER_WRITE_ENV_VAR) == 0);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the arguments list for this process.
 *
 * The program executable path will be the first element in the list.  The second element will be
 * the process name to for this process.  Subsequent elements in the list will contain command line
 * arguments for the process.  The list of arguments will be terminated by a NULL pointer.
 *
 * The arguments list will be passed out to the caller in argsPtr.
 *
 * The caller must provide a list of buffers, argsBuffers, that can be used to store the fetched
 * arguments.  Note that this buffer does not necessarily store the arguments in the correct order.
 * The caller should read argsPtr to see the proper arguments list.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetArgs
(
    proc_Ref_t procRef,             ///< [IN] The process to get the args for.
    char argsBuffers[LIMIT_MAX_NUM_CMD_LINE_ARGS][LIMIT_MAX_ARGS_STR_BYTES], ///< [OUT] A pointer to
                                                                             /// an array of buffers
                                                                             /// used to store
                                                                             /// arguments.
    char* argsPtr[NUM_ARGS_PTRS]    ///< [OUT] An array of pointers that will point to the valid
                                    ///       arguments list.  The list is terminated by NULL.
)
{
    // Get a config iterator to the arguments list.
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);
    le_cfg_GoToNode(procCfg, CFG_NODE_ARGS);

    if (le_cfg_GoToFirstChild(procCfg) != LE_OK)
    {
        LE_ERROR("No arguments for process '%s'.", procRef->name);
        le_cfg_CancelTxn(procCfg);
        return LE_FAULT;
    }

    size_t ptrIndex = 0;
    size_t bufIndex = 0;

    // Record the executable path.
    if (le_cfg_GetString(procCfg, "", argsBuffers[bufIndex], LIMIT_MAX_ARGS_STR_BYTES, "") != LE_OK)
    {
        LE_ERROR("Error reading argument '%s...' for process '%s'.",
                 argsBuffers[bufIndex],
                 procRef->name);

        le_cfg_CancelTxn(procCfg);
        return LE_FAULT;
    }

    argsPtr[ptrIndex++] = argsBuffers[bufIndex++];

    // Record the process name in the list.
    argsPtr[ptrIndex++] = procRef->name;

    // Record the arguments in the caller's list of buffers.
    while(1)
    {
        if (le_cfg_GoToNextSibling(procCfg) != LE_OK)
        {
            // Terminate the list.
            argsPtr[ptrIndex] = NULL;
            break;
        }
        else if (bufIndex >= LIMIT_MAX_NUM_CMD_LINE_ARGS)
        {
            LE_ERROR("Too many arguments for process '%s'.", procRef->name);
            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }

        if (le_cfg_IsEmpty(procCfg, ""))
        {
            LE_ERROR("Empty node in argument list for process '%s'.", procRef->name);

            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }

        if (le_cfg_GetString(procCfg, "", argsBuffers[bufIndex],
                             LIMIT_MAX_ARGS_STR_BYTES, "") != LE_OK)
        {
            LE_ERROR("Argument too long '%s...' for process '%s'.",
                     argsBuffers[bufIndex],
                     procRef->name);

            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }

        // Point to the string.
        argsPtr[ptrIndex++] = argsBuffers[bufIndex++];
    }

    le_cfg_CancelTxn(procCfg);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Configure non-sandboxed processes.
 */
//--------------------------------------------------------------------------------------------------
static void ConfigNonSandboxedProcess
(
    const char* workingDirPtr       ///< [IN] The path to the process's working directory.
)
{
    // Set the working directory for this process.
    LE_FATAL_IF(chdir(workingDirPtr) != 0,
                "Could not change working directory to '%s'.  %m", workingDirPtr);

    // NOTE: For now, at least, we run all unsandboxed apps as root to prevent major permissions
    //       issues when trying to perform system operations, such as changing routing tables.
    //       Consider using non-root users with capabilities later for another security layer.
}

//--------------------------------------------------------------------------------------------------
/**
 * Called when a process has not died due to a soft kill signal within the timeout period.
 * Handles soft kill timeout by performing a hard kill.
 */
//--------------------------------------------------------------------------------------------------
static void SoftKillExpiryHandler
(
    le_timer_Ref_t timerRef
)
{
    proc_Ref_t procRef = (proc_Ref_t)le_timer_GetContextPtr(timerRef);

    if (procRef->pid == -1)
    {
        LE_WARN("Process has already exited");
        return;
    }

    LE_WARN("Hard killing %d", procRef->pid);

    if (kill(procRef->pid, SIGKILL) == -1)
    {
        // Process could have exited while we haven't received the SIGCHLD yet
        // Determine if it's still alive
        LE_FATAL_IF( (kill(procRef->pid, 0) == 0),
                    "Could not send SIGKILL to process '%s' (PID: %d).  %m.",
                    procRef->name,
                    procRef->pid);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the process.
 *
 * If the sandboxDirPtr is not Null then the process will chroot to the sandboxDirPtr and
 * workingDirPtr is relative to the sandboxDirPtr.
 *
 * If sandboxDirPtr is NULL then the process will not be sandboxed and workingDirPtr is relative to
 * the current working directory of the calling process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static inline le_result_t StartProc
(
    proc_Ref_t procRef,             ///< [IN] The process to start.
    const char* workingDirPtr,      ///< [IN] The path to the process's working directory, relative
                                    ///       to the sandbox directory.
    uid_t uid,                      ///< [IN] The user ID to start the process as.
    gid_t gid,                      ///< [IN] The primary group ID for this process.
    const gid_t* groupsPtr,         ///< [IN] List of supplementary groups for this process.
    size_t numGroups,               ///< [IN] The number of groups in the supplementary groups list.
    const char* sandboxDirPtr       ///< [IN] The path to the root of the sandbox this process is to
                                    ///       run in.  If NULL then process will be unsandboxed.
)
{
#define READ_PIPE   0
#define WRITE_PIPE  1

    if (procRef->pid != -1)
    {
        LE_ERROR("Process '%s' (PID: %d) cannot be started because it is already running.",
                 procRef->name, procRef->pid);
        return LE_FAULT;
    }

    // Create a pipe for parent/child synchronization.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) == -1, "Could not create synchronization pipe.  %m.");

    // @Note The current IPC system does not support forking so any reads to the config DB must be
    //       done in the parent process.

    // Get the environment variables from the config tree for this process.
    EnvVar_t envVars[LIMIT_MAX_NUM_ENV_VARS];
    int numEnvVars = GetEnvironmentVariables(procRef, envVars, LIMIT_MAX_NUM_ENV_VARS);

    if (numEnvVars == LE_FAULT)
    {
        LE_ERROR("Error getting environment variables.  Process '%s' cannot be started.",
                 procRef->name);
        return LE_FAULT;
    }

    // Get the command line arguments from the config tree for this process.
    char argsBuffers[LIMIT_MAX_NUM_CMD_LINE_ARGS][LIMIT_MAX_ARGS_STR_BYTES];
    char* argsPtr[NUM_ARGS_PTRS];

    if (GetArgs(procRef, argsBuffers, argsPtr) != LE_OK)
    {
        LE_ERROR("Could not get command line arguments, process '%s' cannot be started.",
                 procRef->name);
        return LE_FAULT;
    }

    // Create the child process
    pid_t pID = fork();

    if (pID < 0)
    {
        LE_EMERG("Failed to fork.  %m.");
        return LE_FAULT;
    }

    if (pID == 0)
    {
        freopen("/dev/console", "a", stdout);
        freopen("/dev/console", "a", stderr);

        // Set the umask so that files are not accidentally created with global permissions.
        umask(S_IRWXG | S_IRWXO);

        // Unblock all signals that might have been blocked.
        sigset_t sigSet;
        LE_ASSERT(sigfillset(&sigSet) == 0);
        LE_ASSERT(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) == 0);

        SetEnvironmentVariables(envVars, numEnvVars);

        // Wait for the parent to allow us to continue by blocking on the read pipe until it
        // is closed.
        fd_Close(syncPipeFd[WRITE_PIPE]);

        ssize_t numBytesRead;
        int dummyBuf;
        do
        {
            numBytesRead = read(syncPipeFd[READ_PIPE], &dummyBuf, 1);
        }
        while ( ((numBytesRead == -1)  && (errno == EINTR)) || (numBytesRead != 0) );

        LE_FATAL_IF(numBytesRead == -1, "Could not read synchronization pipe.  %m.");

        // The parent has allowed us to continue.

        // Close all non-standard file descriptors.
        fd_CloseAllNonStd();

        if (sandboxDirPtr != NULL)
        {
            // Sandbox the process.
            sandbox_ConfineProc(sandboxDirPtr, uid, gid, groupsPtr, numGroups, workingDirPtr);
        }
        else
        {
            ConfigNonSandboxedProcess(workingDirPtr);
        }

        // Launch the child program.  This should not return unless there was an error.
        LE_INFO("Execing '%s'", argsPtr[0]);
        execvp(argsPtr[0], &(argsPtr[1]));

        // The program could not be started.  Log an error message.
        LE_FATAL("Could not exec '%s'.  %m.", argsPtr[0]);
    }

    procRef->pid = pID;
    procRef->paused = false;

    // Don't need this end of the pipe.
    fd_Close(syncPipeFd[READ_PIPE]);

    // Set the scheduling priority for the child process while the child process is blocked.
    SetSchedulingPriority(procRef);

    // Set the resource limits for the child process while the child process is blocked.
    if (resLim_SetProcLimits(procRef) != LE_OK)
    {
        LE_ERROR("Could not set the resource limits.  %m.");
        LE_ASSERT(kill(procRef->pid, SIGKILL) == 0);
    }

    LE_INFO("Starting process %s with pid %d", procRef->name, procRef->pid);

    // Unblock the child process.
    fd_Close(syncPipeFd[WRITE_PIPE]);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process, running as the root user, in a given working directory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_Start
(
    proc_Ref_t procRef,             ///< [IN] The process to start.
    const char* workingDirPtr       ///< [IN] The path to the process's working directory, relative
                                    ///       to the sandbox directory.
)
{
    return proc_StartInSandbox(procRef,
                               workingDirPtr,
                               0,
                               0,
                               NULL,
                               0,
                               NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the process in a sandbox.
 *
 * The process will chroot to the sandboxDirPtr and assume the workingDirPtr is relative to the
 * sandboxDirPtr.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_StartInSandbox
(
    proc_Ref_t procRef,             ///< [IN] The process to start.
    const char* workingDirPtr,      ///< [IN] The path to the process's working directory, relative
                                    ///       to the sandbox directory.
    uid_t uid,                      ///< [IN] The user ID to start the process as.
    gid_t gid,                      ///< [IN] The primary group ID for this process.
    const gid_t* groupsPtr,         ///< [IN] List of supplementary groups for this process.
    size_t numGroups,               ///< [IN] The number of groups in the supplementary groups list.
    const char* sandboxDirPtr       ///< [IN] The path to the root of the sandbox this process is to
                                    ///       run in.  If NULL then process will be unsandboxed.
)
{
    return StartProc(procRef,
                     workingDirPtr,
                     uid,
                     gid,
                     groupsPtr,
                     numGroups,
                     sandboxDirPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops (kills) the process.  This is an asynchronous function call that returns immediately but
 * the process state may not be updated right away.  Set a state change handler using
 * proc_SetStateChangeHandler to get notified when the process actually dies.
 */
//--------------------------------------------------------------------------------------------------
void proc_Stop
(
    proc_Ref_t procRef              ///< [IN] The process to stop.
)
{
    LE_ASSERT(procRef->pid != -1);

    LE_DEBUG("Soft killing %d", procRef->pid);

    if (kill(procRef->pid, SIGTERM) == -1)
    {
        LE_FATAL("Could not send SIGTERM to process '%s' (PID: %d).  %m.",
                 procRef->name,
                 procRef->pid);
    }

    LE_ASSERT(procRef->timerRef == NULL);

    {
        char timerName[30];

        snprintf(timerName, NUM_ARRAY_MEMBERS(timerName), "%d killer", procRef->pid);
        procRef->timerRef = le_timer_Create(timerName);

        le_timer_SetInterval(procRef->timerRef, KillTimeout);

        le_timer_SetContextPtr(procRef->timerRef, (void*)procRef);
        le_timer_SetHandler(procRef->timerRef, SoftKillExpiryHandler);

        le_timer_Start(procRef->timerRef);
    }

    // Set this flag to indicate that the process was intentionally killed and its fault action
    // should not be respected.
    procRef->cmdKill = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Pause the running process.  This is an asynchronous function call that returns immediately but
 * the process state may not be updated right away.  Set a state change handler using
 * proc_SetStateChangeHandler to get notified when the process actually pauses.
 */
//--------------------------------------------------------------------------------------------------
void proc_Pause
(
    proc_Ref_t procRef              ///< [IN] The process to pause.
)
{
    LE_ASSERT(procRef->pid != -1);

    if (kill(procRef->pid, SIGSTOP) == -1)
    {
        LE_FATAL("Could not send SIGSTOP to process '%s' (PID: %d).  %m.", procRef->name, procRef->pid);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Resume the running process.  This is an asynchronous function call that returns immediately but
 * the process state may not be updated right away.  Set a state change handler using
 * proc_SetStateChangeHandler to get notified when the process actually resumes.
 */
//--------------------------------------------------------------------------------------------------
void proc_Resume
(
    proc_Ref_t procRef              ///< [IN] The process to resume.
)
{
    LE_ASSERT(procRef->pid != -1);

    if (kill(procRef->pid, SIGCONT) == -1)
    {
        LE_FATAL("Could not send SIGCONT to process '%s' (PID: %d).  %m.", procRef->name, procRef->pid);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process state.
 *
 * @return
 *      The process state.
 */
//--------------------------------------------------------------------------------------------------
proc_State_t proc_GetState
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    if (procRef->pid == -1)
    {
        return PROC_STATE_STOPPED;
    }
    else if (!(procRef->paused))
    {
        return PROC_STATE_RUNNING;
    }
    else
    {
        return PROC_STATE_PAUSED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the process's PID.
 *
 * @return
 *      The process's PID if the state is not PROC_STATE_DEAD.
 *      -1 if the state is PROC_STATE_DEAD.
 */
//--------------------------------------------------------------------------------------------------
pid_t proc_GetPID
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return procRef->pid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's name.
 *
 * @return
 *      The process's name.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetName
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return (const char*)procRef->name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the application that this process belongs to.
 *
 * @return
 *      The application name.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetAppName
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return (const char*)procRef->appName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's previous fault time.
 *
 * @return
 *      The process's name.
 */
//--------------------------------------------------------------------------------------------------
time_t proc_GetFaultTime
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return procRef->faultTime;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's config path.
 *
 * @return
 *      The process's config path.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetConfigPath
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return (const char*)procRef->cfgPathRoot;
}


//--------------------------------------------------------------------------------------------------
/**
 * Determines if the process is a realtime process.
 *
 * @return
 *      true if the process has realtime priority.
 *      false if the process does not have realtime priority.
 */
//--------------------------------------------------------------------------------------------------
proc_State_t proc_IsRealtime
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    // Read the priority setting from the config tree.
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);

    char priorStr[LIMIT_MAX_PRIORITY_NAME_BYTES];
    le_result_t result = le_cfg_GetString(procCfg,
                                          CFG_NODE_PRIORITY,
                                          priorStr,
                                          sizeof(priorStr),
                                          "medium");

    le_cfg_CancelTxn(procCfg);

    if ( (result == LE_OK) &&
         (priorStr[0] == 'r') && (priorStr[1] == 't') )
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the fault action for the process.
 *
 * @return
 *      The fault action.
 */
//--------------------------------------------------------------------------------------------------
static proc_FaultAction_t GetFaultAction
(
    proc_Ref_t procRef              ///< [IN] The process reference.
)
{
    if (procRef->cmdKill)
    {
        // The cmdKill flag was set which means the process died because we killed it so
        // it was not a fault.  Reset the cmdKill flag so that if this process is restarted
        // faults will still be caught.
        procRef->cmdKill = false;

        return PROC_FAULT_ACTION_NO_FAULT;
    }

    // Record the fault time.
    procRef->faultTime = (le_clk_GetAbsoluteTime()).sec;

    // Read the process's fault action from the config tree.
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);

    char faultActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
    le_result_t result = le_cfg_GetString(procCfg, CFG_NODE_FAULT_ACTION,
                                          faultActionStr, sizeof(faultActionStr), "");

    le_cfg_CancelTxn(procCfg);

    // Set the fault action based on the fault action string.
    if (result != LE_OK)
    {
        LE_CRIT("Fault action string for process '%s' is too long.  Assume fault action is 'ignore'.",
                procRef->name);
        return PROC_FAULT_ACTION_IGNORE;
    }

    if (strcmp(faultActionStr, RESTART_STR) == 0)
    {
        return PROC_FAULT_ACTION_RESTART;
    }

    if (strcmp(faultActionStr, RESTART_APP_STR) == 0)
    {
        return PROC_FAULT_ACTION_RESTART_APP;
    }

    if (strcmp(faultActionStr, STOP_APP_STR) == 0)
    {
        return PROC_FAULT_ACTION_STOP_APP;
    }

    if (strcmp(faultActionStr, REBOOT_STR) == 0)
    {
        return PROC_FAULT_ACTION_REBOOT;
    }

    if (strcmp(faultActionStr, IGNORE_STR) == 0)
    {
        return PROC_FAULT_ACTION_IGNORE;
    }

    LE_WARN("Unrecognized fault action for process '%s'.  Assume fault action is 'ignore'.",
            procRef->name);
    return PROC_FAULT_ACTION_IGNORE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the watchdog action for this process.
 *
 * @return
 *      The watchdog action that should be taken for this process or one of the following:
 *          WATCHDOG_ACTION_NOT_FOUND - no action was configured for this process
 *          WATCHDOG_ACTION_ERROR     - the action could not be read or is unknown
 *          WATCHDOG_ACTION_HANDLED   - no further action is required, it is already handled.
 */
//--------------------------------------------------------------------------------------------------
proc_FaultAction_t proc_GetWatchdogAction
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    // No actions are performed here. This just looks up the action for this process.
    // The result is passed back up to app to handle as with fault action.
    wdog_action_WatchdogAction_t watchdogAction = WATCHDOG_ACTION_NOT_FOUND;
    {

        if (procRef->paused)
        {
            return WATCHDOG_ACTION_HANDLED;
        };

        // Read the process's fault action from the config tree.
        le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathRoot);

        char watchdogActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
        le_result_t result = le_cfg_GetString(procCfg, wdog_action_GetConfigNode(),
                watchdogActionStr, sizeof(watchdogActionStr), "");

        le_cfg_CancelTxn(procCfg);

        // Set the watchdog action based on the fault action string.
        if (result == LE_OK)
        {
            LE_WARN("%s watchdogAction '%s' in proc section", procRef->name, watchdogActionStr);
            watchdogAction = wdog_action_EnumFromString(watchdogActionStr);
            if (WATCHDOG_ACTION_ERROR == watchdogAction)
            {
                LE_WARN("%s watchdogAction '%s' unknown", procRef->name, watchdogActionStr);
            }
        }
        else
        {
            LE_CRIT("Watchdog action string for process '%s' is too long.",
                    procRef->name);
            watchdogAction = WATCHDOG_ACTION_ERROR;
        }
    }

    return watchdogAction;
}

//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for the specified process.
 *
 * @return
 *      The fault action that should be taken for this process.
 */
//--------------------------------------------------------------------------------------------------
proc_FaultAction_t proc_SigChildHandler
(
    proc_Ref_t procRef,             ///< [IN] The process reference.
    int procExitStatus              ///< [IN] The status of the process given by wait().
)
{
    proc_FaultAction_t faultAction = PROC_FAULT_ACTION_NO_FAULT;

    if (WIFSTOPPED(procExitStatus))
    {
        procRef->paused = true;

        LE_INFO("Process '%s' (PID: %d) has paused.", procRef->name, procRef->pid);
    }
    else if (WIFCONTINUED(procExitStatus))
    {
        procRef->paused = false;

        LE_INFO("Process '%s' (PID: %d) has been continued.", procRef->name, procRef->pid);
    }
    else
    {
        // The process died.

        if (procRef->timerRef != NULL)
        {
           le_timer_Delete(procRef->timerRef);
           procRef->timerRef = NULL;
        }

        if (WIFEXITED(procExitStatus))
        {
            LE_INFO("Process '%s' (PID: %d) has exited with exit code %d.",
                    procRef->name, procRef->pid, WEXITSTATUS(procExitStatus));

            if (WEXITSTATUS(procExitStatus) != EXIT_SUCCESS)
            {
                faultAction = GetFaultAction(procRef);
            }
        }
        else if (WIFSIGNALED(procExitStatus))
        {
            LE_INFO("Process '%s' (PID: %d) has exited due to signal %d.",
                    procRef->name, procRef->pid, WTERMSIG(procExitStatus));

            faultAction = GetFaultAction(procRef);
        }

        procRef->pid = -1;
        procRef->paused = false;
    }

    return faultAction;
}
