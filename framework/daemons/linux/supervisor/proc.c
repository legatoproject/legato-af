//--------------------------------------------------------------------------------------------------
/** @file supervisor/proc.c
 *
 * This is the process class that is used to reference the Supervisor's child processes in
 * applications.  This class has methods for starting and stopping processes and keeping process
 * state information.  However, a processes state must be updated by calling the
 * proc_SigChildHandler() from within a SIGCHILD handler.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "app.h"
#include "proc.h"
#include "resourceLimits.h"
#include "supervisor.h"
#include "watchdogAction.h"

#include "fileDescriptor.h"
#include "interfaces.h"
#include "killProc.h"
#include "le_cfg_interface.h"
#include "limit.h"
#include "linux/logPlatform.h"
#include "smack.h"
#include "sysStatus.h"
#include "user.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's command-line arguments.
 *
 * The list of arguments is the command-line argument list used to start the process.  The first
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
 *              relinquishes the CPU appropriately.
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
 * The name in the config tree that contains the watchdog action for the process
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_WDOG_ACTION "watchdogAction"


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
    char*   namePtr;                ///< Name of the process.
    char*   cfgPathPtr;             ///< Path in the config tree. If NULL use default settings.
    app_Ref_t appRef;               ///< Reference to the app that we are part of.
    pid_t   pid;                    ///< The pid of the process.
    time_t  faultTime;              ///< The time of the last fault.
    bool    cmdKill;                ///< true if the process was killed by proc_Stop().
    int     stdInFd;                ///< Fd to direct standard in to.  If -1 then use /dev/null.
    int     stdOutFd;               ///< Fd to direct standard out to.  If -1 then use /dev/null.
    int     stdErrFd;               ///< Fd to direct standard error to.  If -1 then use /dev/null.
    char*   execPathPtr;            ///< Executable path override.
    char*   priorityPtr;            ///< Priority string override.
    le_sls_List_t argsList;         ///< Arguments list override.
    bool    argsListValid;          ///< Arguments list override valid flag.  true if argsList is
                                    ///  valid (possibly empty).
    FaultAction_t faultAction;      ///< Fault action.
    FaultAction_t defaultFaultAction;///< Default fault action from config tree.
    wdog_action_WatchdogAction_t watchdogAction;///< Watchdog action.
    bool    run;                    ///< run override.
    bool    debug;                  ///< Should be started in debugger
    int     blockPipe;              ///< Write end of a pipe to the actual child process.  Used to
                                    ///  control blocking of the child process.
    proc_BlockCallback_t  blockCallback;  ///< Callback function to indicate when the process is
                                          ///  has been blocked after the fork but before the exec.
    void* blockContextPtr;          ///< Context pointer for the blockCallback.
}
Process_t;


//--------------------------------------------------------------------------------------------------
/**
 * Argument object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            argument[LIMIT_MAX_ARGS_STR_BYTES]; ///< Argument string.
    le_sls_Link_t   link;                               ///< Link in a list of arguments.
}
Arg_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for process objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcessPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for path and name strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PathPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for priority strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PriorityPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for argument strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ArgsPool;


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
 * Definitions for the read and write ends of a pipe.
 */
//--------------------------------------------------------------------------------------------------
#define READ_PIPE       0
#define WRITE_PIPE      1


//--------------------------------------------------------------------------------------------------
/**
 * The fault limits.
 *
 * @todo Put in the config tree so that it can be configured.
 */
//--------------------------------------------------------------------------------------------------
#define FAULT_LIMIT_INTERVAL_RESTART                10   // in seconds
#define FAULT_LIMIT_INTERVAL_RESTART_APP            10   // in seconds


//--------------------------------------------------------------------------------------------------
/**
 * Gets the fault action for the process from the config tree and store in the process record
 */
//--------------------------------------------------------------------------------------------------
static void GetFaultAction
(
    proc_Ref_t procRef,              ///< [IN] The process reference.
    le_cfg_IteratorRef_t procCfg     ///< [IN] The process config tree reference
)
{
    if (procCfg == NULL)
    {
        // Use the default fault action.
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_IGNORE;
        return;
    }

    char faultActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
    le_result_t result = le_cfg_GetString(procCfg, CFG_NODE_FAULT_ACTION,
                                          faultActionStr, sizeof(faultActionStr), "");

    // Set the fault action based on the fault action string.
    if (result != LE_OK)
    {
        LE_CRIT("Fault action string for process '%s' is too long.  Assume 'ignore'.",
                procRef->namePtr);
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_IGNORE;
        return;
    }

    if (strcmp(faultActionStr, RESTART_STR) == 0)
    {
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_RESTART_PROC;
        return;
    }

    if (strcmp(faultActionStr, RESTART_APP_STR) == 0)
    {
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_RESTART_APP;
        return;
    }

    if (strcmp(faultActionStr, STOP_APP_STR) == 0)
    {
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_STOP_APP;
        return;
    }

    if (strcmp(faultActionStr, REBOOT_STR) == 0)
    {
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_REBOOT;
        return;
    }

    if (strcmp(faultActionStr, IGNORE_STR) == 0)
    {
        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_IGNORE;
        return;
    }

    if (faultActionStr[0] == '\0')  // If no fault action is specified,
    {
        LE_INFO("No fault action specified for process '%s'. Assuming 'ignore'.",
                procRef->namePtr);

        procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_IGNORE;
        return;
    }

    LE_WARN("Unrecognized fault action for process '%s'.  Assume 'ignore'.",
            procRef->namePtr);
    procRef->defaultFaultAction = procRef->faultAction = FAULT_ACTION_IGNORE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the watchdog action for the process from the config tree and store in the process record
 */
//--------------------------------------------------------------------------------------------------
static void GetWatchdogAction
(
    proc_Ref_t procRef,              ///< [IN] The process reference.
    le_cfg_IteratorRef_t procCfg     ///< [IN] The process config tree reference
)
{
    if (procCfg == NULL)
    {
        procRef->watchdogAction = WATCHDOG_ACTION_NOT_FOUND;
    }
    else
    {
        char watchdogActionStr[LIMIT_MAX_FAULT_ACTION_NAME_BYTES];
        le_result_t result = le_cfg_GetString(procCfg, CFG_NODE_WDOG_ACTION,
                                              watchdogActionStr, sizeof(watchdogActionStr), "");

        // Set the watchdog action based on the fault action string.
        if (result == LE_OK)
        {
            LE_WARN("%s watchdogAction '%s' in proc section", procRef->namePtr, watchdogActionStr);
            procRef->watchdogAction = wdog_action_EnumFromString(watchdogActionStr);
        }
        else
        {
            LE_CRIT("Watchdog action string for process '%s' is too long.",
                    procRef->namePtr);
            procRef->watchdogAction = WATCHDOG_ACTION_ERROR;
        }
    }
}


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
    PathPool = le_mem_CreatePool("Paths", LIMIT_MAX_PATH_BYTES);
    PriorityPool = le_mem_CreatePool("Priority", LIMIT_MAX_PRIORITY_NAME_BYTES);
    ArgsPool = le_mem_CreatePool("Args", sizeof(Arg_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a process object.
 *
 * @note If the config path is given, the last node in the path must be the name of the process.
 *
 * @return
 *      A reference to a process object if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
proc_Ref_t proc_Create
(
    const char* namePtr,            ///< [IN] Name of the process.
    app_Ref_t appRef,               ///< [IN] Reference to the app that we are part of.
    const char* cfgPathRootPtr      ///< [IN] The path in the config tree for this process.  Can be
                                    ///       NULL if there is no config entry for this process.
)
{
    Process_t* procPtr = le_mem_ForceAlloc(ProcessPool);

    if (cfgPathRootPtr != NULL)
    {
        procPtr->cfgPathPtr = le_mem_ForceAlloc(PathPool);

        // Copy the config path to our buffer.
        if (le_utf8_Copy(procPtr->cfgPathPtr,
                         cfgPathRootPtr,
                         LIMIT_MAX_PATH_BYTES,
                         NULL) == LE_OVERFLOW)
        {
            LE_ERROR("Config path '%s' is too long.", cfgPathRootPtr);

            le_mem_Release(procPtr->cfgPathPtr);
            le_mem_Release(procPtr);
            return NULL;
        }

        // The name of the process is the node name (last part) of the cfgPathRootPtr.
        procPtr->namePtr = le_path_GetBasenamePtr(procPtr->cfgPathPtr, "/");
    }
    else
    {
        procPtr->namePtr = le_mem_ForceAlloc(PathPool);

        // Copy the name to our buffer
        if (le_utf8_Copy(procPtr->namePtr,
                         namePtr,
                         LIMIT_MAX_PATH_BYTES,
                         NULL) == LE_OVERFLOW)
        {
            LE_ERROR("Process name '%s' is too long.", namePtr);

            le_mem_Release(procPtr->namePtr);
            le_mem_Release(procPtr);
            return NULL;
        }

        procPtr->cfgPathPtr = NULL;
    }

    // Initialize all other parameters.
    procPtr->appRef = appRef;
    procPtr->faultTime = 0;
    procPtr->pid = -1;  // Processes that are not running are assigned -1 as its pid.
    procPtr->cmdKill = false;

    // Default to using /dev/null for standard streams.
    procPtr->stdInFd = -1;
    procPtr->stdOutFd = -1;
    procPtr->stdErrFd = -1;

    procPtr->execPathPtr = NULL;
    procPtr->priorityPtr = NULL;
    procPtr->argsListValid = false;
    procPtr->argsList = LE_SLS_LIST_INIT;
    procPtr->run = true;
    procPtr->debug = false;
    procPtr->blockPipe = -1;
    procPtr->blockCallback = NULL;
    procPtr->blockContextPtr = NULL;

    // Get watchdog action & fault action from config tree now, if this process has a config
    // tree entry.
    //
    // Since something will be going
    // wrong when these are used, we don't want to rely on the config tree being available.
    le_cfg_IteratorRef_t procCfg = NULL;
    if (procPtr->cfgPathPtr != NULL)
    {
        procCfg = le_cfg_CreateReadTxn(procPtr->cfgPathPtr);
    }
    GetFaultAction(procPtr, procCfg);
    GetWatchdogAction(procPtr, procCfg);
    if (procCfg)
    {
        le_cfg_CancelTxn(procCfg);
    }

    // If watchdog action isn't available in process environment, get it from the app environment.
    if ((WATCHDOG_ACTION_NOT_FOUND == procPtr->watchdogAction ||
         WATCHDOG_ACTION_ERROR == procPtr->watchdogAction))
    {
        const char* appCfgPath = app_GetConfigPath(appRef);

        if (NULL != appCfgPath)
        {
            LE_DEBUG("Getting watchdog action for process '%s' from app '%s'",
                     procPtr->namePtr, app_GetName(appRef));
            le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(appCfgPath);
            GetWatchdogAction(procPtr, appCfg);
            le_cfg_CancelTxn(appCfg);
        }
    }

    return procPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete the process object.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void proc_Delete
(
    proc_Ref_t procRef              ///< [IN] The process to start.
)
{
    // Delete arguments override list.
    proc_ClearArgs(procRef);

    // Close any open file descriptors.
    if (procRef->stdInFd != -1)
    {
        fd_Close(procRef->stdInFd);
    }

    if (procRef->stdOutFd != -1)
    {
        fd_Close(procRef->stdOutFd);
    }

    if (procRef->stdErrFd != -1)
    {
        fd_Close(procRef->stdErrFd);
    }

    if (procRef->blockPipe != -1)
    {
        fd_Close(procRef->blockPipe);
    }

    // Delete priority override string.
    if (procRef->priorityPtr != NULL)
    {
        le_mem_Release(procRef->priorityPtr);
    }

    // Delete executable override path.
    if (procRef->execPathPtr != NULL)
    {
        le_mem_Release(procRef->execPathPtr);
    }

    // Delete config path or name.
    if (procRef->cfgPathPtr != NULL)
    {
        le_mem_Release(procRef->cfgPathPtr);
    }
    else
    {
        le_mem_Release(procRef->namePtr);
    }

    // Release the process object.
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
le_result_t SetProcPriority
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

        // Set no limits for realtime processes to allow processes to increase their nice level if
        // the change the policy to be non-realtime later.
        // TODO: Set nice and priority limits according to configured limits.
        struct rlimit lim = {RLIM_INFINITY, RLIM_INFINITY};

        LE_ERROR_IF(prlimit(pid, RLIMIT_NICE, &lim, NULL) == -1,
                    "Could not set nice limit.  %m.");
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
 * Sets the scheduling policy, priority and/or nice level for the specified process.
 *
 * @note This function kills the specified process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void SetSchedulingPriority
(
    proc_Ref_t procRef      ///< [IN] The process to set the priority for.
)
{
    char priorStr[LIMIT_MAX_PRIORITY_NAME_BYTES] = "medium";
    char* priorStrPtr = priorStr;

    if (procRef->priorityPtr != NULL)
    {
        priorStrPtr = procRef->priorityPtr;
    }
    else if (procRef->cfgPathPtr != NULL)
    {
        // Read the priority setting from the config tree.
        le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathPtr);

        if (le_cfg_GetString(procCfg, CFG_NODE_PRIORITY, priorStr, sizeof(priorStr), "medium") != LE_OK)
        {
            LE_CRIT("Priority string for process %s is too long.  Using default priority.", procRef->namePtr);

            LE_ASSERT(le_utf8_Copy(priorStr, "medium", sizeof(priorStr), NULL) == LE_OK);
        }

        le_cfg_CancelTxn(procCfg);
    }

    if (SetProcPriority(priorStrPtr, procRef->pid) != LE_OK)
    {
        kill_Hard(procRef->pid);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the environment variable from the list of environment variables in the config tree.
 *
 * @return
 *      The number of environment variables read from the config tree if successful.
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
    int numEnvVars = 0;

    if (procRef->cfgPathPtr != NULL)
    {
        le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathPtr);
        le_cfg_GoToNode(procCfg, CFG_NODE_ENV_VARS);

        if (le_cfg_GoToFirstChild(procCfg) != LE_OK)
        {
            LE_WARN("No environment variables for process '%s'.", procRef->namePtr);

            le_cfg_CancelTxn(procCfg);
            return 0;
        }

        int i = 0;
        for (i = 0; i < maxNumEnvVars; i++)
        {
            if ( (le_cfg_GetNodeName(procCfg, "", envVars[i].name, LIMIT_MAX_ENV_VAR_NAME_BYTES) != LE_OK) ||
                 (le_cfg_GetString(procCfg, "", envVars[i].value, LIMIT_MAX_PATH_BYTES, "") != LE_OK) )
            {
                le_cfg_CancelTxn(procCfg);
                goto errorReading;
            }

            if (le_cfg_GoToNextSibling(procCfg) != LE_OK)
            {
                break;
            }
            else if (i >= maxNumEnvVars-1)
            {
                le_cfg_CancelTxn(procCfg);
                goto errorReading;
            }
        }

        le_cfg_CancelTxn(procCfg);

        numEnvVars = i + 1;
    }
    // If the config path is NULL (likely because the process is auxiliary and thus "unconfigured"),
    // then default PATH is provided depending on the app is sandboxed or not. This default PATH is
    // the same as the one written to the config during app build-time.
    else
    {
        size_t size = sizeof(envVars[0].name);
        if (snprintf(envVars[0].name, size, "PATH") >= size)
        {
            goto errorReading;
        }

        size = sizeof(envVars[0].value);
        if (app_GetIsSandboxed(procRef->appRef))
        {
            if (snprintf(envVars[0].value, size, "/usr/local/bin:/usr/bin:/bin") >= size)
            {
                goto errorReading;
            }
        }
        else
        {
            const char* appName = app_GetName(procRef->appRef);

            if (snprintf(envVars[0].value, size,
                         "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:"
                         "/legato/systems/current/appsWriteable/%s/bin:"
                         "/legato/systems/current/appsWriteable/%s/usr/bin:"
                         "/legato/systems/current/appsWriteable/%s/usr/local/bin",
                         appName, appName, appName) >= size)
            {
                goto errorReading;
            }
        }

        numEnvVars = 1;
    }

    return numEnvVars;

errorReading:

    LE_ERROR("Error reading environment variables for process '%s'.", procRef->namePtr);
    return LE_FAULT;
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
#define INDEX_EXEC      0
#define INDEX_PROC      INDEX_EXEC + 1
#define INDEX_ARGS      INDEX_PROC + 1

    size_t ptrIndex = 0;
    size_t bufIndex = 0;

    // Initialize the executable path.
    argsPtr[INDEX_EXEC] = procRef->execPathPtr;

    // Initialize the process name.
    argsPtr[INDEX_PROC] = procRef->namePtr;

    // Initialized the arguments.
    if (procRef->argsListValid)
    {
        le_sls_Link_t* argLinkPtr = le_sls_Peek(&(procRef->argsList));

        while (argLinkPtr != NULL)
        {
            Arg_t* argPtr = CONTAINER_OF(argLinkPtr, Arg_t, link);

            argsPtr[INDEX_ARGS + ptrIndex] = argPtr->argument;
            ptrIndex++;

            argLinkPtr = le_sls_PeekNext(&(procRef->argsList), argLinkPtr);
        }
    }

    // Set the executable and the args if necessary.
    if (procRef->cfgPathPtr != NULL)
    {
        // Get a config iterator to the arguments list.
        le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathPtr);
        le_cfg_GoToNode(procCfg, CFG_NODE_ARGS);

        if (le_cfg_GoToFirstChild(procCfg) != LE_OK)
        {
            LE_ERROR("No arguments for process '%s'.", procRef->namePtr);
            le_cfg_CancelTxn(procCfg);
            return LE_FAULT;
        }

        // Record the executable path.
        if (procRef->execPathPtr == NULL)
        {
            if (le_cfg_GetString(procCfg, "", argsBuffers[bufIndex],
                                 LIMIT_MAX_ARGS_STR_BYTES, "") != LE_OK)
            {
                LE_ERROR("Error reading argument '%s...' for process '%s'.",
                         argsBuffers[bufIndex],
                         procRef->namePtr);

                le_cfg_CancelTxn(procCfg);
                return LE_FAULT;
            }

            argsPtr[INDEX_EXEC] = argsBuffers[bufIndex];
            bufIndex++;
        }

        // Record the arguments in the caller's list of buffers.
        if (!procRef->argsListValid)
        {
            ptrIndex = 0;

            while(1)
            {
                if (le_cfg_GoToNextSibling(procCfg) != LE_OK)
                {
                    break;
                }
                else if (bufIndex >= LIMIT_MAX_NUM_CMD_LINE_ARGS)
                {
                    LE_ERROR("Too many arguments for process '%s'.", procRef->namePtr);
                    le_cfg_CancelTxn(procCfg);
                    return LE_FAULT;
                }

                if (le_cfg_IsEmpty(procCfg, ""))
                {
                    LE_ERROR("Empty node in argument list for process '%s'.", procRef->namePtr);

                    le_cfg_CancelTxn(procCfg);
                    return LE_FAULT;
                }

                if (le_cfg_GetString(procCfg, "", argsBuffers[bufIndex],
                                     LIMIT_MAX_ARGS_STR_BYTES, "") != LE_OK)
                {
                    LE_ERROR("Argument too long '%s...' for process '%s'.",
                             argsBuffers[bufIndex],
                             procRef->namePtr);

                    le_cfg_CancelTxn(procCfg);
                    return LE_FAULT;
                }

                // Point to the string.
                argsPtr[INDEX_ARGS + ptrIndex] = argsBuffers[bufIndex];
                ptrIndex++;
                bufIndex++;
            }
        }

        le_cfg_CancelTxn(procCfg);
    }

    // Terminate the list.
    argsPtr[INDEX_ARGS + ptrIndex] = NULL;

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
    if (chdir(workingDirPtr) != 0)
    {
        LE_FATAL("Could not change working directory to '%s'.  %m", workingDirPtr);
    }

    // NOTE: For now, at least, we run all unsandboxed apps as root to prevent major permissions
    //       issues when trying to perform system operations, such as changing routing tables.
    //       Consider using non-root users with capabilities later for another security layer.
}


//--------------------------------------------------------------------------------------------------
/**
 * Send the read end of the pipe to the log daemon for logging.  Closes both ends of the local pipe
 * afterwards.
 */
//--------------------------------------------------------------------------------------------------
static void SendStdPipeToLogDaemon
(
    proc_Ref_t procRef, ///< [IN] Process that owns the write end of the pipe.
    int pipefd[2],      ///< [IN] Pipe fds.
    int streamNum       ///< [IN] Either STDOUT_FILENO or STDERR_FILENO.
)
{
    if (pipefd[READ_PIPE] != -1)
    {
        // Send the read end to the log daemon.  The fd is closed once it is sent.
        if (streamNum == STDOUT_FILENO)
        {
            logFd_StdOut(pipefd[READ_PIPE],
                         app_GetName(procRef->appRef),
                         procRef->namePtr,
                         procRef->pid);
        }
        else
        {
            logFd_StdErr(pipefd[READ_PIPE],
                         app_GetName(procRef->appRef),
                         procRef->namePtr,
                         procRef->pid);
        }

        // Close the write end of the pipe because we don't need it.
        fd_Close(pipefd[WRITE_PIPE]);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Redirects the calling process's specified standard stream to the specified fd if the fd is a
 * valid file descriptor.  Otherwise redirect the standard stream to the log pipe.  The log pipe is
 * always closed afterwards.
 */
//--------------------------------------------------------------------------------------------------
static void RedirectStdStream
(
    int fd,                 ///< [IN] Fd to redirect to.
    int logPipe[2],         ///< [IN] Log standard out pipe.
    int streamNum           ///< [IN] Either STDOUT_FILENO or STDERR_FILENO.
)
{
    if (fd >= 0)
    {
        // Duplicate the fd onto the process' standard stream.  Leave the original fd open so it can
        // be re-used later.
        LE_FATAL_IF(dup2(fd, streamNum) == -1, "Could not duplicate fd.  %m.");
    }
    else
    {
        // Duplicate the write end of the log pipe onto the process' standard stream.
        LE_FATAL_IF(dup2(logPipe[WRITE_PIPE], streamNum) == -1,
                    "Could not duplicate fd.  %m.");

        // Close the two ends of the pipe because we don't need them.
        fd_Close(logPipe[READ_PIPE]);
        fd_Close(logPipe[WRITE_PIPE]);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Redirects the calling process's standard in, standard out and standard error to either the
 * process's stored file descriptors (possibly set by a client process) or to the specified log
 * pipes.  The log pipes are always closed afterwards.
 */
//--------------------------------------------------------------------------------------------------
static void RedirectStdStreams
(
    proc_Ref_t procRef,         ///< [IN] Process reference.
    int stdOutLogPipe[2],       ///< [IN] Log standard out pipe.
    int stdErrLogPipe[2]        ///< [IN] Log standard in pipe.
)
{
    RedirectStdStream(procRef->stdErrFd, stdErrLogPipe, STDERR_FILENO);
    RedirectStdStream(procRef->stdOutFd, stdOutLogPipe, STDOUT_FILENO);

    if (procRef->stdInFd >= 0)
    {
        // Duplicate the fd onto the process' standard in.  Leave the original fd open so it can
        // be re-used later.
        LE_FATAL_IF(dup2(procRef->stdInFd, STDIN_FILENO) == -1, "Could not duplicate fd.  %m.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pipe for logging either stdout or stderr.  The logging pipe is only created if the
 * process's stdout/stderr should not be redirected somewhere else.  If the pipe is not created the
 * pipe's fd values are set to -1.
 */
//--------------------------------------------------------------------------------------------------
static void CreateLogPipe
(
    proc_Ref_t procRef, ///< [IN] Process to create the pipe for.
    int pipefd[2],      ///< [OUT] Pipe fds.
    int streamNum       ///< [IN] Either STDOUT_FILENO or STDERR_FILENO.
)
{
    if ( ((streamNum == STDERR_FILENO) && (procRef->stdErrFd != -1)) ||
         ((streamNum == STDOUT_FILENO) && (procRef->stdOutFd != -1)) )
    {
        // Don't create the log pipe.
        pipefd[0] = -1;
        pipefd[1] = -1;

        return;
    }

    if (pipe(pipefd) != 0)
    {
        pipefd[0] = -1;
        pipefd[1] = -1;

        if (streamNum == STDERR_FILENO)
        {
            LE_ERROR("Could not create pipe. %s process' stderr will not be available.  %m.",
                     procRef->namePtr);
        }
        else
        {
            LE_ERROR("Could not create pipe. %s process' stdout will not be available.  %m.",
                     procRef->namePtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Confines the calling process into the sandbox.  The current working directory will be set to "/"
 * relative to the sandbox.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void ConfineProcInSandbox
(
    const char* sandboxRootPtr, ///< [IN] Path to the sandbox root.
    uid_t uid,                  ///< [IN] The user ID the process should be set to.
    gid_t gid,                  ///< [IN] The group ID the process should be set to.
    const gid_t* groupsPtr,     ///< [IN] List of supplementary groups for this process.
    size_t numGroups            ///< [IN] The number of groups in the supplementary groups list.
)
{
    // @Note: The order of the following statements is important and should not be changed carelessly.

    // Change working directory.
    LE_FATAL_IF(chdir(sandboxRootPtr) != 0,
                "Could not change working directory to '%s'.  %m", sandboxRootPtr);

    // Chroot to the sandbox.
    LE_FATAL_IF(chroot(sandboxRootPtr) != 0, "Could not chroot to '%s'.  %m", sandboxRootPtr);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");

    // Populate our supplementary groups list with the provided list.
    LE_FATAL_IF(setgroups(numGroups, groupsPtr) == -1,
                "Could not set the supplementary groups list.  %m.");

    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    // Set our process's user ID.  This sets all of our user IDs (real, effective, saved).  This
    // call also clears all cababilities.  This function in particular MUST be called after all
    // the previous system calls because once we make this call we will lose root priviledges.
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Blocks the calling thread's execution by performing a blocking read on the read end of the pipe.
 * This function will unblock and return once the other end of the pipe is closed.  This function
 * is used to synchronize parent/child processes and assumes that both the parent and child have
 * copies of the pipe.
 *
 * When this function exits both ends of the pipe are closed.
 */
//--------------------------------------------------------------------------------------------------
static void BlockOnPipe
(
    int pipeFd[2]           ///< [IN]
)
{
    // Don't need the write end of the pipe.
    fd_Close(pipeFd[WRITE_PIPE]);

    // Perform a blocking read on the read end of the pipe.  Once the other end of the pipe is
    // closed this function will exit.
    ssize_t numBytesRead;
    int dummyBuf;
    do
    {
        numBytesRead = read(pipeFd[READ_PIPE], &dummyBuf, 1);
    }
    while ( ((numBytesRead == -1)  && (errno == EINTR)) || (numBytesRead != 0) );

    LE_FATAL_IF(numBytesRead == -1, "Could not read pipe.  %m.");

    fd_Close(pipeFd[READ_PIPE]);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process.  If the process belongs to a sandboxed app the process will run in its sandbox,
 * otherwise the process will run in its working directory as root.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_Start
(
    proc_Ref_t procRef              ///< [IN] The process to start.
)
{
    if (procRef->run == false)
    {
        LE_INFO("Process '%s' is configured to not run.", procRef->namePtr);
        return LE_OK;
    }

    if (procRef->pid != -1)
    {
        LE_ERROR("Process '%s' (PID: %d) cannot be started because it is already running.",
                 procRef->namePtr, procRef->pid);
        return LE_FAULT;
    }

    if (framework_IsStopping())
    {
        LE_ERROR("Process '%s' cannot be started because framework is shutting down.",
                 procRef->namePtr);
        return LE_FAULT;
    }

    // Create a pipe for parent/child synchronization.
    int syncPipeFd[2];
    LE_FATAL_IF(pipe(syncPipeFd) == -1, "Could not create synchronization pipe.  %m.");

    // Create a pipe that can be used to block the child after the fork and initialization but
    // before the exec() call.
    int blockPipeFd[2] = {-1, -1};

    if (procRef->blockCallback != NULL)
    {
        LE_FATAL_IF(pipe(blockPipeFd) == -1, "Could not create block pipe.  %m.");
    }

    // @Note The current IPC system does not support forking so any reads to the config DB must be
    //       done in the parent process.

    // Get the environment variables from the config tree for this process.
    EnvVar_t envVars[LIMIT_MAX_NUM_ENV_VARS] = {{{ 0 }}};
    int numEnvVars = GetEnvironmentVariables(procRef, envVars, LIMIT_MAX_NUM_ENV_VARS);

    if ((numEnvVars < 0) || (numEnvVars > LIMIT_MAX_NUM_ENV_VARS))
    {
        LE_ERROR("Error getting environment variables.  Process '%s' cannot be started.",
                 procRef->namePtr);
        return LE_FAULT;
    }

    // Get the command line arguments from the config tree for this process.
    char argsBuffers[LIMIT_MAX_NUM_CMD_LINE_ARGS][LIMIT_MAX_ARGS_STR_BYTES];
    char* argsPtr[NUM_ARGS_PTRS];

    if (GetArgs(procRef, argsBuffers, argsPtr) != LE_OK)
    {
        LE_ERROR("Could not get command line arguments, process '%s' cannot be started.",
                 procRef->namePtr);
        return LE_FAULT;
    }

    // Get the resource limits from the config tree for this process.
    resLim_ProcLimits_t procLimits;
    resLim_GetProcLimits(procRef, &procLimits);

    // Create pipes for the process's standard error and standard out streams.
    int logStdOutPipe[2];
    int logStdErrPipe[2];
    CreateLogPipe(procRef, logStdOutPipe, STDOUT_FILENO);
    CreateLogPipe(procRef, logStdErrPipe, STDERR_FILENO);

    // Create the child process
    pid_t pID = fork();

    if (pID < 0)
    {
        LE_EMERG("Failed to fork.  %m.");
        return LE_FAULT;
    }

    if (pID == 0)
    {
        // Wait for the parent to allow us to continue by blocking on the read pipe until it
        // is closed.
        BlockOnPipe(syncPipeFd);

        // The parent has allowed us to continue.

        // Redirect the process's standard streams.
        RedirectStdStreams(procRef, logStdOutPipe, logStdErrPipe);

        // Set the process's SMACK label.
        char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppLabel(app_GetName(procRef->appRef), smackLabel, sizeof(smackLabel));

        smack_SetMyLabel(smackLabel);

        // Set the umask so that files are not accidentally created with global permissions.
        umask(S_IRWXG | S_IRWXO);

        // Unblock all signals that might have been blocked.
        sigset_t sigSet;
        LE_ASSERT(0 == sigfillset(&sigSet));
        LE_ASSERT(0 == pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL));

        SetEnvironmentVariables(envVars, numEnvVars);

        // Setup the process environment.
        if (app_GetIsSandboxed(procRef->appRef))
        {
            // Get the app's supplementary groups list.
            gid_t groups[LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS];
            size_t numGroups = LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS;

            LE_FATAL_IF(app_GetSupplementaryGroups(procRef->appRef, groups, &numGroups) != LE_OK,
                        "Supplementary groups list is too small.");

            // Sandbox the process.
            ConfineProcInSandbox(app_GetWorkingDir(procRef->appRef),
                                app_GetUid(procRef->appRef),
                                app_GetGid(procRef->appRef),
                                groups,
                                numGroups);
        }
        else
        {
            ConfigNonSandboxedProcess(app_GetWorkingDir(procRef->appRef));
        }

        if (procRef->blockCallback != NULL)
        {
            // Call the block callback function.
            procRef->blockCallback(getpid(), procRef->namePtr, procRef->blockContextPtr);

            BlockOnPipe(blockPipeFd);
        }

        // Launch the child program.  This should not return unless there was an error.
        LE_INFO("Execing '%s'", argsPtr[0]);

        // Close all non-standard file descriptors.
        fd_CloseAllNonStd();

        // Set resource limits.  This needs to be done as late as possible to avoid failures
        // when opening files before closing supervisor file descriptors
        resLim_SetProcLimits(&procLimits);

        // If starting under debugger, wait for debugger to attach.
        if (procRef->debug)
        {
            raise(SIGSTOP);
        }

        execvp(argsPtr[0], &(argsPtr[1]));

        // Store the errno returned by exec().
        int execErrno = errno;

        // The program could not be started.  Log an error message.
        log_ReInit();
        LE_FATAL("Could not exec '%s'.  %s.", argsPtr[0], strerror(execErrno));
    }

    procRef->pid = pID;

    // Don't need this end of the pipe.
    fd_Close(syncPipeFd[READ_PIPE]);

    // Set the scheduling priority for the child process while the child process is blocked.
    SetSchedulingPriority(procRef);

    // Send standard pipes to the log daemon so they will show up in the logs.
    SendStdPipeToLogDaemon(procRef, logStdErrPipe, STDERR_FILENO);
    SendStdPipeToLogDaemon(procRef, logStdOutPipe, STDOUT_FILENO);

    // Set the cgroups for the child process while the child process is blocked.
    resLim_SetCGroups(procRef);

    LE_INFO("Starting process '%s' with pid %d", procRef->namePtr, procRef->pid);

    // Unblock the child process.
    fd_Close(syncPipeFd[WRITE_PIPE]);

    // Check if the child process should be blocked.
    if (procRef->blockCallback != NULL)
    {
        // Don't need the read end of this pipe.
        fd_Close(blockPipeFd[READ_PIPE]);

        // Store the write end in the process's data struct.
        procRef->blockPipe = blockPipeFd[WRITE_PIPE];
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Used to indicate that the process is intentionally being stopped externally and not due to a
 * fault.  The process state is not updated right away only when the process actually stops.
 */
//--------------------------------------------------------------------------------------------------
void proc_Stopping
(
    proc_Ref_t procRef              ///< [IN] The process to stop.
)
{
    LE_ASSERT(procRef->pid != -1);

    // Set this flag to indicate that the process was intentionally killed and its fault action
    // should not be respected.
    procRef->cmdKill = true;
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
    else
    {
        return PROC_STATE_RUNNING;
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
    return (const char*)procRef->namePtr;
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
    return app_GetName(procRef->appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the process's config path.
 *
 * @return
 *      The process's config path.
 *      NULL if the process does not have a config.
 */
//--------------------------------------------------------------------------------------------------
const char* proc_GetConfigPath
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return (const char*)procRef->cfgPathPtr;
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
bool proc_IsRealtime
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    char priorStr[LIMIT_MAX_PRIORITY_NAME_BYTES] = "medium";
    char* priorStrPtr = priorStr;

    if (procRef->priorityPtr != NULL)
    {
        priorStrPtr = procRef->priorityPtr;
    }
    else if (procRef->cfgPathPtr != NULL)
    {
        // Read the priority setting from the config tree.
        le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(procRef->cfgPathPtr);

        le_result_t result = le_cfg_GetString(procCfg,
                                              CFG_NODE_PRIORITY,
                                              priorStr,
                                              sizeof(priorStr),
                                              "medium");

        le_cfg_CancelTxn(procCfg);

        if (result != LE_OK)
        {
            return false;
        }
    }

    if ( (priorStrPtr[0] == 'r') && (priorStrPtr[1] == 't') )
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's file descriptors to use as its standard in.
 *
 * By default the standard in is directed to /dev/null.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdIn
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdInFd                 ///< [IN] File descriptor to use as the app proc's standard in.
)
{
    if (procRef->stdInFd != -1)
    {
        fd_Close(procRef->stdInFd);
    }
    procRef->stdInFd = stdInFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's file descriptors to use as its standard out.
 *
 * By default the standard out is directed to the logs.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdOut
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdOutFd                ///< [IN] File descriptor to use as the app proc's standard out.
)
{
    if (procRef->stdOutFd != -1)
    {
        fd_Close(procRef->stdOutFd);
    }
    procRef->stdOutFd = stdOutFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's file descriptors to use as its standard error.
 *
 * By default the standard out is directed to the logs.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetStdErr
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    int stdErrFd                ///< [IN] File descriptor to use as the app proc's standard error.
)
{
    if (procRef->stdErrFd != -1)
    {
        fd_Close(procRef->stdErrFd);
    }
    procRef->stdErrFd = stdErrFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process's executable path.
 *
 * This overrides the configured executable path if available.  If the configuration for the process
 * is unavailable this function must be called to set the executable path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the executable path is too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_SetExecPath
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* execPathPtr     ///< [IN] Executable path to use for this process.  NULL to clear
                                ///       the executable path.
)
{
    // Clear the executable path.
    if (execPathPtr == NULL)
    {
        if (procRef->execPathPtr == NULL)
        {
            return LE_OK;
        }

        le_mem_Release(procRef->execPathPtr);
        procRef->execPathPtr = NULL;
        return LE_OK;
    }

    // Set the executable path.
    if (procRef->execPathPtr == NULL)
    {
        procRef->execPathPtr = le_mem_ForceAlloc(PathPool);
    }

    return le_utf8_Copy(procRef->execPathPtr, execPathPtr, LIMIT_MAX_PATH_BYTES, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the process's priority.
 *
 * This overrides the configured priority if available.
 *
 * The priority level string can be either "idle", "low", "medium", "high", "rt1" ... "rt32".
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the priority string is too long.
 *      LE_FAULT if the priority string is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_SetPriority
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* priorityPtr     ///< [IN] Priority string.  NULL to clear the priority.
)
{
    // Clear the priority string.
    if (priorityPtr == NULL)
    {
        if (procRef->priorityPtr == NULL)
        {
            return LE_OK;
        }

        le_mem_Release(procRef->priorityPtr);
        procRef->priorityPtr = NULL;
        return LE_OK;
    }

    // Check if the priority string is valid.
    if ( (strcmp(priorityPtr, "idle") == 0) ||
         (strcmp(priorityPtr, "low") == 0) ||
         (strcmp(priorityPtr, "medium") == 0) ||
         (strcmp(priorityPtr, "high") == 0) )
    {
        goto setPriority;
    }

    if ( (priorityPtr[0] == 'r') && (priorityPtr[1] == 't') )
    {
        int rtLevel;

        if ( (le_utf8_ParseInt(&rtLevel, &(priorityPtr[2])) == LE_OK) &&
             (rtLevel >= MIN_RT_PRIORITY) &&
             (rtLevel <= MAX_RT_PRIORITY) )
        {
            goto setPriority;
        }
    }

    return LE_FAULT;

setPriority:

    // Set the priority string.
    if (procRef->priorityPtr == NULL)
    {
        procRef->priorityPtr = le_mem_ForceAlloc(PathPool);
    }

    return le_utf8_Copy(procRef->priorityPtr, priorityPtr, LIMIT_MAX_PRIORITY_NAME_BYTES, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a cmd-line argument to a process.  Adding a NULL argPtr is valid and can be used to validate
 * the args list without actually adding an argument.  This is useful for overriding the configured
 * arguments with an empty list.
 *
 * This overrides the configured arguments if available.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the argument string is too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t proc_AddArgs
(
    proc_Ref_t procRef,         ///< [IN] The process reference.
    const char* argPtr          ///< [IN] Argument string.
)
{
    if (argPtr != NULL)
    {
        Arg_t* argObjPtr = le_mem_ForceAlloc(ArgsPool);

        if (le_utf8_Copy(argObjPtr->argument, argPtr, LIMIT_MAX_ARGS_STR_BYTES, NULL) != LE_OK)
        {
            le_mem_Release(argObjPtr);
            return LE_OVERFLOW;
        }

        argObjPtr->link = LE_SLS_LINK_INIT;

        le_sls_Queue(&(procRef->argsList), &(argObjPtr->link));
    }

    procRef->argsListValid = true;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes and invalidates the cmd-line arguments to a process.  This means the process will only
 * use arguments from the config if available.
 */
//--------------------------------------------------------------------------------------------------
void proc_ClearArgs
(
    proc_Ref_t procRef          ///< [IN] The process reference.
)
{
    procRef->argsListValid = false;

    // Delete arguments override list.
    le_sls_Link_t* argLinkPtr = le_sls_Pop(&(procRef->argsList));

    while (argLinkPtr != NULL)
    {
        Arg_t* argPtr = CONTAINER_OF(argLinkPtr, Arg_t, link);

        le_mem_Release(argPtr);

        argLinkPtr = le_sls_Pop(&(procRef->argsList));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set fault action.
 *
 * This overrides the configured fault action if available.
 *
 * The fault action can be set to FAULT_ACTION_NONE to indicate that the configured fault action
 * should be used if available.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetFaultAction
(
    proc_Ref_t procRef,                 ///< [IN] The process reference.
    FaultAction_t faultAction           ///< [IN] Fault action.
)
{
    if (FAULT_ACTION_NONE == faultAction)
    {
        procRef->faultAction = procRef->defaultFaultAction;
    }
    else
    {
        procRef->faultAction = faultAction;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get fault action for a given process.
 */
//--------------------------------------------------------------------------------------------------

FaultAction_t proc_GetFaultAction
(
    proc_Ref_t procRef
)
{
    return (procRef->defaultFaultAction);
}


//--------------------------------------------------------------------------------------------------
/**
 * Blocks the process on startup, after it is forked and initialized but before it has execed.  The
 * specified callback function will be called when the process has blocked.  Clearing the callback
 * function means the process should not block on startup.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetBlockCallback
(
    proc_Ref_t procRef,                     ///< [IN] The process reference.
    proc_BlockCallback_t blockCallback,     ///< [IN] Callback to set.  NULL to clear.
    void* contextPtr                        ///< [IN] Context pointer.
)
{
    procRef->blockCallback = blockCallback;
    procRef->blockContextPtr = contextPtr;

    // Clean up the fd if the block is not being used.
    if (procRef->blockCallback == NULL)
    {
        proc_Unblock(procRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Unblocks a process that was blocked on startup.
 */
//--------------------------------------------------------------------------------------------------
void proc_Unblock
(
    proc_Ref_t procRef                      ///< [IN] The process reference.
)
{
    if (procRef->blockPipe != -1)
    {
        fd_Close(procRef->blockPipe);
        procRef->blockPipe = -1;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the run flag.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetRun
(
    proc_Ref_t procRef, ///< [IN] The process reference.
    bool run            ///< [IN] Run flag.
)
{
    procRef->run = run;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the debug flag.
 */
//--------------------------------------------------------------------------------------------------
void proc_SetDebug
(
    proc_Ref_t procRef, ///< [IN] The process reference.
    bool debug          ///< [IN] Debug flag.
)
{
    procRef->debug = debug;
}


//--------------------------------------------------------------------------------------------------
/**
 * Called to capture any extra data that may help indicate what contributed to the fault that caused
 * the given process to fail.
 *
 * This function calls a shell script that will save a dump of the system log and any core files
 * that have been generated into a known location.
 */
//--------------------------------------------------------------------------------------------------
static void CaptureDebugData
(
    proc_Ref_t procRef,             ///< [IN] The process reference.
    bool isRebooting                ///< [IN] Is the supervisor going to reboot the system?
)
{
    char command[LIMIT_MAX_PATH_BYTES];
    int s = snprintf(command,
                     sizeof(command),
                     "/legato/systems/current/bin/saveLogs %s %s %s",
                     app_GetName(procRef->appRef),
                     procRef->namePtr,
                     isRebooting ? "REBOOT" : "");

    if (s >= sizeof(command))
    {
        LE_FATAL("Could not create command, buffer is too small.  "
                 "Buffer is %zd bytes but needs to be %d bytes.",
                 sizeof(command),
                 s);
    }

    int r = system(command);

    if (!WIFEXITED(r) || (WEXITSTATUS(r) != EXIT_SUCCESS))
    {
        LE_ERROR("Could not save log and core file.");
    }
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
wdog_action_WatchdogAction_t proc_GetWatchdogAction
(
    proc_Ref_t procRef             ///< [IN] The process reference.
)
{
    return procRef->watchdogAction;
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
    proc_Ref_t procRef,                 ///< [IN] The process reference.
    FaultAction_t currFaultAction,      ///< [IN] The process's current fault action.
    time_t prevFaultTime                ///< [IN] Time of the previous fault.
)
{
    switch (currFaultAction)
    {
        case FAULT_ACTION_RESTART_PROC:
            if ( (procRef->faultTime != 0) &&
                 (procRef->faultTime - prevFaultTime <= FAULT_LIMIT_INTERVAL_RESTART) )
            {
                return true;
            }
            break;

        case FAULT_ACTION_RESTART_APP:
            if ( (procRef->faultTime != 0) &&
                 (procRef->faultTime - prevFaultTime <= FAULT_LIMIT_INTERVAL_RESTART_APP) )
            {
                return true;
            }
            break;

        default:
            // Fault limits do not apply to the other fault actions.
            return false;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for the specified process.
 *
 * @return
 *      The fault action that should be taken for this process.
 */
//--------------------------------------------------------------------------------------------------
FaultAction_t proc_SigChildHandler
(
    proc_Ref_t procRef,             ///< [IN] The process reference.
    int procExitStatus              ///< [IN] The status of the process given by wait().
)
{
    FaultAction_t faultAction = FAULT_ACTION_NONE;

    if (procRef->cmdKill)
    {
        // The cmdKill flag was set which means the process died because we killed it so
        // it was not a fault.  Reset the cmdKill flag so that if this process is restarted
        // faults will still be caught.
        procRef->cmdKill = false;

        // Remember that this process is dead.
        procRef->pid = -1;

        return FAULT_ACTION_NONE;
    }

    // Remember the previous fault time.
    time_t prevFaultTime = procRef->faultTime;

    // Record the fault time.
    procRef->faultTime = (le_clk_GetAbsoluteTime()).sec;

    if (WIFEXITED(procExitStatus))
    {
        LE_INFO("Process '%s' (PID: %d) has exited with exit code %d.",
                procRef->namePtr, procRef->pid, WEXITSTATUS(procExitStatus));

        if (WEXITSTATUS(procExitStatus) != EXIT_SUCCESS)
        {
            faultAction = procRef->faultAction;
        }
    }
    else if (WIFSIGNALED(procExitStatus))
    {
        int sig = WTERMSIG(procExitStatus);

        // WARNING: strsignal() is non-reentrant.  We use it here because the Supervisor is
        //          single threaded.
        LE_INFO("Process '%s' (PID: %d) has exited due to signal %d (%s).",
                procRef->namePtr, procRef->pid, sig, strsignal(sig));

        faultAction = procRef->faultAction;
    }
    else
    {
        LE_FATAL("Unexpected status value (%d) for pid %d.", procExitStatus, procRef->pid);
    }

    // Record the fact that the process is dead.
    procRef->pid = -1;

    // If the process has reached its fault limit, take action to stop
    // the apparently futile attempts to start this thing.
    if (ReachedFaultLimit(procRef, faultAction, prevFaultTime))
    {
        if (sysStatus_IsGood())
        {
            LE_CRIT("Process '%s' reached the fault limit (in a 'good' system) "
                     "and will be stopped.",
                     procRef->namePtr);

            faultAction = FAULT_ACTION_STOP_APP;
        }
        else
        {
            LE_EMERG("Process '%s' reached fault limit while system in probation. "
                     "Device will be rebooted.",
                     procRef->namePtr);

            faultAction = FAULT_ACTION_REBOOT;
        }
    }

    // If the process stopped due to an error, save all relevant data for future diagnosis.
    if (faultAction != FAULT_ACTION_NONE)
    {
        // Check if we're rebooting.  If we are, this data needs to be saved in a more permanent
        // location.
        bool isRebooting = (faultAction == FAULT_ACTION_REBOOT);
        CaptureDebugData(procRef, isRebooting);
    }

    return faultAction;
}
