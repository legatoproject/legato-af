/** @file mutex.c
 *
 * Legato @ref c_mutex implementation.
 *
 * Each mutex is represented by a <b> Mutex object </b>.  They are dynamically allocated from the
 * <b> Mutex Pool </b> and are stored on the <b> Mutex List </b> until they are destroyed.
 *
 * In addition, each thread has a <b> Per-Thread Mutex Record </b>, which is kept in the Thread
 * object inside the thread module and is fetched through a call to thread_GetMutexRecPtr().
 * That Per-Thread Mutex Record holds a pointer to a mutex that the thread is waiting on
 * (or NULL if not waiting on a mutex).  It also holds a list of mutexes that the thread
 * currently holds the lock for.
 *
 * Some of the tricky features of the Mutexes have to do with the diagnostic capabilities provided
 * by command-line tools.  That is, the command-line tools can ask:
 *  -# What mutexes are currently held by a given thread?
 *    - To support this, a list of locked mutexes is kept per-thread.
 *  -# What mutex is a given thread currently waiting on?
 *    - A single mutex reference per thread keeps track of this (NULL if not waiting).
 *  -# What mutexes currently exist in the process?
 *    - A single per-process list of all mutexes keeps track of this (the Mutex List).
 *  -# What threads, if any, are currently waiting on a given mutex?
 *    - Each Mutex object has a list of Per-Thread Mutex Records for this.
 *  -# What thread holds the lock on a given mutex?
 *    - Each Mutex object has a single thread reference for this (NULL if no one holds the lock).
 *  -# What is a given mutex's lock count?
 *    - Each Mutex object keeps track of its lock count.
 *  -# What type of mutex is a given mutex? (recursive?)
 *    - Stored in each Mutex object as a boolean flag.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "mutex.h"
#include "thread.h"


// ==============================
//  PRIVATE DATA
// ==============================

/// Static pool for mutexes
LE_MEM_DEFINE_STATIC_POOL(MutexPool, LE_CONFIG_MAX_MUTEX_POOL_SIZE, sizeof(Mutex_t));


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the mutex list.
 */
//--------------------------------------------------------------------------------------------------
static size_t MutexListChangeCount = 0;
static size_t* MutexListChangeCountRef = &MutexListChangeCount;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex Pool.
 *
 * Memory pool from which Mutex objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MutexPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Mutex List.
 *
 * List on which all Mutex objects in the process are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t MutexList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Mutex List Mutex.
 *
 * Basic pthreads mutex used to protect the Mutex List from multi-threaded race conditions.
 * Helper macro functions LOCK_MUTEX_LIST() and UNLOCK_MUTEX_LIST() are provided to lock and unlock
 * this mutex respectively.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_LINUX
static pthread_mutex_t MutexListMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
static pthread_mutex_t MutexListMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

//--------------------------------------------------------------------------------------------------
/**
 * All ifgen initialization shares a single mutex.  Define that mutex here.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_ifgen_InitMutexRef = NULL;

// ==============================
//  PRIVATE FUNCTIONS
// ==============================

/// Lock the Mutex List Mutex.
#define LOCK_MUTEX_LIST()   LE_ASSERT(pthread_mutex_lock(&MutexListMutex) == 0)

/// Unlock the Mutex List Mutex.
#define UNLOCK_MUTEX_LIST() LE_ASSERT(pthread_mutex_unlock(&MutexListMutex) == 0)


#if LE_CONFIG_LINUX_TARGET_TOOLS
/// Lock a mutex's Waiting List Mutex.
#define LOCK_WAITING_LIST(mutexPtr) \
            LE_ASSERT(pthread_mutex_lock(&(mutexPtr)->waitingListMutex) == 0)

/// Unlock a mutex's Waiting List Mutex.
#define UNLOCK_WAITING_LIST(mutexPtr) \
            LE_ASSERT(pthread_mutex_unlock(&(mutexPtr)->waitingListMutex) == 0)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MUTEX_NAMES_ENABLED
#   define  MUTEX_NAME(var) (var)
#else
#   define  MUTEX_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_mutex.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_EVENT_NAMES_ENABLED
LE_DEFINE_INLINE le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char *nameStr
);

LE_DEFINE_INLINE le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char *nameStr
);
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Creates a mutex.
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, so no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t CreateMutex
(
#if LE_CONFIG_MUTEX_NAMES_ENABLED
    const char* nameStr,
#endif
    bool        isRecursive
)
//--------------------------------------------------------------------------------------------------
{
    // Allocate a Mutex object and initialize it.
    Mutex_t* mutexPtr = le_mem_ForceAlloc(MutexPoolRef);
    mutexPtr->mutexListLink = LE_DLS_LINK_INIT;
    mutexPtr->lockingThreadRef = NULL;
#if LE_CONFIG_LINUX_TARGET_TOOLS
    mutexPtr->lockedByThreadLink = LE_DLS_LINK_INIT;
    mutexPtr->waitingList = LE_DLS_LIST_INIT;
    pthread_mutex_init(&mutexPtr->waitingListMutex, NULL);  // Default attributes = Fast mutex.
#endif
    mutexPtr->isRecursive = isRecursive;
    mutexPtr->lockCount = 0;

#if LE_CONFIG_MUTEX_NAMES_ENABLED
    if (le_utf8_Copy(mutexPtr->name, nameStr, sizeof(mutexPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Mutex name '%s' truncated to '%s'.", nameStr, mutexPtr->name);
    }
#endif /* end LE_CONFIG_MUTEX_NAMES_ENABLED */

    // Initialize the underlying POSIX mutex according to whether the mutex is recursive or not.
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    int mutexType;
    if (isRecursive)
    {
#if LE_CONFIG_LINUX
        mutexType = PTHREAD_MUTEX_RECURSIVE_NP;
#else
        mutexType = PTHREAD_MUTEX_RECURSIVE;
#endif
    }
    else
    {
#if LE_CONFIG_LINUX
        mutexType = PTHREAD_MUTEX_ERRORCHECK_NP;
#else
        mutexType = PTHREAD_MUTEX_ERRORCHECK;
#endif
    }
    int result = pthread_mutexattr_settype(&mutexAttrs, mutexType);
    if (result != 0)
    {
        LE_FATAL("Failed to set the mutex type to %d.  result = %d.", mutexType, result);
    }
    pthread_mutex_init(&mutexPtr->mutex, &mutexAttrs);
    pthread_mutexattr_destroy(&mutexAttrs);

    // Add the mutex to the process's Mutex List.
    LOCK_MUTEX_LIST();
    le_dls_Queue(&MutexList, &mutexPtr->mutexListLink);
    UNLOCK_MUTEX_LIST();

    return mutexPtr;
}

#if LE_CONFIG_LINUX_TARGET_TOOLS
//--------------------------------------------------------------------------------------------------
/**
 * Adds a thread's Mutex Record to a Mutex object's waiting list.
 */
//--------------------------------------------------------------------------------------------------
static void AddToWaitingList
(
    Mutex_t*            mutexPtr,
    mutex_ThreadRec_t*  perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    perThreadRecPtr->waitingOnMutex = mutexPtr;

    LOCK_WAITING_LIST(mutexPtr);

    le_dls_Queue(&mutexPtr->waitingList, &perThreadRecPtr->waitingListLink);

    UNLOCK_WAITING_LIST(mutexPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes a thread's Mutex Record from a Mutex object's waiting list.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromWaitingList
(
    Mutex_t*            mutexPtr,
    mutex_ThreadRec_t*  perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    LOCK_WAITING_LIST(mutexPtr);

    le_dls_Remove(&mutexPtr->waitingList, &perThreadRecPtr->waitingListLink);

    UNLOCK_WAITING_LIST(mutexPtr);

    perThreadRecPtr->waitingOnMutex = NULL;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Mark a mutex "locked".
 *
 * This updates all the data structures to reflect the fact that this mutex was just locked
 * by the calling thread.
 *
 * @note    Assumes that the lock count has already been updated before this function was called.
 *
 * @warning Assumes that the calling thread already holds the pthreads mutex lock.
 */
//--------------------------------------------------------------------------------------------------
static void MarkLocked
(
    mutex_ThreadRec_t*  perThreadRecPtr,    ///< [in] Pointer to the thread's mutex info record.
    Mutex_t*            mutexPtr            ///< [in] Pointer to the Mutex object that was locked.
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_LINUX_TARGET_TOOLS
    if (perThreadRecPtr)
    {
        MutexListChangeCount++;
        // Push it onto the calling thread's list of locked mutexes.
        // NOTE: Mutexes tend to be locked and unlocked in a nested manner,
        //       so treat this like a stack.
        le_dls_Stack(&perThreadRecPtr->lockedMutexList, &mutexPtr->lockedByThreadLink);

    }
#endif

    if (perThreadRecPtr)
    {
        // Record the current thread in the Mutex object as the thread that currently holds the
        // lock.
        mutexPtr->lockingThreadRef = le_thread_GetCurrent();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a mutex "unlocked".
 *
 * This updates all the data structures to reflect the fact that this mutex is about to be unlocked
 * by the calling thread.
 *
 * @note    Assumes that the lock count has already been updated before this function was called.
 *
 * @warning Assumes that the calling thread actually still holds the pthreads mutex lock.
 */
//--------------------------------------------------------------------------------------------------
static void MarkUnlocked
(
    Mutex_t* mutexPtr
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_LINUX_TARGET_TOOLS
    mutex_ThreadRec_t* perThreadRecPtr = thread_TryGetMutexRecPtr();

    if (!perThreadRecPtr)
    {
        return;
    }

    MutexListChangeCount++;
    // Remove it the calling thread's list of locked mutexes.
    le_dls_Remove(&perThreadRecPtr->lockedMutexList, &mutexPtr->lockedByThreadLink);
#endif

    // Record in the Mutex object that no thread currently holds the lock.
    mutexPtr->lockingThreadRef = NULL;
}


#if LE_CONFIG_LINUX_TARGET_TOOLS
//--------------------------------------------------------------------------------------------------
/**
 * The thread is dying.  Make sure no mutexes are held by it and clean up thread-specific data.
 **/
//--------------------------------------------------------------------------------------------------
static void ThreadDeathCleanUp
(
    void* contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    mutex_ThreadRec_t* perThreadRecPtr = contextPtr;

    if (le_dls_IsEmpty(&perThreadRecPtr->lockedMutexList) == false)
    {
        le_dls_Link_t* linkPtr = le_dls_Peek(&perThreadRecPtr->lockedMutexList);
        while (linkPtr != NULL)
        {
#if LE_CONFIG_MUTEX_NAMES_ENABLED
            Mutex_t* mutexPtr = CONTAINER_OF(linkPtr, Mutex_t, lockedByThreadLink);
#endif

            LE_EMERG("Thread died while holding mutex '%s'.", MUTEX_NAME(mutexPtr->name));

            linkPtr = le_dls_PeekNext(&perThreadRecPtr->lockedMutexList, linkPtr);
        }
        LE_FATAL("Killing process to prevent future deadlock.");
    }

    if (perThreadRecPtr->waitingOnMutex != NULL)
    {
        RemoveFromWaitingList(perThreadRecPtr->waitingOnMutex, perThreadRecPtr);
    }
}
#endif


// ==============================
//  INTRA-FRAMEWORK FUNCTIONS
// ==============================

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the mutex list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** mutex_GetMutexListChgCntRef
(
    void
)
{
    return (&MutexListChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Mutex module.
 *
 * This function must be called exactly once at process start-up before any other mutex module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void mutex_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    MutexPoolRef = le_mem_InitStaticPool(MutexPool, LE_CONFIG_MAX_MUTEX_POOL_SIZE, sizeof(Mutex_t));

    le_ifgen_InitMutexRef = le_mutex_CreateNonRecursive("ifgenMutex");
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific parts of the mutex module.
 *
 * This function must be called once by each thread when it starts, before any other mutex module
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void mutex_ThreadInit
(
    void
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_LINUX_TARGET_TOOLS
    mutex_ThreadRec_t* perThreadRecPtr = thread_GetMutexRecPtr();

    perThreadRecPtr->waitingOnMutex = NULL;
    perThreadRecPtr->lockedMutexList = LE_DLS_LIST_INIT;
    perThreadRecPtr->waitingListLink = LE_DLS_LINK_INIT;

    // Register a thread destructor function to check that everything has been cleaned up properly.
    (void)le_thread_AddDestructor(ThreadDeathCleanUp, perThreadRecPtr);
#endif
}


// ==============================
//  PUBLIC API FUNCTIONS
// ==============================

//--------------------------------------------------------------------------------------------------
/**
 * Create a Recursive mutex
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, so no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MUTEX_NAMES_ENABLED
le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
)
{
    return CreateMutex(nameStr, true);
}
#else /* if not LE_CONFIG_MUTEX_NAMES_ENABLED */
le_mutex_Ref_t _le_mutex_CreateRecursive(void)
{
    return CreateMutex(true);
}
#endif /* end LE_CONFIG_MUTEX_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Create a Non-Recursive mutex
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, so no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MUTEX_NAMES_ENABLED
le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
)
{
    return CreateMutex(nameStr, false);
}
#else /* if not LE_CONFIG_MUTEX_NAMES_ENABLED */
le_mutex_Ref_t _le_mutex_CreateNonRecursive(void)
{
    return CreateMutex(false);
}
#endif /* end LE_CONFIG_MUTEX_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Delete a mutex
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Delete
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
)
//--------------------------------------------------------------------------------------------------
{
    // Remove the Mutex object from the Mutex List.
    LOCK_MUTEX_LIST();
    le_dls_Remove(&MutexList, &mutexRef->mutexListLink);
    UNLOCK_MUTEX_LIST();

    // Destroy the pthreads mutex.
    if (pthread_mutex_destroy(&mutexRef->mutex) != 0)
    {
        if (mutexRef->lockingThreadRef)
        {
            char threadName[LIMIT_MAX_THREAD_NAME_BYTES];
            le_thread_GetName(mutexRef->lockingThreadRef, threadName, sizeof(threadName));
            LE_FATAL(   "Mutex '%s' deleted while still locked by thread '%s'!",
                        MUTEX_NAME(mutexRef->name),
                        threadName  );
        }
        else
        {
            LE_FATAL("Mutex '%s' deleted while still locked by unknown thread!",
                     MUTEX_NAME(mutexRef->name));
        }
    }

    // Release the Mutex object back to the Mutex Pool.
    le_mem_Release(mutexRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Lock a mutex
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Lock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
)
//--------------------------------------------------------------------------------------------------
{
    int result;

    mutex_ThreadRec_t* perThreadRecPtr = thread_TryGetMutexRecPtr();

#if LE_CONFIG_LINUX_TARGET_TOOLS
    if (perThreadRecPtr)
    {
        AddToWaitingList(mutexRef, perThreadRecPtr);
    }
#endif

    result = pthread_mutex_lock(&mutexRef->mutex);

#if LE_CONFIG_LINUX_TARGET_TOOLS
    if (perThreadRecPtr)
    {
        RemoveFromWaitingList(mutexRef, perThreadRecPtr);
    }
#endif

    if (result == 0)
    {
        // Got the lock!

        // NOTE: the lock count is protected by the mutex itself.  That is, it can never be
        //       updated by anyone who doesn't hold the lock on the mutex.

        // If the mutex wasn't already locked by this thread before, we need to update
        // the data structures to indicate that it now holds the lock.
        if (mutexRef->lockCount == 0)
        {
            MarkLocked(perThreadRecPtr, mutexRef);
        }

        // Update the lock count.
        mutexRef->lockCount++;
    }
    else
    {
        if (result == EDEADLK)
        {
            LE_FATAL("DEADLOCK DETECTED! Thread '%s' attempting to re-lock mutex '%s'.",
                     le_thread_GetMyName(),
                     MUTEX_NAME(mutexRef->name));
        }
        else
        {
            LE_FATAL("Thread '%s' failed to lock mutex '%s'. Error code %d.",
                     le_thread_GetMyName(),
                     MUTEX_NAME(mutexRef->name),
                     result);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Try a lock on a mutex
 *
 * Locks a mutex, if no other thread holds the mutex.  Otherwise, returns without locking.
 *
 * @return
 *  - LE_OK if mutex was locked.
 *  - LE_WOULD_BLOCK if mutex was already held by someone else.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mutex_TryLock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
)
//--------------------------------------------------------------------------------------------------
{
    int result = pthread_mutex_trylock(&mutexRef->mutex);

    if (result == 0)
    {
        // Got the lock!

        // NOTE: the lock count is protected by the mutex itself.  That is, it can never be
        //       updated by anyone who doesn't hold the lock on the mutex.

        // If the mutex wasn't already locked by this thread before, we need to update
        // the data structures to indicate that it now holds the lock.
        if (mutexRef->lockCount == 0)
        {
            mutex_ThreadRec_t* perThreadRecPtr = thread_TryGetMutexRecPtr();

            MarkLocked(perThreadRecPtr, mutexRef);
        }

        // Update the lock count.
        mutexRef->lockCount++;
    }
    else if (result == EBUSY)
    {
        // The mutex is already held by someone else.
        return LE_WOULD_BLOCK;
    }
    else
    {
        LE_FATAL("Thread '%s' failed to trylock mutex '%s'. Error code %d.",
                 le_thread_GetMyName(),
                 MUTEX_NAME(mutexRef->name),
                 result);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Unlock a mutex
 */
//--------------------------------------------------------------------------------------------------
void le_mutex_Unlock
(
    le_mutex_Ref_t    mutexRef   ///< [in] Mutex reference
)
//--------------------------------------------------------------------------------------------------
{
    int result;

    le_thread_Ref_t lockingThread = mutexRef->lockingThreadRef;

    // Make sure that the lock count is at least 1.
    LE_FATAL_IF(mutexRef->lockCount <= 0,
                "Mutex '%s' unlocked too many times!",
                MUTEX_NAME(mutexRef->name));

    // Check locking thread if there is one.  If not, ensure this is a non-Legato thread
    if (lockingThread)
    {
        le_thread_Ref_t currentThread = le_thread_GetCurrent();

        // Make sure that the current thread is the one holding the mutex lock.
        if (lockingThread != currentThread)
        {
            char threadName[LIMIT_MAX_THREAD_NAME_BYTES];
            le_thread_GetName(lockingThread, threadName, sizeof(threadName));
            LE_FATAL("Attempt to unlock mutex '%s' held by other thread '%s'.",
                     MUTEX_NAME(mutexRef->name),
                     threadName);
        }
    }
    else
    {
        LE_FATAL_IF(thread_TryGetMutexRecPtr(),
                    "Attempt to unlock mutex '%s' from a non-Legato thread",
                    MUTEX_NAME(mutexRef->name));
    }


    // Update the lock count.
    // NOTE: the lock count is protected by the mutex itself.  That is, it can never be
    //       updated by anyone who doesn't hold the lock on the mutex.
    mutexRef->lockCount--;

    // If we have now reached a lock count of zero, the mutex is about to be unlocked, so
    // Update the data structures to reflect that the current thread no longer holds the
    // mutex.
    if (mutexRef->lockCount == 0)
    {
        MarkUnlocked(mutexRef);
    }

    // Warning!  If the lock count is now zero, then as soon as we call this function another
    // thread may grab the lock.
    result = pthread_mutex_unlock(&mutexRef->mutex);
    if (result != 0)
    {
        LE_FATAL("Failed to unlock mutex '%s'. Errno = %d.",
                 MUTEX_NAME(mutexRef->name),
                 result );
    }
}
