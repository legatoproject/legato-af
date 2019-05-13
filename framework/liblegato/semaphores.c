/**
 * @file semaphores.c
 *
 * Legato @ref c_semaphore implementation.
 *
 * Each semaphore is represented by a <b> Semaphore object </b>.  They are dynamically allocated
 * from the <b> Semaphore Pool </b> and are stored on the <b> Semaphore List </b> until they are
 * destroyed.
 *
 * In addition, each thread has a <b> Per-Thread Semaphore Record </b>, which is kept in the
 * Thread object inside the thread module and is fetched through a call to
 * thread_GetSemaphoreRecPtr().
 * That Per-Thread Semaphore Record holds a pointer to a semaphore that the thread is waiting on
 * (or NULL if not waiting on a semaphore).
 *
 * Some of the tricky features of the Semaphore have to do with the diagnostic capabilities
 * provided by command-line tools.  That is, the command-line tools can ask:
 *  -# What semaphore is a given thread currently waiting on?
 *    - A single semaphore reference per thread keeps track of this (NULL if not waiting).
 *  -# What semaphores currently exist in the process?
 *    - A single per-process list of all semaphores keeps track of this (the Semaphore List).
 *  -# What threads, if any, are currently waiting on a given semaphore?
 *    - Each Semaphore object has a list of Per-Thread Semaphore Records for this.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "semaphores.h"
#include "thread.h"

// ==============================
//  PRIVATE DATA
// ==============================


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the semaphore.
 */
//--------------------------------------------------------------------------------------------------
static size_t SemaphoreListChangeCount = 0;
static size_t* SemaphoreListChangeCountRef = &SemaphoreListChangeCount;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for semaphores.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SemaphorePool, LE_CONFIG_MAX_SEM_POOL_SIZE, sizeof(Semaphore_t));


//--------------------------------------------------------------------------------------------------
/**
 * Semaphore Pool.
 *
 * Memory pool from which Semaphore objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SemaphorePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore List.
 *
 * List on which all Semaphore objects in the process are kept.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t SemaphoreList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore List Mutex.
 *
 * Basic pthreads mutex used to protect the Semaphore List from multi-threaded race conditions.
 * Helper macro functions LOCK_MUTEX_LIST() and UNLOCK_MUTEX_LIST() are provided to lock and unlock
 * this mutex respectively.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_LINUX
static pthread_mutex_t SemaphoreListMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
static pthread_mutex_t SemaphoreListMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


// ==============================
//  PRIVATE FUNCTIONS
// ==============================

/// Lock the Semaphore List Mutex.
#define LOCK_SEMAPHORE_LIST()   LE_ASSERT(pthread_mutex_lock(&SemaphoreListMutex) == 0)

/// Unlock the Semaphore List Mutex.
#define UNLOCK_SEMAPHORE_LIST() LE_ASSERT(pthread_mutex_unlock(&SemaphoreListMutex) == 0)

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SEM_NAMES_ENABLED
#   define  SEM_NAME(var) (var)
#else
#   define  SEM_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_semaphore.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_SEM_NAMES_ENABLED
LE_DEFINE_INLINE le_sem_Ref_t le_sem_Create
(
    const char  *name,
    int32_t      initialCount
);

LE_DEFINE_INLINE le_sem_Ref_t le_sem_FindSemaphore
(
    const char* name
);
#endif /* end LE_CONFIG_SEM_NAMES_ENABLED */

#if LE_CONFIG_LINUX_TARGET_TOOLS

/// Lock a semaphore's Waiting List Mutex.
#define LOCK_WAITING_LIST(semaphorePtr) \
LE_ASSERT(pthread_mutex_lock(&(semaphorePtr)->waitingListMutex) == 0)

/// Unlock a semaphore's Waiting List Mutex.
#define UNLOCK_WAITING_LIST(semaphorePtr) \
LE_ASSERT(pthread_mutex_unlock(&(semaphorePtr)->waitingListMutex) == 0)

//--------------------------------------------------------------------------------------------------
/**
 * Adds a thread's Semaphore Record to a Semaphore object's waiting list.
 */
//--------------------------------------------------------------------------------------------------
static void AddToWaitingList
(
    Semaphore_t*        semaphorePtr,
    sem_ThreadRec_t*    perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    LOCK_WAITING_LIST(semaphorePtr);

    le_dls_Queue(&semaphorePtr->waitingList, &perThreadRecPtr->waitingListLink);

    UNLOCK_WAITING_LIST(semaphorePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes a thread's Semaphore Record from a Semaphore object's waiting list.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromWaitingList
(
    Semaphore_t*        semaphorePtr,
    sem_ThreadRec_t*    perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    LOCK_WAITING_LIST(semaphorePtr);

    le_dls_Remove(&semaphorePtr->waitingList, &perThreadRecPtr->waitingListLink);

    UNLOCK_WAITING_LIST(semaphorePtr);
}
#endif /* end LE_CONFIG_LINUX_TARGET_TOOLS */


// ==============================
//  INTRA-FRAMEWORK FUNCTIONS
// ==============================

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the semaphore list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** sem_GetSemaphoreListChgCntRef
(
    void
)
{
    return (&SemaphoreListChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Semaphore module.
 *
 * This function must be called exactly once at process start-up before any other semaphore module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void sem_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    SemaphorePoolRef = le_mem_InitStaticPool(SemaphorePool, LE_CONFIG_MAX_SEM_POOL_SIZE,
        sizeof(Semaphore_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific parts of the semaphore module.
 *
 * This function must be called once by each thread when it starts, before any other semaphore module
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void sem_ThreadInit
(
    void
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_LINUX_TARGET_TOOLS
    sem_ThreadRec_t* perThreadRecPtr = thread_GetSemaphoreRecPtr();

    perThreadRecPtr->waitingOnSemaphore  = NULL;
    perThreadRecPtr->waitingListLink     = LE_DLS_LINK_INIT;
#endif

    // TODO: Register a thread destructor function to check that everything has been cleaned up
    //       properly.  (Is it possible to release semaphores inside the thread destructors?)
}


// ==============================
//  PUBLIC API FUNCTIONS
// ==============================
//--------------------------------------------------------------------------------------------------
/**
 * Create a semaphore shared by threads within the same process
 *
 * @return Upon successful completion, it shall return reference to the semaphore, otherwise,
 * assert with LE_FATAL and log.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_SEM_NAMES_ENABLED
le_sem_Ref_t le_sem_Create
(
    const char *name,
    int32_t     initialCount
)
#else
le_sem_Ref_t _le_sem_Create
(
    int32_t     initialCount        ///< [IN] initial number of semaphore
)
#endif
{
    // Allocate a semaphore object and initialize it.
    Semaphore_t* semaphorePtr = le_mem_ForceAlloc(SemaphorePoolRef);
    semaphorePtr->semaphoreListLink = LE_DLS_LINK_INIT;
#if LE_CONFIG_LINUX_TARGET_TOOLS
    semaphorePtr->waitingList = LE_DLS_LIST_INIT;
    pthread_mutex_init(&semaphorePtr->waitingListMutex, NULL);  // Default attributes = Fast mutex.
#endif

#if LE_CONFIG_SEM_NAMES_ENABLED
    if (le_utf8_Copy(semaphorePtr->nameStr, name, sizeof(semaphorePtr->nameStr), NULL) ==
        LE_OVERFLOW)
    {
        LE_WARN("Semaphore name '%s' truncated to '%s'.", name, semaphorePtr->nameStr);
    }
#endif /* end LE_CONFIG_SEM_NAMES_ENABLED */

    // Initialize the underlying POSIX semaphore shared between thread.
    int result = sem_init(&semaphorePtr->semaphore,0, initialCount);
    if (result != 0)
    {
        LE_FATAL("Failed to set the semaphore . errno = %d.", errno);
    }

    // Add the semaphore to the process's Semaphore List.
    LOCK_SEMAPHORE_LIST();
    le_dls_Queue(&SemaphoreList, &semaphorePtr->semaphoreListLink);
    UNLOCK_SEMAPHORE_LIST();

    return semaphorePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a semaphore
 *
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Delete
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore
)
{
    // Remove the Semaphore object from the Semaphore List.
    LOCK_SEMAPHORE_LIST();
    le_dls_Remove(&SemaphoreList, &semaphorePtr->semaphoreListLink);
    UNLOCK_SEMAPHORE_LIST();

#if LE_CONFIG_LINUX_TARGET_TOOLS
    LOCK_WAITING_LIST(semaphorePtr);
    if ( le_dls_Peek(&semaphorePtr->waitingList)==NULL ) {
        UNLOCK_WAITING_LIST(semaphorePtr);
        if (pthread_mutex_destroy(&semaphorePtr->waitingListMutex) != 0)
        {
            LE_FATAL(   "Semaphore '%s' could not destroy internal mutex!",
                        SEM_NAME(semaphorePtr->nameStr));
        }
    } else {
        UNLOCK_WAITING_LIST(semaphorePtr);
        // TODO print more information
        LE_FATAL("Semaphore '%s' deleted while threads are still waiting for it!",
                 SEM_NAME(semaphorePtr->nameStr));
    }
#endif

    // Destroy the semaphore.
    if (sem_destroy(&semaphorePtr->semaphore) != 0)
    {
        LE_FATAL("Semaphore '%s' is not a valid semaphore!", SEM_NAME(semaphorePtr->nameStr));
    }

    // Release the semaphore object back to the Semaphore Pool.
    le_mem_Release(semaphorePtr);
}

#if LE_CONFIG_SEM_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Finds a semaphore given the semaphore's name.
 *
 * @return
 *      Reference to the semaphore, or NULL if the semaphore doesn't exist.
 *      Invalid Name will do LE_FATAL
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t le_sem_FindSemaphore
(
    const char* name    ///< [IN] The name of the semaphore.
)
{
    le_dls_Link_t *semaphorePtr;
    Semaphore_t *Nodeptr;

    //Invalid Semaphore Name
    if ((NULL == name) || (strlen(name) > LIMIT_MAX_SEMAPHORE_NAME_LEN))
    {
        LE_FATAL("Invalid Semaphore Name");
    }
    LOCK_SEMAPHORE_LIST();
    semaphorePtr = le_dls_Peek(&SemaphoreList);
    do
    {
        if (NULL == semaphorePtr)
        {
            break;
        }
        Nodeptr = CONTAINER_OF(semaphorePtr,Semaphore_t,semaphoreListLink);
        if (0 == strcmp(name,Nodeptr->nameStr))
        {
            UNLOCK_SEMAPHORE_LIST();
            return Nodeptr;
        }
        // Get Next element
        semaphorePtr = le_dls_PeekNext(&SemaphoreList,semaphorePtr);
    } while (NULL != semaphorePtr);

    UNLOCK_SEMAPHORE_LIST();
    return NULL;
}
#endif /* end LE_CONFIG_SEM_NAMES_ENABLED */

//--------------------------------------------------------------------------------------------------
/**
 * Wait for a semaphore.
 *
 * @return Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Wait
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore
)
{
    int result;

#if LE_CONFIG_LINUX_TARGET_TOOLS
    sem_ThreadRec_t* perThreadRecPtr = thread_TryGetSemaphoreRecPtr();

    if (perThreadRecPtr)
    {
        SemaphoreListChangeCount++;
        perThreadRecPtr->waitingOnSemaphore = semaphorePtr;
        AddToWaitingList(semaphorePtr, perThreadRecPtr);
    }
#endif

    result = sem_wait(&semaphorePtr->semaphore);

#if LE_CONFIG_LINUX_TARGET_TOOLS
    if (perThreadRecPtr)
    {
        RemoveFromWaitingList(semaphorePtr, perThreadRecPtr);
        SemaphoreListChangeCount++;
        perThreadRecPtr->waitingOnSemaphore = NULL;
    }
#endif

    LE_FATAL_IF( (result!=0), "Thread '%s' failed to wait on semaphore '%s'. Error code %d.",
                le_thread_GetMyName(),
                SEM_NAME(semaphorePtr->nameStr),
                result);
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to wait for a semaphore.
 *
 * it is the same as @ref le_sem_Wait, exept that if the decrement cannot be immediately performes,
 * then call returns an LE_WOULD_BLOCK instead of blocking
 *
 * @return Upon successful completion, it shall return LE_OK (0), otherwise,
 * LE_WOULD_BLOCK as the call would block if it was a blocking call.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sem_TryWait
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore
)
{
    int result;

    result = sem_trywait(&semaphorePtr->semaphore);

    if (result != 0)
    {
        if ( errno == EAGAIN ) {
            return LE_WOULD_BLOCK;
        } else {
            LE_FATAL("Thread '%s' failed to trywait on semaphore '%s'. Error code %d.",
                    le_thread_GetMyName(),
                    SEM_NAME(semaphorePtr->nameStr),
                    result);
        }
    }

    return LE_OK;
}

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
)
{
    struct timespec timeOut;
    int result;

    // Prepare the timer
    le_clk_Time_t currentUtcTime = le_clk_GetAbsoluteTime();
    le_clk_Time_t wakeUpTime = le_clk_Add(currentUtcTime,timeToWait);
    timeOut.tv_sec = wakeUpTime.sec;
    timeOut.tv_nsec = wakeUpTime.usec * 1000;

#if LE_CONFIG_LINUX_TARGET_TOOLS
    // Retrieve reference thread
    sem_ThreadRec_t* perThreadRecPtr = thread_TryGetSemaphoreRecPtr();
    if (perThreadRecPtr)
    {
        // Save into waiting list (on Legato threads)
        SemaphoreListChangeCount++;
        perThreadRecPtr->waitingOnSemaphore = semaphorePtr;
        AddToWaitingList(semaphorePtr, perThreadRecPtr);
    }
#endif

    result = sem_timedwait(&semaphorePtr->semaphore,&timeOut);

#if LE_CONFIG_LINUX_TARGET_TOOLS
    if (perThreadRecPtr)
    {
        // Remove from waiting list (on Legato threads)
        RemoveFromWaitingList(semaphorePtr, perThreadRecPtr);
        SemaphoreListChangeCount++;
        perThreadRecPtr->waitingOnSemaphore = NULL;
    }
#endif

    if (result != 0)
    {
        if ( errno == ETIMEDOUT ) {
            return LE_TIMEOUT;
        } else {
            LE_FATAL("Thread '%s' failed to wait on semaphore '%s'. Error code %d.",
                    le_thread_GetMyName(),
                    SEM_NAME(semaphorePtr->nameStr),
                    result);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Post a semaphore.
 *
 * @return Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_sem_Post
(
    le_sem_Ref_t    semaphorePtr      ///< [IN] Pointer to the semaphore
)
{
    int result;

    result = sem_post (&semaphorePtr->semaphore);

    LE_FATAL_IF((result!=0),"Failed to post on semaphore '%s'. Errno = %d.",
                SEM_NAME(semaphorePtr->nameStr),
                result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the value of a semaphore.
 *
 * @return value of the semaphore
 */
//--------------------------------------------------------------------------------------------------
int le_sem_GetValue
(
    le_sem_Ref_t    semaphorePtr   ///< [IN] Pointer to the semaphore
)
{
    int value;

    if ( sem_getvalue(&(semaphorePtr->semaphore),&value) != 0)
    {
        LE_FATAL("Cannot get %s semaphore value", SEM_NAME(semaphorePtr->nameStr));
    }

    return value;
}
