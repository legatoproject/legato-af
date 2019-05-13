/**
 * @page c_mutex Mutex API
 *
 * @subpage le_mutex.h "API Reference"
 *
 * <HR>
 *
 * The Mutex API provides standard mutex functionality with added diagnostics capabilities.
 * These mutexes can be shared by threads within the same process, but can't
 * be shared by threads in different processes.
 *
 * @warning  Multithreaded programming is an advanced subject with many pitfalls.
 * A general discussion of why and how mutexes are used in multithreaded programming is beyond
 * the scope of this documentation.  If you are not familiar with these concepts @e please seek
 * out training and mentorship before attempting to work on multithreaded production code.
 *
 * Two kinds of mutex are supported by Legato:
 *  - @b Recursive or
 *  - @b Non-Recursive
 *
 * All mutexes can be locked and unlocked. The same lock, unlock, and delete
 * functions work for all the mutexes, regardless of what type they are.
 *
 * A recursive mutex can be locked again by the same thread that already has the lock, but
 * a non-recursive mutex can only be locked once before being unlocked.
 *
 * If a thread grabs a non-recursive mutex lock and then tries to grab that same lock again, a
 * deadlock occurs.  Legato's non-recursive mutexes will detect this deadlock, log a fatal error
 * and terminate the process.
 *
 * If a thread grabs a recursive mutex, and then the same thread grabs the same lock again, the
 * mutex's "lock count" is incremented.  When the thread unlocks that mutex, the lock count is
 * decremented.  Only when the lock count reaches zero will the mutex actually unlock.
 *
 * There's a limit to the number of times the same recursive mutex can be locked by the same
 * thread without ever unlocking it, but that limit is so high (at least 2 billion), if that
 * much recursion is going on, there are other, more serious problems with the program.
 *
 * @section c_mutex_create Creating a Mutex
 *
 * In Legato, mutexes are dynamically allocated objects.  Functions that create them
 * return references to them (of type le_mutex_Ref_t).
 *
 * Functions for creating mutexes:
 *  - @c le_mutex_CreateRecursive() - creates a recursive mutex.
 *  - @c le_mutex_CreateNonRecursive() - creates a non-recursive mutex.
 *
 * All mutexes have names, required for diagnostic purposes.  See
 * @ref c_mutex_diagnostics below.
 *
 * @section c_mutex_locking Using a Mutex
 *
 * Functions for locking and unlocking mutexes:
 *  - @c le_mutex_Lock()
 *  - @c le_mutex_Unlock()
 *  - @c le_mutex_TryLock()
 *
 * It doesn't matter what type of mutex you are using, you
 * still use the same functions for locking and unlocking your mutex.
 *
 * @section c_mutex_delete Deleting a Mutex
 *
 * When you are finished with a mutex, you must delete it by calling le_mutex_Delete().
 *
 * There must not be anyone using the mutex when it is deleted (i.e., no one can be holding it).
 *
 * @section c_mutex_diagnostics Diagnostics
 *
 * The command-line diagnostic tool @ref toolsTarget_inspect can be used to list the mutexes
 * that currently exist inside a given process.  The state of each mutex can be
 * seen, including a list of any threads that might be waiting for that mutex.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_mutex.h
 *
 * Legato @ref c_mutex include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MUTEX_INCLUDE_GUARD
#define LEGATO_MUTEX_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a Mutex object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mutex* le_mutex_Ref_t;


#if LE_CONFIG_MUTEX_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Create a Recursive mutex.
 *
 *  @param[in]  nameStr Name of the mutex
 *
 *  @return  Returns a reference to the mutex.
 *
 *  @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char *nameStr
);
#else /* if not LE_CONFIG_MUTEX_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_mutex_CreateRecursive().
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t _le_mutex_CreateRecursive(void);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Create a Recursive mutex.
 *
 *  @param[in]  nameStr Name of the mutex
 *
 *  @return  Returns a reference to the mutex.
 *
 *  @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char *nameStr
)
{
    LE_UNUSED(nameStr);
    return _le_mutex_CreateRecursive();
}
#endif /* end LE_CONFIG_MUTEX_NAMES_ENABLED */


#if LE_CONFIG_MUTEX_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Create a Non-Recursive mutex.
 *
 *  @param[in]  nameStr Name of the mutex
 *
 *  @return  Returns a reference to the mutex.
 *
 *  @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char *nameStr
);
#else /* if not LE_CONFIG_MUTEX_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_mutex_CreateNonRecursive().
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t _le_mutex_CreateNonRecursive(void);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Create a Non-Recursive mutex.
 *
 *  @param[in]  nameStr Name of the mutex
 *
 *  @return  Returns a reference to the mutex.
 *
 *  @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char *nameStr
)
{
    LE_UNUSED(nameStr);
    return _le_mutex_CreateNonRecursive();
}
#endif /* end LE_CONFIG_MUTEX_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Delete a mutex.
 *
 * @return  Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Delete
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Lock a mutex.
 *
 * @return  Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Lock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Try a lock on a mutex.
 *
 * Locks a mutex, if no other thread holds it.  Otherwise, returns without locking.
 *
 * @return
 *  - LE_OK if mutex was locked.
 *  - LE_WOULD_BLOCK if mutex was already held by someone else.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mutex_TryLock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Unlock a mutex.
 *
 * @return  Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Unlock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Declare a static mutex reference variable and accessor functions.
 *
 * This is handy when you need a single, file-scope mutex for use inside your module
 * to protect other file-scope data structures from multi-threaded race conditions.
 *
 * Adding the line
 * @code
 * LE_MUTEX_DECLARE_REF(MyMutexRef);
 * @endcode
 * near the top of your file will create a file-scope variable called "MyMutexRef" of type
 * le_mutex_Ref_t and functions called "Lock" and "Unlock" that access that variable.
 *
 * See @ref c_mutex_locking_tips for more information.
 *
 * @param   refName Name of the mutex reference variable.
 *
 * @return  Nothing.
 */
//--------------------------------------------------------------------------------------------------
#define LE_MUTEX_DECLARE_REF(refName) \
    static le_mutex_Ref_t refName; \
    static inline void Lock(void) { le_mutex_Lock(refName); } \
    static inline void Unlock(void) { le_mutex_Unlock(refName); }

#endif /* LEGATO_MUTEX_INCLUDE_GUARD */
