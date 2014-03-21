/** @page c_semaphore Semaphore API
 *
 * @ref le_semaphore.h "Click here for the API Reference documentation."
 *
 * <HR>
 * 
 * @ref create_semaphore <br>
 * @ref use_semaphore <br>
 * @ref delete_semaphore <br>
 * @ref diagnostics_semaphore <br>
 *
 * This API provides standard semaphore functionality, but with added diagnostic capabilities.
 * These semaphores can only be shared by threads within the same process.
 *
 * Two kinds of semaphore are supported by Legato:
 *  - @b Normal
 *  - @b Traceable
 *
 * Normal semaphores are faster than traceable semaphores and consume less memory, but still offer
 * some diagnosic capabilities. Traceable semaphores generally behave the same as Normal
 * sempahores, but can also log their activities.
 *
 * All semaphores can wait (decrease value by one) and post (increase value by one).
 * The same wait, post, and delete functions work for all the semaphores,
 * regardless of what type they are.  This means that a semaphore can be changed from Normal to
 * Traceable or vice versa just by changing the function you use to create it.
 * This helps when troubleshooting race conditions or deadlocks because it's
 * easy to switch one semaphore or a select few semaphores to Traceable, without suffering the
 * runtime cost of switching @e all semaphores to the slower Traceable semaphores.
 *
 *
 * @section create_semaphore Creating a Semaphore
 *
 * In Legato, semaphores are dynamically allocated objects. Functions that create
 * them return references to them (of type le_sem_Ref_t).
 * The functions for creating semaphores are:
 *  - le_sem_Create() - creates a @b normal, @b semaphore.
 *  - le_sem_CreateTraceable() - creates a @b traceable, @b semaphore.
 *
 * @todo add into semaphore API function le_sem_OpenShared() for semaphore shared between process.
 *
 * Note that all semaphores have names.  This is required for diagnostic purposes.  See
 * @ref diagnostics_semaphore below.
 *
 * @section use_semaphore Using a Semaphore
 *
 * Functions to increase and decrease semaphores are:
 *  - @c le_sem_Post()
 *  - @c le_sem_Wait()
 *  - @c le_sem_TryWait()
 *
 * Function to get semaphores values is:
 *  - @c le_sem_GetValue()
 *
 * It doesn't matter what type of semaphore you are using,
 * you still use the same functions for increasing, decreasing, getting value your semaphore.
 *
 * @section delete_semaphore Deleting a Semaphore
 *
 * When you are finished with a semaphore, you must delete it by calling le_sem_Delete().
 *
 * There must not be anything using the semaphore when it is deleted
 * (i.e., no one can be waiting on it).
 *
 * @section diagnostics_semaphore Diagnostics
 *
 * Both Normal and Traceable semaphores have some diagnostics capabilities.
 *
 * The command-line diagnostic tool @ref tool_lssemaphore "lssemaphore" can be used to list the
 * semaphores that currently exist inside a given process.
 * The state of each semaphore can be seen, including a list of any threads that
 * might be waiting for that semaphore.
 *
 * The tool @ref tool_threadlook "threadlook" will show if a given thread is currently
 * waiting for a semaphore, and will name that semaphore.
 *
 * If there are Traceable semaphores in a process, then it will be possible to use the
 * @ref tool_log "log" tool to enable or disable tracing on that semaphore.
 * The trace keyword name is the name of the process, the name of the component,
 * and the name of the semaphore, separated by slashes (e.g., "process/component/semaphore").
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 *
 */

/** @file le_semaphore.h
 *
 *
 * Legato @ref c_semaphore include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SEMAPHORE_INCLUDE_GUARD
#define LEGATO_SEMAPHORE_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reference to Semaphore structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sem_t* le_sem_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * Create a semaphore shared by threads within the same process.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t le_sem_Create
(
    const char*     name,               ///< [IN] Name of the semaphore.
    int32_t         initialCount        ///< [IN] Initial number of semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a traceable semaphore shared by threads within the same process.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t le_sem_CreateTraceable
(
    const char*     name,               ///< [IN] Name of the semaphore.
    int32_t         initialCount        ///< [IN] Initial number of semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a semaphore.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Delete
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Finds a specified semaphore's name.
 *
 * @return
 *      Reference to the semaphore, or NULL if the semaphore doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t le_sem_FindSemaphore
(
    const char* name    ///< [IN] Name of the semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Wait for a semaphore.
 *
 * @return Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Wait
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Try to wait for a semaphore.
 *
 * It's the same as @ref le_sem_Wait, except if it can't be immediately performed,
 * then returns an LE_WOULD_BLOCK instead of blocking it.
 *
 * @return Upon successful completion, returns LE_OK (0), otherwise it returns
 * LE_WOULD_BLOCK as the call would block if it was a blocking call.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sem_TryWait
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Post a semaphore.
 *
 * @return Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Post
(
    le_sem_Ref_t    semaphorePtr      ///< [IN] Pointer to the semaphore.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a semaphore.
 *
 * @return value of the semaphore
 */
//--------------------------------------------------------------------------------------------------
int le_sem_GetValue
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore.
);

#endif // LEGATO_SEMAPHORE_INCLUDE_GUARD
