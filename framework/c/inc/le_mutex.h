/**
 * @page c_mutex Mutex API
 *
 * @ref le_mutex.h "API Reference"
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
 *  - @b Normal
 *  - @b Traceable
 *
 * Normal mutexes are faster than traceable mutexes and consume less memory, but still offer some
 * diagnosic capabilities.  Traceable mutexes generally behave the same as Normal mutexes, but
 * can also log their activities.
 *
 * In addition, both Normal and Traceable mutexes can be either
 *  - @b Recursive or
 *  - @b Non-Recursive
 *
 * All mutexes can be locked and unlocked. The same lock, unlock, and delete
 * functions work for all the mutexes, regardless of what type they are.  his means that a mutex
 * can be changed from Normal to Traceable (or vice versa) by changing the function you use
 * to create it. This helps to troubleshoot race conditions or deadlocks because it's
 * easy to switch one mutex or a select few mutexes to Traceable without suffering the
 * runtime cost of switching @e all mutexes to the slower Traceable mutexes.
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
 * thread without ever unlocking it, but that limit is so high (at least 2 billion), if that much recursion is
 *  going on, there are other, more serious problems with the program.
 *
 * @section c_mutex_create Creating a Mutex
 *
 * In Legato, mutexes are dynamically allocated objects.  Functions that create them
 * return references to them (of type le_mutex_Ref_t). 
 *
 * These are the functions to create mutexes:
 *  - @c le_mutex_CreateRecursive() - creates a @b normal, @b recursive mutex.
 *  - @c le_mutex_CreateNonRecursive() - creates a @b normal, @b non-recursive mutex.
 *  - @c le_mutex_CreateTraceableRecursive() - creates a @b traceable, @b recursive mutex.
 *  - @c le_mutex_CreateTraceableNonRecursive() - creates a @b traceable, @b non-recursive mutex.
 *
 * All mutexes have names, required for diagnostic purposes.  See
 * @ref c_mutex_diagnostics below.
 *
 * @section c_mutex_locking Using a Mutex
 *
 * These are the functions to lock and unlock mutexes:
 *  - @c le_mutex_Lock()
 *  - @c le_mutex_Unlock()
 *  - @c le_mutex_TryLock()
 *
 * It doesn't matter what type of mutex you are using, you
 * still use the same functions for locking and unlocking your mutex.
 *
 * @subsection c_mutex_locking_tips Tip
 *
 * @todo Do we really want this feature?
 *
 * A common case is often where a module has a single mutex  it uses for some data
 * structure that may get accessed by multiple threads. To make the locking and unlocking of that
 * mutex jump out at readers of the code (and to make coding a little easier too), the following
 * can be created in that module:
 *
 * @code
 * static le_mutex_Ref_t MyMutexRef;
 * static inline Lock(void) { le_mutex_Lock(MyMutexRef); }
 * static inline Unlock(void) { le_mutex_Unlock(MyMutexRef); }
 * @endcode
 *
 * This results in code that looks like this:
 *
 * @code
 * static void SetParam(int param)
 * {
 *     Lock();
 *    
 *     MyObjPtr->param = param;
 *    
 *     Unlock();
 * }
 * @endcode
 *
 * To make this easier, the Mutex API provides the LE_MUTEX_DECLARE_REF()
 * macro.  Using that macro, the three declaration lines
 *
 * @code
 * static le_mutex_Ref_t MyMutexRef;
 * static inline Lock(void) { le_mutex_Lock(MyMutexRef); }
 * static inline Unlock(void) { le_mutex_Unlock(MyMutexRef); }
 * @endcode
 *
 * can be replaced with one:
 *
 * @code
 * LE_MUTEX_DECLARE_REF(MyMutexRef);
 * @endcode
 *
 * @section c_mutex_delete Deleting a Mutex
 *
 * When you are finished with a mutex, you must delete it by calling le_mutex_Delete().
 *
 * There must not be anyone using the mutex when it is deleted (i.e., no one can be holding it).
 *
 * @section c_mutex_diagnostics Diagnostics
 *
 * Both Normal and Traceable mutexes have some diagnostics capabilities.
 *
 * The command-line diagnostic tool lsmutex can be used to list the mutexes
 * that currently exist inside a given process.  The state of each mutex can be
 * seen, including a list of any threads that might be waiting for that mutex.
 *
 * The tool threadlook will report if a given thread is currently
 * holding the lock on a mutex or waiting for a mutex along with the mutex name.
 *
 * If there are Traceable mutexes in a process, it's possible to use the
 * log tool to enable or disable tracing on that mutex.  The trace keyword name is
 * the name of the process, the name of the component, and the name of the mutex, separated by
 * slashes (e.g., "process/component/mutex").
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_mutex.h
 *
 * Legato @ref c_mutex include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

//--------------------------------------------------------------------------------------------------
/**
 * Create a Normal, Recursive mutex.
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a Normal, Non-Recursive mutex.
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a Traceable, Recursive mutex.
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateTraceableRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a Traceable, Non-Recursive mutex.
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateTraceableNonRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
);

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
