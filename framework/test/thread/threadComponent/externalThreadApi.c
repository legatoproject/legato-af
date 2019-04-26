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

/// Number of threads to test
#ifdef LE_CONFIG_REDUCE_FOOTPRINT
#   define NB_THREADS 10
#else
#   define NB_THREADS 100
#endif

/// Counter variable that the threads all increment and decrement.
static uint64_t Counter;

// Define MutexRef and Lock() and Unlock() functions to use to protect the Counter from races.
LE_MUTEX_DECLARE_REF(MutexRef);


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets run by all the threads.
 *
 * @return NULL.
 **/
//--------------------------------------------------------------------------------------------------
static void* ThreadMain
(
    void* argPtr   ///< [IN] mandatory thread argument; always NULL
)
{
    LE_UNUSED(argPtr);

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

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a single thread.
 **/
//--------------------------------------------------------------------------------------------------
static pthread_t StartThread
(
    pthread_attr_t* attrPtr
)
{
    pthread_t handle;

    int result = pthread_create(&handle,
                                attrPtr,
                                ThreadMain,
                                NULL);
    LE_TEST_OK(result == 0,  "pthread_create() returned result %d.", result);
    LE_TEST_INFO("thread %p created", (void*)handle);

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
    void
)
{
    // save the current number of blocks in use
    size_t initialNbOfBlocks;

    Counter = 0;

    le_mem_PoolRef_t pool = NULL;
    le_mem_PoolStats_t stats;

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_MEM_POOL_NAMES_ENABLED, 1);
    // Need to separately #ifdef this out as the internal function _le_mem_FindPool is not
    // even defined if LE_CONFIG_MEM_POOL_NAMES_ENABLED is not set
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    pool = le_mem_FindPool("ThreadPool");
#endif
    LE_TEST_ASSERT(pool != NULL, "thread pool created");
    le_mem_GetStats(pool, &stats);
    initialNbOfBlocks = stats.numBlocksInUse;
    LE_TEST_INFO("Initial numBlocksInUse=%u", (unsigned int)initialNbOfBlocks);
    LE_TEST_END_SKIP();

    MutexRef = le_mutex_CreateRecursive("externalThreadApiTest");

    pthread_attr_t attr;

    // Initialize the pthreads attribute structure.
    LE_TEST_ASSERT(pthread_attr_init(&attr) == 0, "thread attributes created");

    // Start a few threads
    int i;
    pthread_t threads[NB_THREADS];
    for (i = 0; i < NB_THREADS; i++)
    {
        threads[i] = StartThread(&attr);
    }

    // Then wait for those threads to finish
    for (i = 0; i < NB_THREADS; i++)
    {
        void* retVal;
        pthread_join(threads[i], &retVal);
    }

    // Destruct the thread attributes structure.
    pthread_attr_destroy(&attr);

    // The Counter should be back to zero.
    LE_ASSERT(Counter == 0);

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_MEM_POOL_NAMES_ENABLED, 1);
    // Check final number of blocks in use
    le_mem_GetStats(pool, &stats);
    LE_TEST_INFO("numBlocksInUse=%u", (unsigned int)stats.numBlocksInUse);
    LE_TEST_OK(stats.numBlocksInUse == initialNbOfBlocks, "no leaked blocks");
    LE_TEST_END_SKIP();
}


// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void eta_CheckResults(void)
{
    // Just clean up the mutex.
    le_mutex_Delete(MutexRef);
}
