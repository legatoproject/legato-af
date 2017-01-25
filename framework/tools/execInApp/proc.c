//--------------------------------------------------------------------------------------------------
/** @file execInApp/proc.c
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
#include "limit.h"
#include "le_cfg_interface.h"
#include "fileDescriptor.h"
#include "user.h"
#include "log.h"
#include "smack.h"
#include "killProc.h"
#include "interfaces.h"


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
    char*           name;           ///< Name of the process.
    char            cfgPathRoot[LIMIT_MAX_PATH_BYTES];      ///< Our path in the config tree.
    app_Ref_t       appRef;         ///< Reference to the app that we are part of.
    pid_t           pid;            ///< The pid of the process.
    time_t          faultTime;      ///< The time of the last fault.
    bool            cmdKill;        ///< true if the process was killed by proc_Stop().
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
    app_Ref_t appRef                ///< [IN] Reference to the app that we are part of.
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



    procPtr->name = le_path_GetBasenamePtr(procPtr->cfgPathRoot, "/");

    procPtr->appRef = appRef;
    procPtr->faultTime = 0;
    procPtr->pid = -1;  // Processes that are not running are assigned -1 as its pid.
    procPtr->cmdKill = false;

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
 * Confines the calling process into the sandbox.  The current working directory will be set to "/"
 * relative to the sandbox.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void proc_ConfineProcInSandbox
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
