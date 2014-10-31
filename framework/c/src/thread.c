/** @file thread.c
 *
 * This thread implementation is based on PThreads but is structured slightly differently.  Threads
 * are first created, then thread attributes are set, and finally the thread is started in a
 * seperate function call.
 *
 * When a thread is created a ThreadObj_t object is created for that thread and used to maintain
 * such things as the thread's name, attributes, destructor list, local data list, etc.  The thread
 * object is the implementation of the opaque thread reference le_thread_Ref_t given to the user.
 *
 * When a thread is started the static function PThreadStartRoutine is always executed.  The
 * PThreadStartRoutine is responsible for pushing and popping the static function
 * CleanupThread() onto and off of the pthread's clean up stack and calling the user's main
 * thread function.  This ensures that the CleanupThread() is always called when a thread exits.
 * The CleanupThread then calls the list of destructors registered for this thread and cleans
 * up the thread object itself.
 *
 * Alternatively, if a thread is started using pthreads directly, or some other pthreads wrapper
 * (such as a Boost thread object), that thread can call le_thread_InitLegatoThreadData() to
 * create a ThreadObj_t for that thread and store a pointer to it as thread-specific data using
 * the appropriate key.  This allows Legato APIs, such as the event loop, timers, and IPC to
 * work in that thread.  Furthermore, if le_thread_InitLegatoThreadData() is called for a thread
 * and that thread is to die a long time before the process dies, to prevent memory leaks
 * le_thread_CleanupLegatoThreadData() can be called by that thread (which calls CleanupThread()
 * manually).
 *
 * NOTE: If the thread only dies when the process dies, then the OS will clean up the
 * thread-specific data, so le_thread_CleanupLegatoThreadData() doesn't need to be called in that
 * case.
 *
 * Copyright (C) Sierra Wireless, Inc. 2012 - 2014.  Use of this work is subject to license.
 */

#include "legato.h"
#include "thread.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum thread name size in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_THREAD_NAME_SIZE        24

/// Expected number of threads in the process.
/// @todo Make this configurable.
#define THREAD_POOL_SIZE    4


//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map for Thread References.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ThreadRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * The legato thread structure containing all of the thread's attributes.
 *
 * @note    A Thread object created using le_thread_InitLegatoThreadData() will have its mainFunc
 *          set to NULL, and will not be joinable using le_thread_Join(), regardless of the thread's
 *          actual detach state.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char    name[MAX_THREAD_NAME_SIZE];     ///< The name of the thread.
    pthread_attr_t          attr;           ///< The thread's attributes.
    bool                    isJoinable;     ///< true = the thread is joinable, false = detached.
    bool                    isStarted;      ///< true = the thread has been started.
    le_thread_MainFunc_t    mainFunc;       ///< The main function for the thread.
    void*                   context;        ///< Context value to be passed to mainFunc.
    le_sls_List_t           destructorList; ///< The destructor list for this thread.
    mutex_ThreadRec_t       mutexRec;       ///< The thread's mutex record.
    sem_ThreadRec_t         semaphoreRec;   ///< the thread's semaphore record.
    event_PerThreadRec_t    eventRec;       ///< The thread's event record.
    pthread_t               threadHandle;   ///< The pthreads thread handle.
    le_thread_Ref_t         safeRef;        ///< Safe reference for this object.
    timer_ThreadRec_t       timerRec;       ///< The thread's timer record.
}
ThreadObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Key under which the pointer to the Thread Object (ThreadObj_t) will be kept in thread-local
 * storage.  This allows a thread to quickly get a pointer to its own Thread Object.
 */
//--------------------------------------------------------------------------------------------------
static pthread_key_t ThreadLocalDataKey;


//--------------------------------------------------------------------------------------------------
/**
 * A memory pool of thread objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ThreadPool;



//--------------------------------------------------------------------------------------------------
/**
 * The destructor object that can be added to a destructor list.  Used to hold user destructors.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Destructor_t destructor;      // The destructor function.
    void* context;                          // The context to pass to the destructor function.
    le_sls_Link_t link;                     // A link in the thread's list of destructors.
}
DestructorObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * A memory pool for the destructor objects.  This pool is shared amongst all threads.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DestructorObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect data structures within this module from multithreaded race conditions.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // Pthreads FAST mutex.



// ===================================
//  PRIVATE FUNCTIONS
// ===================================

//--------------------------------------------------------------------------------------------------
/**
 * Locks the module's mutex.
 */
//--------------------------------------------------------------------------------------------------
static inline void Lock
(
    void
)
{
    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Unlocks the module's mutex.
 */
//--------------------------------------------------------------------------------------------------
static inline void Unlock
(
    void
)
{
    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds destructor object to a given thread's Destructor List.
 */
//--------------------------------------------------------------------------------------------------
static void AddDestructor
(
    ThreadObj_t*            threadPtr,  ///< [in] Ptr to the Thread Object to add the destructor to.
    le_thread_Destructor_t  destructor, ///< [in] The function to be called.
    void*                   context     ///< [in] Parameter to pass to the destructor function.
)
{
    // Create the destructor object.
    DestructorObj_t* destructorObjPtr = le_mem_ForceAlloc(DestructorObjPool);

    // Init the destructor object.
    destructorObjPtr->destructor = destructor;
    destructorObjPtr->context = context;
    destructorObjPtr->link = LE_SLS_LINK_INIT;

    // Get a pointer to the calling thread's Thread Object and
    // Add the destructor object to its list.
    le_sls_Stack(&(threadPtr->destructorList), &(destructorObjPtr->link));
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a thread object.
 **/
//--------------------------------------------------------------------------------------------------
static void DeleteThread
(
    ThreadObj_t* threadPtr
)
{
    // Destruct the thread attributes structure.
    pthread_attr_destroy(&(threadPtr->attr));

    // Release the Thread object back to the pool it was allocated from.
    le_mem_Release(threadPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Clean-up function that gets run by a thread just before it dies.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupThread
(
    void* objPtr    ///< Pointer to the Thread object.
)
{
    ThreadObj_t* threadObjPtr = objPtr;

    // Call all destructors in the list.
    le_sls_Link_t* destructorLinkPtr = le_sls_Pop(&(threadObjPtr->destructorList));

    while (destructorLinkPtr != NULL)
    {
        // Get the destructor object
        DestructorObj_t* destructorObjPtr = CONTAINER_OF(destructorLinkPtr, DestructorObj_t, link);

        // Call the destructor.
        if (destructorObjPtr->destructor != NULL)
        {
            destructorObjPtr->destructor(destructorObjPtr->context);
        }

        // Free the destructor object.
        le_mem_Release(destructorObjPtr);

        destructorLinkPtr = le_sls_Pop(&(threadObjPtr->destructorList));
    }

    // Destruct the event loop.
    event_DestructThread();

    // If this thread is NOT joinable, then immediately invalidate its safe reference and free
    // the thread object.  Otherwise, wait until someone joins with it.
    if (! threadObjPtr->isJoinable)
    {
        Lock();

        le_ref_DeleteRef(ThreadRefMap, threadObjPtr->safeRef);

        Unlock();

        DeleteThread(threadObjPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Perform thread specific initialization for the current thread
 */
//--------------------------------------------------------------------------------------------------
void thread_InitThread
(
    void
)
{
    // Init the thread's mutex tracking structures.
    mutex_ThreadInit();

    // Init the thread's mutex.
    sem_ThreadInit();

    // Init the event loop.
    event_InitThread();

    // Init the thread's timer resources
    timer_InitThread();
}


//--------------------------------------------------------------------------------------------------
/**
 * This is a pthread start routine function wrapper.  We pass this function to the created pthread
 * and we pass the thread object as a parameter to this function.  This function then calls the
 * user's main function.
 * We do this because the user's main function has a different format then the start routine that
 * pthread expects.
 */
//--------------------------------------------------------------------------------------------------
static void* PThreadStartRoutine
(
    void* threadObjPtr
)
{
    void* returnValue;
    ThreadObj_t* threadPtr = threadObjPtr;

    // WARNING: This code must be very carefully crafted to avoid the possibility of hitting
    //          a cancellation point before pthread_cleanup_push() is called.  Otherwise, it's
    //          possible that any destructor function set before the thread was started will
    //          not get executed, which could create intermittent resource leaks.

    // Store the Thread Object pointer in thread-local storage so GetCurrentThreadPtr() can
    // find it later.
    // NOTE: pthread_setspecific() is not a cancellation point.
    if (pthread_setspecific(ThreadLocalDataKey, threadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    // Push the default destructor onto the thread's cleanup stack.
    pthread_cleanup_push(CleanupThread, threadPtr);

    // Perform thread specific init
    thread_InitThread();

    // Call the user's main function.
    returnValue = threadPtr->mainFunc(threadPtr->context);

    // Pop the default destructor and call it.
    pthread_cleanup_pop(1);

    return returnValue;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Thread object and initializes it.
 *
 * @return A reference to the thread (doesn't return if failed).
 *
 * @warning This function will also be called for the process's main thread by the processes
 *          main thread.  Keep that in mind when modifying this function.
 */
//--------------------------------------------------------------------------------------------------
static ThreadObj_t* CreateThread
(
    const char*             name,       ///< [in] Name of the thread.
    le_thread_MainFunc_t    mainFunc,   ///< [in] The thread's main function.
    void*                   context     ///< [in] Value to pass to mainFunc when it is called.
)
{
    // Create a new thread object.
    ThreadObj_t* threadPtr = le_mem_ForceAlloc(ThreadPool);

    // Copy the name.  We will make the names unique by adding the thread ID later so we allow any
    // string as the name.
    LE_WARN_IF(le_utf8_Copy(threadPtr->name, name, sizeof(threadPtr->name), NULL) == LE_OVERFLOW,
               "Thread name '%s' has been truncated to '%s'.",
               name,
               threadPtr->name);

    // Initialize the pthreads attribute structure.
    LE_ASSERT(pthread_attr_init(&(threadPtr->attr)) == 0);

    // Make sure when we create the thread it takes it attributes from the attribute object,
    // as opposed to inheriting them from its parent thread.
    if (pthread_attr_setinheritsched(&(threadPtr->attr), PTHREAD_EXPLICIT_SCHED) != 0)
    {
        LE_CRIT("Could not set scheduling policy inheritance for thread '%s'.", name);
    }

    // By default, Legato threads are not joinable (they are detached).
    if (pthread_attr_setdetachstate(&(threadPtr->attr), PTHREAD_CREATE_DETACHED) != 0)
    {
        LE_CRIT("Could not set the detached state for thread '%s'.", name);
    }

    threadPtr->isJoinable = false;
    threadPtr->isStarted = false;
    threadPtr->mainFunc = mainFunc;
    threadPtr->context = context;
    threadPtr->destructorList = LE_SLS_LIST_INIT;
    threadPtr->threadHandle = 0;

    memset(&threadPtr->mutexRec, 0, sizeof(threadPtr->mutexRec));
    memset(&threadPtr->semaphoreRec, 0, sizeof(threadPtr->semaphoreRec));
    memset(&threadPtr->eventRec, 0, sizeof(threadPtr->eventRec));
    memset(&threadPtr->timerRec, 0, sizeof(threadPtr->timerRec));

    // Create a safe reference for this object.
    Lock();
    threadPtr->safeRef = le_ref_CreateRef(ThreadRefMap, threadPtr);
    Unlock();

    return threadPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the calling thread's Thread Object.
 *
 * @return  Pointer to the thread's object.
 */
//--------------------------------------------------------------------------------------------------
static ThreadObj_t* GetCurrentThreadPtr
(
    void
)
{
    ThreadObj_t* threadPtr = pthread_getspecific(ThreadLocalDataKey);

    LE_FATAL_IF(threadPtr == NULL, "Legato threading API used in non-Legato thread!");

    return threadPtr;
}


// ===================================
//  INTER-MODULE FUNCTIONS
// ===================================

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the thread system.  This function must be called before any other thread functions
 * are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void thread_Init
(
    void
)
{
    // Create the thread memory pool.
    ThreadPool = le_mem_CreatePool("Thread Pool", sizeof(ThreadObj_t));
    le_mem_ExpandPool(ThreadPool, THREAD_POOL_SIZE);

    // Create the Safe Reference Map for Thread References.
    Lock();
    ThreadRefMap = le_ref_CreateMap("ThreadRef", THREAD_POOL_SIZE);
    Unlock();

    // Create the destructor object pool.
    DestructorObjPool = le_mem_CreatePool("DestructorObjs", sizeof(DestructorObj_t));

    // Create the thread-local data key to be used to store a pointer to each thread object.
    LE_ASSERT(pthread_key_create(&ThreadLocalDataKey, NULL) == 0);

    // Create a Thread Object for the main thread (the thread running this function).
    ThreadObj_t* threadPtr = CreateThread("main", NULL, NULL);

    // Store the Thread Object pointer in thread-local storage so GetCurrentThreadPtr() can
    // find it later.
    LE_ASSERT(pthread_setspecific(ThreadLocalDataKey, threadPtr) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's mutex record.
 */
//--------------------------------------------------------------------------------------------------
mutex_ThreadRec_t* thread_GetMutexRecPtr
(
    void
)
{
    return &((GetCurrentThreadPtr())->mutexRec);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's semaphore record.
 */
//--------------------------------------------------------------------------------------------------
sem_ThreadRec_t* thread_GetSemaphoreRecPtr
(
    void
)
{
    return &((GetCurrentThreadPtr())->semaphoreRec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's event record pointer.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetEventRecPtr
(
    void
)
{
    return &((GetCurrentThreadPtr())->eventRec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets another thread's event record.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetOtherEventRecPtr
(
    le_thread_Ref_t threadRef
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, threadRef);

    Unlock();

    LE_ASSERT(threadPtr != NULL);

    return &(threadPtr->eventRec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's timer record pointer.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* thread_GetTimerRecPtr
(
    void
)
{
    return &((GetCurrentThreadPtr())->timerRec);
}


// ===================================
//  PUBLIC API FUNCTIONS
// ===================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Legato thread of execution.  After creating the thread, you have the opportunity
 * to set attributes before it starts.  It won't start until le_thread_Start() is called.
 *
 * @return A reference to the thread (doesn't return if fails).
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t le_thread_Create
(
    const char*             name,       ///< [in] Name of the thread.
    le_thread_MainFunc_t    mainFunc,   ///< [in] The thread's main function.
    void*                   context     ///< [in] Value to pass to mainFunc when it is called.
)
{
    // Create a new thread object.
    ThreadObj_t* threadPtr = CreateThread(name, mainFunc, context);

    return threadPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the priority of a thread.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_PERMITTED if the calling thread doesn't have the necessary permission levels to
 *                          use the requested priority level.
 *      - LE_OUT_OF_RANGE if the priority level requested is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_SetPriority
(
    le_thread_Ref_t         thread,     ///< [in]
    le_thread_Priority_t    priority    ///< [in]
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);

    Unlock();

    LE_ASSERT(threadPtr != NULL);

    if (priority == LE_THREAD_PRIORITY_NORMAL)
    {
        // Set the policy to Normal.
        if (pthread_attr_setschedpolicy(&(threadPtr->attr), SCHED_OTHER) == 0)
        {
            LE_CRIT("Failed to set scheduling policy to SCHED_OTHER for thread '%s'.",
                    threadPtr->name);
        }
    }
    else if (priority <= LE_THREAD_PRIORITY_IDLE)
    {
        // Set the policy to Idle.
        if (pthread_attr_setschedpolicy(&(threadPtr->attr), SCHED_IDLE) == 0)
        {
            LE_CRIT("Failed to set scheduling policy to SCHED_IDLE for thread '%s'.",
                    threadPtr->name);
        }
    }
    else if (   (priority >= LE_THREAD_PRIORITY_RT_LOWEST)
             && (priority <= LE_THREAD_PRIORITY_RT_HIGHEST) )
    {
        struct sched_param param = {.sched_priority = priority};

        // Set the policy to a real-time policy.  Set the priority level.
        if (pthread_attr_setschedpolicy(&(threadPtr->attr), SCHED_RR) != 0)
        {
            LE_CRIT("Failed to set scheduling policy to SCHED_RR for thread '%s'.",
                    threadPtr->name);
        }
        else if (pthread_attr_setschedparam(&(threadPtr->attr), &param) != 0)
        {
            LE_CRIT("Failed to set real-time priority to %d for thread '%s'.",
                    priority,
                    threadPtr->name);
        }
    }
    else
    {
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the stack size of a thread.
 *
 * @note It is generally not necessary to set the stack size.  Some reasons why you might are:
 *         - you need to increase it beyond the system's default stack size to prevent overflow
 *              for a thread that makes extremely heavy use of the stack;
 *         - you want to decrease it to save memory when:
 *              - running in a system that does not support virtual memory
 *              - the thread has very tight real-time constraints that require that the stack
 *                  memory be locked into physical memory to avoid page faults.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_OVERFLOW if the stack size requested is too small.
 *      - LE_OUT_OF_RANGE if the stack size requested is too large.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_SetStackSize
(
    le_thread_Ref_t     thread,     ///< [in]
    size_t              size        ///< [in] Stack size, in bytes.  May be rounded up to the
                                    ///        nearest virtual memory page size.
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);

    Unlock();

    LE_ASSERT(threadPtr != NULL);

    if (pthread_attr_setstacksize(&(threadPtr->attr), size) == 0)
    {
        return LE_OK;
    }
    else if (size < PTHREAD_STACK_MIN)
    {
        return LE_OVERFLOW;
    }
    else
    {
        return LE_OUT_OF_RANGE;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Makes a thread "joinable", meaning that when it finishes, it will remain in existence until
 * another thread "joins" with it by calling le_thread_Join().  By default, threads are not
 * joinable and will be destructed automatically when they finish.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_SetJoinable
(
    le_thread_Ref_t         thread  ///< [in]
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);

    Unlock();

    LE_ASSERT(threadPtr != NULL);

    threadPtr->isJoinable = true;
    LE_ASSERT(pthread_attr_setdetachstate(&(threadPtr->attr), PTHREAD_CREATE_JOINABLE) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a new Legato thread of execution.  After creating the thread, you have the opportunity
 * to set attributes before it starts.  It won't start until le_thread_Start() is called.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_Start
(
    le_thread_Ref_t     thread      ///< [in]
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);

    Unlock();

    LE_ASSERT(threadPtr != NULL);

    // Start the thread with the default function PThreadStartRoutine, passing the
    // PThreadStartRoutine the thread object.  PThreadStartRoutine will then start the user's main
    // function.
    threadPtr->isStarted = true;
    int result = pthread_create(&(threadPtr->threadHandle),
                                &(threadPtr->attr),
                                PThreadStartRoutine,
                                threadPtr);
    if (result != 0)
    {
        int err = errno;
        LE_EMERG("pthread_create() failed with error code %d (%m).", err);
        if (err == EPERM)
        {
            LE_FATAL("Insufficient permissions to create thread '%s' with its current attributes.",
                     threadPtr->name);
        }
        else
        {
            LE_FATAL("Failed to create thread '%s'.", threadPtr->name);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * "Joins" the calling thread with another thread.  Blocks the calling thread until the other
 * thread finishes.
 *
 * After a thread has been joined with, its thread reference is no longer valid and must never
 * be used again.
 *
 * The other thread's result value (the value it returned from its main function or passed into
 * le_thread_Exit()) can be obtained.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_DEADLOCK if a thread tries to join with itself or two threads try to join each other.
 *      - LE_NOT_FOUND if the other thread doesn't exist.
 *      - LE_NOT_POSSIBLE if the other thread can't be joined with.
 *
 * @warning The other thread must be "joinable".  See le_thread_SetJoinable();
 *
 * @warning It is an error for two or more threads try to join with the same thread.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_Join
(
    le_thread_Ref_t     thread,         ///< [in]
    void**              resultValuePtr  ///< [out] Ptr to where the finished thread's result value
                                        ///        will be stored.  Can be NULL if the result is
                                        ///        not needed.
)
{
    int error;

    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);

    if (threadPtr == NULL)
    {
        Unlock();

        return LE_NOT_FOUND;
    }
    else
    {
        pthread_t pthreadHandle = threadPtr->threadHandle;
        bool isJoinable = threadPtr->isJoinable;

        Unlock();

        if (!isJoinable)
        {
            return LE_NOT_POSSIBLE;
        }
        else
        {
            error = pthread_join(pthreadHandle, resultValuePtr);

            switch (error)
            {
                case 0:
                    // If the join was successful, it's time to delete the safe reference and
                    // release the Thread Object.
                    Lock();
                    le_ref_DeleteRef(ThreadRefMap, threadPtr->safeRef);
                    Unlock();
                    DeleteThread(threadPtr);

                    return LE_OK;

                case EDEADLK:
                    return LE_DEADLOCK;

                case ESRCH:
                    return LE_NOT_FOUND;

                default:
                    LE_CRIT("Unexpected return code from pthread_join(): %d (%m)",
                            error);
                    return LE_NOT_POSSIBLE;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Terminates the calling thread.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_Exit
(
    void*   resultValue     ///< [in] The result value.  If this thread is joinable this result
                            ///         can be obtained by another thread calling le_thread_Join()
                            ///         on this thread.
)
{
    pthread_exit(resultValue);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tells another thread to terminate.  This function returns immediately but the termination of the
 * thread happens asynchronously and is not guaranteed to occur when this function returns.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if the thread doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_Cancel
(
    le_thread_Ref_t     threadToCancel
)
{
    le_result_t result = LE_OK;

    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, threadToCancel);

    if ((threadPtr == NULL) || (pthread_cancel(threadPtr->threadHandle) != 0))
    {
        result = LE_NOT_FOUND;
    }

    Unlock();

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's thread reference.
 *
 * @return  The calling thread's thread reference.
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t le_thread_GetCurrent
(
    void
)
{
    return GetCurrentThreadPtr()->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of a given thread.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_GetName
(
    le_thread_Ref_t threadRef,  ///< [in]
    char* buffPtr,              ///< [out] Buffer to store the name of the thread.
    size_t buffSize             ///< [in] The size of the buffer.
)
{
    Lock();

    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, threadRef);

    if (threadPtr == NULL)
    {
        LE_WARN("Thread %p not found.", threadPtr);
        le_utf8_Copy(buffPtr, "(dead)", buffSize, NULL);
    }
    else
    {
        LE_WARN_IF(le_utf8_Copy(buffPtr, threadPtr->name, buffSize, NULL) == LE_OVERFLOW,
                   "Thread name '%s' has been truncated to '%s'.",
                   threadPtr->name,
                   buffPtr);
    }

    Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of the calling thread. Returns "unknown" if it can't obtain the thread
 */
//--------------------------------------------------------------------------------------------------
const char* le_thread_GetMyName
(
    void
)
{
    ThreadObj_t* threadPtr = pthread_getspecific(ThreadLocalDataKey);

    if (NULL == threadPtr) return "unknown";

    return threadPtr->name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a destructor function for the calling thread.  The destructor will be called by that
 * thread just before it terminates.
 *
 * A thread can register its own destructor functions any time.
 *
 * See @ref threadDestructors for more information on destructors.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_AddDestructor
(
    le_thread_Destructor_t  destructor, ///< [in] The function to be called.
    void*                   context     ///< [in] Parameter to pass to the destructor.
)
{
    AddDestructor(GetCurrentThreadPtr(), destructor, context);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a destructor function for a child thread.  The destructor will be called by the
 * child thread just before it terminates.
 *
 * This can only be done before the child thread is started.  After that, only the child thread
 * can add its own destructors.
 *
 * The reason for allowing another thread to register a destructor function for a thread is to
 * avoid a race condition that can cause resource leakage when a parent thread passes dynamically
 * allocated resources to threads that they create.  This is only a problem if the child thread
 * is expected to release the resources when they are finished with them, and the child thread
 * may get cancelled at any time.
 *
 * For example, a thread @e T1 could allocate an object from a memory pool, create a thread @e T2,
 * and pass that object to @e T2 for processing and release.  @e T2 could register a destructor
 * function to release the resource whenever it terminates, whether through cancellation or
 * normal exit.  But, if it's possible that @e T2 could get cancelled before it even has a
 * chance to register a destructor function for itself, the memory pool object could never get
 * released.  So, we allow @e T1 to register a destructor function for @e T2 before starting @e T2.
 *
 * See @ref threadDestructors for more information on destructors.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_AddChildDestructor
(
    le_thread_Ref_t         thread,     ///< [in] The thread to attach the destructor to.
    le_thread_Destructor_t  destructor, ///< [in] The function to be called.
    void*                   context     ///< [in] Parameter to pass to the destructor.
)
{
    // Get a pointer to the thread's Thread Object.
    Lock();
    ThreadObj_t* threadPtr = le_ref_Lookup(ThreadRefMap, thread);
    Unlock();

    LE_FATAL_IF(threadPtr == NULL, "Invalid thread reference %p provided!.", thread);

    LE_FATAL_IF(threadPtr->isStarted,
                "Thread '%s' attempted to add destructor to other running thread '%s'!",
                le_thread_GetMyName(),
                threadPtr->name);

    AddDestructor(threadPtr, destructor, context);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific data needed by the Legato framework for the calling thread.
 *
 * This is used to turn a non-Legato thread (a thread that was created using a non-Legato API,
 * such as pthread_create() ) into a Legato thread.
 *
 * @note This is not needed if the thread was started using le_thread_Start().
 **/
//--------------------------------------------------------------------------------------------------
void le_thread_InitLegatoThreadData
(
    const char* name    ///< [IN] A name for the thread (will be copied, so can be temporary).
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(ThreadPool == NULL,
                "Legato C Runtime Library (liblegato) has not been initialized!");

    LE_FATAL_IF(pthread_getspecific(ThreadLocalDataKey) != NULL,
                "Legato thread-specific data initialized more than once!");

    // Create a Thread object for the calling thread.
    ThreadObj_t* threadPtr = CreateThread(name, NULL, NULL);

    // Initialize the members of the Thread object according to the current pthreads attributes.
    threadPtr->isStarted = true;

    // Store the Thread Object pointer in thread-specific storage so GetCurrentThreadPtr() can
    // find it later.
    if (pthread_setspecific(ThreadLocalDataKey, threadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    // Perform thread-specific init
    thread_InitThread();
}


//--------------------------------------------------------------------------------------------------
/**
 * Clean-up the thread-specific data that was initialized using le_thread_InitLegatoThreadData().
 *
 * To prevent memory leaks, this must be called by the thread when it dies (unless the whole
 * process is dying).
 *
 * @note This is not needed if the thread was started using le_thread_Start().
 **/
//--------------------------------------------------------------------------------------------------
void le_thread_CleanupLegatoThreadData
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    ThreadObj_t* threadPtr = GetCurrentThreadPtr();

    if (threadPtr->mainFunc != NULL)
    {
        LE_CRIT("Thread was not initialized using le_thread_InitLegatoThreadData().");
    }
    else
    {
        CleanupThread(threadPtr);
    }
}
