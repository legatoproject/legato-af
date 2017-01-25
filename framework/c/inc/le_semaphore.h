/** @page c_semaphore Semaphore API
 *
 * @ref le_semaphore.h "API Reference"
 *
 * <HR>
 * This API provides standard semaphore functionality, but with added diagnostic capabilities.
 * These semaphores can only be shared by threads within the same process.
 *
 * Semaphores can wait (decrease value by one) and post (increase value by one).
 *
 * In Legato, semaphores are dynamically allocated objects.
 *
 * @section create_semaphore Creating a Semaphore
 *
 * le_sem_Create() creates a semaphore, returning a reference to it (of type le_sem_Ref_t).
 *
 * All semaphores have names.  This is required for diagnostic purposes.  See
 * @ref diagnostics_semaphore below.
 *
 * @section use_semaphore Using a Semaphore
 *
 * Functions to increase and decrease semaphores are:
 *  - @c le_sem_Post()
 *  - @c le_sem_Wait()
 *  - @c le_sem_TryWait()
 *  - @c le_sem_WaitWithTimeOut()
 *
 * Function to get a semaphore's current value is:
 *  - @c le_sem_GetValue()
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
 * The command-line @ref toolsTarget_inspect tool can be used to list the
 * semaphores that currently exist inside a given process.
 * The state of each semaphore can be seen, including a list of any threads that
 * might be waiting for that semaphore.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

/** @file le_semaphore.h
 *
 *
 * Legato @ref c_semaphore include file.
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Wait for a semaphore with a limit on how long to wait.
 *
 * @return
 *      - LE_OK         The function succeed
 *      - LE_TIMEOUT    timeToWait elapsed
 *
 * @note When LE_TIMEOUT occurs the semaphore is not decremented.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sem_WaitWithTimeOut
(
    le_sem_Ref_t    semaphorePtr,   ///< [IN] Pointer to the semaphore.
    le_clk_Time_t   timeToWait      ///< [IN] Time to wait
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
