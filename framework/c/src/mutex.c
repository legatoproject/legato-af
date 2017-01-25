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

/// Number of objects in the Mutex Pool to start with.
/// TODO: Change this to be configurable per-process.
#define DEFAULT_POOL_SIZE 4


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
static pthread_mutex_t MutexListMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


// ==============================
//  PRIVATE FUNCTIONS
// ==============================

/// Lock the Mutex List Mutex.
#define LOCK_MUTEX_LIST()   LE_ASSERT(pthread_mutex_lock(&MutexListMutex) == 0)

/// Unlock the Mutex List Mutex.
#define UNLOCK_MUTEX_LIST() LE_ASSERT(pthread_mutex_unlock(&MutexListMutex) == 0)


/// Lock a mutex's Waiting List Mutex.
#define LOCK_WAITING_LIST(mutexPtr) \
            LE_ASSERT(pthread_mutex_lock(&(mutexPtr)->waitingListMutex) == 0)

/// Unlock a mutex's Waiting List Mutex.
#define UNLOCK_WAITING_LIST(mutexPtr) \
            LE_ASSERT(pthread_mutex_unlock(&(mutexPtr)->waitingListMutex) == 0)


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
    const char* nameStr,
    bool        isRecursive
)
//--------------------------------------------------------------------------------------------------
{
    // Allocate a Mutex object and initialize it.
    Mutex_t* mutexPtr = le_mem_ForceAlloc(MutexPoolRef);
    mutexPtr->mutexListLink = LE_DLS_LINK_INIT;
    mutexPtr->lockingThreadRef = NULL;
    mutexPtr->lockedByThreadLink = LE_DLS_LINK_INIT;
    mutexPtr->waitingList = LE_DLS_LIST_INIT;
    pthread_mutex_init(&mutexPtr->waitingListMutex, NULL);  // Default attributes = Fast mutex.
    mutexPtr->isRecursive = isRecursive;
    mutexPtr->lockCount = 0;
    if (le_utf8_Copy(mutexPtr->name, nameStr, sizeof(mutexPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Mutex name '%s' truncated to '%s'.", nameStr, mutexPtr->name);
    }

    // Initialize the underlying POSIX mutex according to whether the mutex is recursive or not.
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    int mutexType;
    if (isRecursive)
    {
        mutexType = PTHREAD_MUTEX_RECURSIVE_NP;
    }
    else
    {
        mutexType = PTHREAD_MUTEX_ERRORCHECK_NP;
    }
    int result = pthread_mutexattr_settype(&mutexAttrs, mutexType);
    if (result != 0)
    {
        LE_FATAL("Failed to set the mutex type to %d.  errno = %d (%m).", mutexType, errno);
    }
    pthread_mutex_init(&mutexPtr->mutex, &mutexAttrs);
    pthread_mutexattr_destroy(&mutexAttrs);

    // Add the mutex to the process's Mutex List.
    LOCK_MUTEX_LIST();
    le_dls_Queue(&MutexList, &mutexPtr->mutexListLink);
    UNLOCK_MUTEX_LIST();

    return mutexPtr;
}


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
    MutexListChangeCount++;
    // Push it onto the calling thread's list of locked mutexes.
    // NOTE: Mutexes tend to be locked and unlocked in a nested manner, so treat this like a stack.
    le_dls_Stack(&perThreadRecPtr->lockedMutexList, &mutexPtr->lockedByThreadLink);

    // Record the current thread in the Mutex object as the thread that currently holds the lock.
    mutexPtr->lockingThreadRef = le_thread_GetCurrent();
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
    mutex_ThreadRec_t* perThreadRecPtr = thread_GetMutexRecPtr();

    MutexListChangeCount++;
    // Remove it the calling thread's list of locked mutexes.
    le_dls_Remove(&perThreadRecPtr->lockedMutexList, &mutexPtr->lockedByThreadLink);

    // Record in the Mutex object that no thread currently holds the lock.
    mutexPtr->lockingThreadRef = NULL;
}


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
            Mutex_t* mutexPtr = CONTAINER_OF(linkPtr, Mutex_t, lockedByThreadLink);

            LE_EMERG("Thread died while holding mutex '%s'.", mutexPtr->name);

            linkPtr = le_dls_PeekNext(&perThreadRecPtr->lockedMutexList, linkPtr);
        }
        LE_FATAL("Killing process to prevent future deadlock.");
    }

    if (perThreadRecPtr->waitingOnMutex != NULL)
    {
        RemoveFromWaitingList(perThreadRecPtr->waitingOnMutex, perThreadRecPtr);
    }
}


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
    MutexPoolRef = le_mem_CreatePool("mutex", sizeof(Mutex_t));
    le_mem_ExpandPool(MutexPoolRef, DEFAULT_POOL_SIZE);
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
    mutex_ThreadRec_t* perThreadRecPtr = thread_GetMutexRecPtr();

    perThreadRecPtr->waitingOnMutex = NULL;
    perThreadRecPtr->lockedMutexList = LE_DLS_LIST_INIT;
    perThreadRecPtr->waitingListLink = LE_DLS_LINK_INIT;

    // Register a thread destructor function to check that everything has been cleaned up properly.
    (void)le_thread_AddDestructor(ThreadDeathCleanUp, perThreadRecPtr);
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
le_mutex_Ref_t le_mutex_CreateRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
)
//--------------------------------------------------------------------------------------------------
{
    return CreateMutex(nameStr, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Non-Recursive mutex
 *
 * @return  Returns a reference to the mutex.
 *
 * @note Terminates the process on failure, so no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mutex_Ref_t le_mutex_CreateNonRecursive
(
    const char* nameStr     ///< [in] Name of the mutex
)
//--------------------------------------------------------------------------------------------------
{
    return CreateMutex(nameStr, false);
}


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

    if (mutexRef->lockingThreadRef != NULL)

    // Destroy the pthreads mutex.
    if (pthread_mutex_destroy(&mutexRef->mutex) != 0)
    {
        char threadName[LIMIT_MAX_THREAD_NAME_BYTES];
        le_thread_GetName(mutexRef->lockingThreadRef, threadName, sizeof(threadName));
        LE_FATAL(   "Mutex '%s' deleted while still locked by thread '%s'!",
                    mutexRef->name,
                    threadName  );
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

    mutex_ThreadRec_t* perThreadRecPtr = thread_GetMutexRecPtr();

    AddToWaitingList(mutexRef, perThreadRecPtr);

    result = pthread_mutex_lock(&mutexRef->mutex);

    RemoveFromWaitingList(mutexRef, perThreadRecPtr);

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
                     mutexRef->name);
        }
        else
        {
            LE_FATAL("Thread '%s' failed to lock mutex '%s'. Error code %d (%m).",
                     le_thread_GetMyName(),
                     mutexRef->name,
                     result );
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
            MarkLocked(thread_GetMutexRecPtr(), mutexRef);
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
        LE_FATAL("Thread '%s' failed to trylock mutex '%s'. Error code %d (%m).",
                 le_thread_GetMyName(),
                 mutexRef->name,
                 result );
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
    le_thread_Ref_t currentThread = le_thread_GetCurrent();

    // Make sure that the lock count is at least 1.
    LE_FATAL_IF(mutexRef->lockCount <= 0,
                "Mutex '%s' unlocked too many times!",
                mutexRef->name);

    // Make sure that the current thread is the one holding the mutex lock.
    if (lockingThread != currentThread)
    {
        char threadName[LIMIT_MAX_THREAD_NAME_BYTES];
        le_thread_GetName(lockingThread, threadName, sizeof(threadName));
        LE_FATAL("Attempt to unlock mutex '%s' held by other thread '%s'.",
                 mutexRef->name,
                 threadName);
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
        LE_FATAL("Failed to unlock mutex '%s'. Errno = %d (%m).",
                 mutexRef->name,
                 result );
    }
}
