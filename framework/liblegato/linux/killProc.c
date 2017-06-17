//--------------------------------------------------------------------------------------------------
/** @file killProc.c
 *
 * API for killing processes.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "killProc.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Process timer object used for killing the process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t pid;                          ///< PID of the process.
    le_timer_Ref_t timerRef;            ///< Kill timer for the process.
}
ProcTimerObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for the process timer object.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t ProcTimerMemPool;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of process timers.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t ProcTimerHash;


//--------------------------------------------------------------------------------------------------
/**
 * Process timer hash map size.
 */
//--------------------------------------------------------------------------------------------------
#define PROC_TIMER_HASH_SIZE        31


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the kill API.
 */
//--------------------------------------------------------------------------------------------------
void kill_Init
(
    void
)
{
    ProcTimerMemPool = le_mem_CreatePool("KillProcs", sizeof(ProcTimerObj_t));

    ProcTimerHash = le_hashmap_Create("KillProcs", PROC_TIMER_HASH_SIZE, le_hashmap_HashUInt32,
                                      le_hashmap_EqualsUInt32);
}


//--------------------------------------------------------------------------------------------------
/**
 * Hard kill the process.  Called when a soft kill timer has expired.
 */
//--------------------------------------------------------------------------------------------------
static void HardKillExpiredProc
(
    le_timer_Ref_t timerRef
)
{
    pid_t* pidPtr = (pid_t*)le_timer_GetContextPtr(timerRef);

    kill_Hard(*pidPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiates a soft kill of a process.  This sends the process a SIGTERM signal allowing the process
 * to catch the signal and perform a graceful shutdown.  If the process fails to shut down within
 * the timeout period a hard kill (SIGKILL) will be performed.  If the calling process knows that
 * the target process has already died it can call kill_Died() to cancel the hard kill timeout.  The
 * calling process must have privileges to send signals to the process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_Soft
(
    pid_t pid,                  ///< [IN] Process to kill.
    size_t timeoutMsecs         ///< [IN] Number of milliseconds until a hard kill is sent.
)
{
    // Check if there is already a timer for this process.
    if (le_hashmap_Get(ProcTimerHash, &pid) != NULL)
    {
        LE_WARN("Trying to kill a process that is already being killed.");
        return;
    }

    // Create a process timer object.
    ProcTimerObj_t* procTimerPtr = le_mem_ForceAlloc(ProcTimerMemPool);
    procTimerPtr->pid = pid;

    // Create a name for this process's timer.
    char timerName[20];
    LE_ASSERT(snprintf(timerName, sizeof(timerName), "kill%d", pid) < sizeof(timerName));

    // Create the timer.
    procTimerPtr->timerRef = le_timer_Create(timerName);

    le_clk_Time_t timeout;
    timeout.sec = timeoutMsecs / 1000;
    timeout.usec = (timeoutMsecs % 1000) * 1000;
    LE_ASSERT(le_timer_SetInterval(procTimerPtr->timerRef, timeout) == LE_OK);

    LE_ASSERT(le_timer_SetContextPtr(procTimerPtr->timerRef, &(procTimerPtr->pid)) == LE_OK);

    LE_ASSERT(le_timer_SetHandler(procTimerPtr->timerRef, HardKillExpiredProc) == LE_OK);

    // Add the process timer to the hash map.
    le_hashmap_Put(ProcTimerHash, &(procTimerPtr->pid), procTimerPtr);

    // Soft Kill the system process.
    LE_DEBUG("Sending SIGTERM to process %d", pid);

    if (kill(pid, SIGTERM) == -1)
    {
        if (errno == ESRCH)
        {
            kill_Died(pid);
            return;
        }

        LE_FATAL("Failed to send SIGTERM to process (PID: %d).  %m.", pid);
    }

    // Start the soft kill timer in case process does not comply.
    le_timer_Start(procTimerPtr->timerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when a process actually dies.  This should be called when the process actually dies so
 * that an additional hard kill is not attempted.
 */
//--------------------------------------------------------------------------------------------------
void kill_Died
(
    pid_t pid                   ///< [IN] Process that died.
)
{
    // Remove the process timer from the hash map.
    ProcTimerObj_t* procTimerPtr = (ProcTimerObj_t*)le_hashmap_Remove(ProcTimerHash, &pid);

    if (procTimerPtr != NULL)
    {
        le_timer_Delete(procTimerPtr->timerRef);
        le_mem_Release(procTimerPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiates a hard kill to kill the process immediately.  The calling process must have privileges
 * to send signals to the process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_Hard
(
    pid_t pid                   ///< [IN] Process to kill.
)
{
    // Remove the timer from the hash map.
    kill_Died(pid);

    LE_DEBUG("Sending SIGKILL to process %d", pid);

    LE_FATAL_IF((kill(pid, SIGKILL) == -1) && (errno != ESRCH),
                "Failed to send SIGKILL to process (PID: %d).  %m.", pid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Kills processes by name.  Kills all occurrences of a process with the specified name.
 */
//--------------------------------------------------------------------------------------------------
void kill_ByName
(
    const char* procNamePtr     ///< [IN] Name of the processes to kill.
)
{
    char killCmd[LIMIT_MAX_PATH_BYTES];
    int s = snprintf(killCmd, sizeof(killCmd), "killall -q %s", procNamePtr);

    if (s >= sizeof(killCmd))
    {
        LE_FATAL("Could not create 'killall' cmd, buffer is too small.  Buffer is %zd bytes but \
needs to be %d bytes.", sizeof(killCmd), s);
    }

    int r = system(killCmd);

    LE_FATAL_IF(!WIFEXITED(r), "Could not send killall cmd.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a signal to a process.  The calling process must have privileges to send signals to the
 * process specified by pid.
 *
 * If the process does not exist this function simply returns.
 *
 * @note
 *      Does not return on error.
 */
//--------------------------------------------------------------------------------------------------
void kill_SendSig
(
    pid_t pid,                  ///< [IN] Process to send the signal to.
    int sig                     ///< [IN] The signal to send.
)
{
    LE_FATAL_IF((kill(pid, sig) == -1) && (errno != ESRCH),
                "Failed to send signal, %d, to process (PID: %d).  %m.", sig, pid);
}
