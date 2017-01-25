//--------------------------------------------------------------------------------------------------
/**
 * Unit test implementation for starting and stopping threads using a threading API other than
 * the one defined in le_threads.h.
 *
 * Specifically, the intention is to test that we can start a thread using pthread_create(),
 * have it call le_thread_InitLegatoThreadData(), call another Legato API function that needs
 * to access thread-specific data (such as the Mutex API), and then clean up after itself using
 * le_thread_CleanupLegatoThreadData().
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "externalThreadApi.h"

/// Counter variable that the threads all increment and decrement.
static uint64_t Counter = 0;


// Define MutexRef and Lock() and Unlock() functions to use to protect the Counter from races.
LE_MUTEX_DECLARE_REF(MutexRef);


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets run by all the threads.
 *
 * @return NULL.
 **/
//--------------------------------------------------------------------------------------------------
static void* ThreadMain(void* completionObjPtr)
{
    le_thread_InitLegatoThreadData("externalApiTest");

    int i;

    for (i = 0; i < 10000; i++)
    {
        Lock();

        Counter++;

        Unlock();

        Lock();

        Counter--;

        Unlock();
    }

    le_thread_CleanupLegatoThreadData();

    le_mem_Release(completionObjPtr);   // Signal that I'm done.

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a single thread.
 **/
//--------------------------------------------------------------------------------------------------
static pthread_t StartThread
(
    pthread_attr_t* attrPtr,
    void* completionObjPtr  ///< [in] Pointer to the object whose reference count is used to signal
                            ///       the completion of the test.
)
{
    // Increment the reference count on the "completion object".  This will be released by
    // ThreadMain() when the thread completes and has cleaned up its Legato thread-specific data.
    le_mem_AddRef(completionObjPtr);

    pthread_t handle;

    int result = pthread_create(&handle,
                                attrPtr,
                                ThreadMain,
                                completionObjPtr);
    LE_FATAL_IF(result != 0,  "pthread_create() failed with errno %m.");

    return handle;
}


// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 *
 * Increments the reference count on a given memory pool object, then releases it when the test is
 * complete.
 */
// -------------------------------------------------------------------------------------------------
void eta_Start
(
    void* completionObjPtr  ///< [in] Pointer to the object whose reference count is used to signal
                            ///       the completion of the test.
)
{
    MutexRef = le_mutex_CreateRecursive("externalThreadApiTest");

    pthread_attr_t attr;

    // Initialize the pthreads attribute structure.
    LE_ASSERT(pthread_attr_init(&attr) == 0);

    int i;
    for (i = 0; i < 100; i++)
    {
        StartThread(&attr, completionObjPtr);
    }

    // Destruct the thread attributes structure.
    pthread_attr_destroy(&attr);
}


// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void eta_CheckResults(void)
{
    // The Counter should be back to zero.
    LE_ASSERT(Counter == 0);

    // We should be back to only one thread now.
    le_mem_PoolRef_t pool = _le_mem_FindPool("framework", "Thread Pool");
    LE_ASSERT(pool != NULL);
    le_mem_PoolStats_t stats;
    le_mem_GetStats(pool, &stats);
    LE_ASSERT(stats.numBlocksInUse == 1);
}
