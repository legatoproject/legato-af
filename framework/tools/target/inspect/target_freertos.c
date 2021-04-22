/**
 * @file target_freertos.c
 *
 * Target-specific functions for inspect tool.  Stops, restarts and inspects memory of the
 * target task.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "inspect_target.h"

#include "FreeRTOS.h"

//--------------------------------------------------------------------------------------------------
/**
 * Store the normal priority of this task so we can restore afterwards.  Priority is used
 * to block other Legato tasks from interrupting inspection.
 */
//--------------------------------------------------------------------------------------------------
static UBaseType_t NormalPriority;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the counterpart address of the specified local reference in the address space of the
 * specified process.
 *
 * @return
 *      Remote address that is the counterpart of the local address.
 */
//--------------------------------------------------------------------------------------------------
uintptr_t target_GetRemoteAddress
(
    pid_t pid,              ///< [IN] Remote process to to get the address for.
    void* localAddrPtr      ///< [IN] Local address to get the offset with.
)
{
    /* RTOSes have no address space translation -- address in == address out */
    return (uintptr_t)localAddrPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attach to the target process in order to gain control of its execution and access its memory
 * space.
 */
//--------------------------------------------------------------------------------------------------
void target_Attach
(
    pid_t pid              ///< [IN] Remote process to attach to
)
{
    // Nothing required, same address space
}

//--------------------------------------------------------------------------------------------------
/**
 * Detach from a process that we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
void target_DetachAndExit
(
    pid_t pid              ///< [IN] Remote process to detach from
)
{
    le_thread_Exit(NULL);

#ifdef __GNUC__
    __builtin_unreachable();
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause execution of a running process which we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
void target_Stop
(
    pid_t pid              ///< [IN] Remote process to stop.
)
{
    // Temporarily suspend scheduler to stop all Legato tasks
    // Do this by setting priority to max to avoid asserting when taking mutexes.
    NormalPriority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, SYSTEM_RESERVED_TASK_PRIO_MAX);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume execution of a previously paused process.
 */
//--------------------------------------------------------------------------------------------------
void target_Start
(
    pid_t pid              ///< [IN] Remote process to restart
)
{
    // Resume normal priority
    vTaskPrioritySet(NULL, NormalPriority);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read from the memory of an attached target process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t target_ReadAddress
(
    pid_t pid,              ///< [IN] Remote process to read address
    uintptr_t remoteAddr,   ///< [IN] Remote address to read from target
    void* buffer,           ///< [OUT] Destination to read into
    size_t size             ///< [IN] Number of bytes to read
)
{
    // Same address space, just memcpy.
    memcpy(buffer, (void *)remoteAddr, size);

    return LE_OK;
}
