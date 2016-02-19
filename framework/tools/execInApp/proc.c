//--------------------------------------------------------------------------------------------------
/** @file execInApp/proc.c
 *
 * Temporary solution until command apps are available.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "app.h"
#include "proc.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Minimum and maximum realtime priority levels.
 */
//--------------------------------------------------------------------------------------------------
#define MIN_RT_PRIORITY                         1
#define MAX_RT_PRIORITY                         32


//--------------------------------------------------------------------------------------------------
/**
 * The process object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct proc_Ref
{
    char*           name;                                   ///< Name of the process.
    char            cfgPathRoot[LIMIT_MAX_PATH_BYTES];      ///< Our path in the config tree.
    app_Ref_t       appRef;         ///< Reference to the app that we are part of.
    bool            paused;         ///< true if the process is paused.
    pid_t           pid;            ///< The pid of the process.
    time_t          faultTime;      ///< The time of the last fault.
    bool            cmdKill;        ///< true if the process was killed by proc_Stop().
    le_timer_Ref_t  timerRef;       ///< Timer used to allow the application to shutdown.
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


