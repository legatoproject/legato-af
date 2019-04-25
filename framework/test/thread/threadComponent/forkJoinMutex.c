//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the thread creating and joining tests.
 *
 * At initialization time, spawns a single thread and records its thread reference.
 * Each thread that runs to completion increments a mutex-protected counter variable.
 * If everything goes as expected, at the end the counter should be set to the correct value
 * and the completion check function should be able to join with that first thread that was
 * created, and the thread's result value should be its own thread reference.
 *
 * See the comment for ThreadMainFunction() for details on how the rest of this test works.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

/* NOTE:
 *  If a thread starts and then gets cancelled before it gets to register its destructor function,
 *  is that going to cause a problem?  For example, if I increment a reference count on an object
 *  and pass it to a thread, expecting that thread to release that object, is it possible that
 *  the thread gets cancelled before it has a chance to register a destructor for itself that
 *  will release the object?
 */

#include "legato.h"
#include "forkJoinMutex.h"

#ifdef LE_CONFIG_REDUCE_FOOTPRINT
#   define FAN_OUT 3
#   define DEPTH 2     // Note: the process main thread at the top level is not counted.
#else
#   define FAN_OUT 7
#   define DEPTH 3     // Note: the process main thread at the top level is not counted.
#endif

typedef struct ForkJoinTestResult
{
    bool createOk;
    bool joinOk;
    void *expectedJoin, *actualJoin;
} ForkJoinTestResult_t;

ForkJoinTestResult_t TestResults[FAN_OUT*(FAN_OUT+1)*(FAN_OUT+1)];

// The expected counter value is computed as follows:
//  - The first nesting level is the FAN_OUT threads that fjm_Init() creates.
//  - At the second nesting level, each of the FAN_OUT threads that fjm_Init() created will
//      create another FAN_OUT threads, so there will be FAN_OUT * FAN_OUT threads at that level.
//  - At the third nesting level, there will be FAN_OUT * FAN_OUT * FAN_OUT threads.
//  - Etc.
// So,
static size_t GetExpectedCounterValue
(
    void
)
{
    unsigned int result = 0;
    int i;

    for (i = 1; i <= DEPTH; i++)
    {
        int j;
        unsigned int numThreadsAtThisDepth = 1;

        for (j = 1; j <= i; j++)
        {
            numThreadsAtThisDepth *= FAN_OUT;
        }

        result += numThreadsAtThisDepth;
    }

    LE_TEST_INFO("Expecting %u threads to be created in total.", result);

    return result;
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static unsigned int Counter;

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static unsigned int ExpectedCounterValue;

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static le_sem_Ref_t CounterSemRef;

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
LE_MUTEX_DECLARE_REF(MutexRef);


// -------------------------------------------------------------------------------------------------
/**
 * Thread context blocks.
 *
 * A parent thread creates one of these, fills it with info, and passes it to all its children,
 * echo of whom releases it once.  When all children
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    size_t depth;           // Indicates what nesting level the thread is at.
                            // 1 = children of process main thread.
}
Context_t;


// -------------------------------------------------------------------------------------------------
/** Memory pool used to hold thread context blocks.
 */
// -------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ContextPoolRef = NULL;


// -------------------------------------------------------------------------------------------------
/**
 * Increment the mutex-protected counter.
 */
// -------------------------------------------------------------------------------------------------
static void IncrementCounter(void)
// -------------------------------------------------------------------------------------------------
{
    Lock();

    Counter++;

    LE_TEST_INFO("Thread '%s' incremented counter to %u.", le_thread_GetMyName(), Counter);

    if (Counter == ExpectedCounterValue)
    {
        le_sem_Post(CounterSemRef);
    }

    Unlock();
}


// -------------------------------------------------------------------------------------------------
/**
 * Destructor function for the thread.
 *
 * Releases all the memory object references that this thread owns.
 */
// -------------------------------------------------------------------------------------------------
static void ThreadDestructor
(
    void* destructorContext
)
// -------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = destructorContext;

    LE_TEST_INFO("Thread '%s' destructor running.", le_thread_GetMyName());

    le_mem_Release(contextPtr);
}


static void* ThreadMainFunction(void* objPtr);

static bool IsThreadJoinable
(
    size_t depth,
    int i
)
{
    return (depth != DEPTH) || (((i + 1) % 3) != 0);
}

// -------------------------------------------------------------------------------------------------
/**
 * Spawns FAN_OUT children, some of which are joinable and some of which are not, then tries to
 * join with all of them and checks the results.
 */
// -------------------------------------------------------------------------------------------------
static void SpawnChildren
(
    size_t depth            // Indicates what nesting level the thread is at.
                            // 1 = children of the process main thread.
)
// -------------------------------------------------------------------------------------------------
{
    int i, j, k;
    char childName[32];
    le_thread_Ref_t children[FAN_OUT];
    const char* threadName = le_thread_GetMyName();

    if (depth == 2)
    {
        LE_ASSERT(sscanf(threadName, "%*[^-]-%d", &j) == 1);
        // switch to zero-based
        j--;
        LE_TEST_INFO("depth 2: j=%d", j);
    }
    else if (depth == 3)
    {
        LE_ASSERT(sscanf(threadName, "%*[^-]-%d-%d", &k, &j) == 2);
        // switch to zero based
        k--;
        j--;
        LE_TEST_INFO("depth 3: j=%d,k=%d", j, k);
    }

    // Create and start all the children.
    for (i = 0; i < FAN_OUT; i++)
    {
        le_thread_Ref_t threadRef;

        Context_t* contextPtr = le_mem_ForceAlloc(ContextPoolRef);
        contextPtr->depth = depth;

        int item = 0;
        if (depth == 1)
        {
            item = i*(FAN_OUT+1)*(FAN_OUT+1);
        }
        if (depth == 2)
        {
            item = (j*(FAN_OUT+1)+(i+1))*(FAN_OUT+1);
        }
        else if (depth == 3)
        {
            item = (k*(FAN_OUT+1) + (j+1))*(FAN_OUT+1) + (i+1);
        }
        if (item >= FAN_OUT*(FAN_OUT+1)*(FAN_OUT+1))
        {
            LE_TEST_FATAL("Result index %d outside test result array size %d!",
                item, FAN_OUT*(FAN_OUT+1)*(FAN_OUT+1));
        }

        snprintf(childName, sizeof(childName), "%s-%d", threadName, i + 1);
        LE_TEST_INFO("Spawning thread '%s' (item %d).", childName, item);

        threadRef = le_thread_Create(childName, ThreadMainFunction, contextPtr);

        TestResults[item].createOk = !!threadRef;

        // Create a thread destructor that will release the Context object and the
        // Test Completion Object that we are going to pass to the child.
        le_thread_AddChildDestructor(threadRef, ThreadDestructor, contextPtr);

        LE_TEST_INFO("Thread '%s' destructor added.", childName);

        // Make every third leaf thread non-joinable and the rest joinable.
        // Non-leaves must be joinable to ensure join
        if (IsThreadJoinable(depth, i))
        {
            le_thread_SetJoinable(threadRef);
            LE_TEST_INFO("Thread '%s' joinability set.", childName);
        }


        // Start the child thread.
        le_thread_Start(threadRef);

        LE_TEST_INFO("Thread '%s' started.", childName);

        // Remember the child's thread reference for later join attempt.
        children[i] = threadRef;
    }

    // Join with all the children.
    for (i = 0; i < FAN_OUT; i++)
    {
        void* threadReturnValue;
        int item = 0;
        if (depth == 1)
        {
            item = i*(FAN_OUT+1)*(FAN_OUT+1);
        }
        if (depth == 2)
        {
            item = (j*(FAN_OUT+1)+(i+1))*(FAN_OUT+1);
        }
        else if (depth == 3)
        {
            item = (k*(FAN_OUT+1)+(j+1))*(FAN_OUT+1) + (i+1);
        }
        if (item >= FAN_OUT*(FAN_OUT+1)*(FAN_OUT+1))
        {
            LE_TEST_FATAL("Result index %d outside test result array size %d!",
                item, FAN_OUT*(FAN_OUT+1)*(FAN_OUT+1));
        }

        snprintf(childName, sizeof(childName), "%s-%d", le_thread_GetMyName(), i + 1);

        if (IsThreadJoinable(depth, i))
        {
            le_result_t result = le_thread_Join(children[i], &threadReturnValue);
            TestResults[item].joinOk = (result == LE_OK);

            if (result != LE_OK)
            {
                LE_TEST_INFO("Failed to join with thread '%s'.", childName);
            }
            else
            {
                LE_TEST_INFO("Successfully joined with thread '%s', which returned %p.",
                             childName,
                             threadReturnValue);
                TestResults[item].expectedJoin = (void*)depth;
                TestResults[item].actualJoin = threadReturnValue;
            }
        }
        else
        {
            // Do not try to join non-joinable threads.  Result is undefined as thread
            // could have exited in the meantime and been recycled.
            TestResults[item].joinOk = false;
        }
    }
}


// -------------------------------------------------------------------------------------------------
/**
 * Thread main function.  If it hasn't reached the full nesting depth, it will spawn a bunch
 * of threads, some joinable and some not, then cancel some of them, and try to join with all
 * of them (some should fail to join).
 *
 * For each thread it spawns, it will increment the reference count on the memory pool object
 * that was passed to it as its thread parameter and pass that same object to the child thread.
 * When this thread is done, it's destructor will release its own reference to the object.
 *
 * @return Its own thread reference.
 */
// -------------------------------------------------------------------------------------------------
static void* ThreadMainFunction
(
    void* objPtr
)
// -------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = objPtr;

    LE_TEST_INFO("Thread '%s' started.", le_thread_GetMyName());

    IncrementCounter();

    if (contextPtr->depth < DEPTH)
    {
        LE_TEST_INFO("Thread '%s' spawning children.", le_thread_GetMyName());

        SpawnChildren(contextPtr->depth + 1);
    }

    LE_TEST_INFO("Thread '%s' terminating.", le_thread_GetMyName());

    return (void*)contextPtr->depth;
}


// -------------------------------------------------------------------------------------------------
/**
 * Starts the Create/Join/Mutex tests.
 *
 * Increments the use count on a given memory pool object, then releases it when the test is
 * complete.
 */
// -------------------------------------------------------------------------------------------------
void fjm_Start
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    LE_TEST_INFO("FJM TESTS START");

    memset(TestResults, 0, sizeof(TestResults));
    Counter = 0;

    // Compute the expected ending counter value.
    ExpectedCounterValue = GetExpectedCounterValue();

    // Create semaphore to trigger
    CounterSemRef = le_sem_Create("CounterSem", 0);

    // Create the mutex.
    MutexRef = le_mutex_CreateNonRecursive("fork-join-mutex-test");

    // Create the Context Pool.
    if (ContextPoolRef == NULL)
    {
        LE_TEST_INFO("Initializing FJM-ContextPool");
        ContextPoolRef = le_mem_CreatePool("FJM-ContextPool", sizeof(Context_t));
        le_mem_ExpandPool(ContextPoolRef, ExpectedCounterValue);
    }

    // Spawn the first child thread.
    SpawnChildren(1);
}


// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of a single Create/Join/Mutex test.
 */
// -------------------------------------------------------------------------------------------------
void fjm_CheckSingleResult
(
    int fanOut1,
    int fanOut2,
    int fanOut3
)
{
    int item = (fanOut1*(FAN_OUT+1) + fanOut2)*(FAN_OUT+1) + fanOut3;
    int i = 0, depth = 0;
    if (fanOut3 != 0)
    {
        i = fanOut3-1;
        depth = 3;
    }
    else if (fanOut2 != 0)
    {
        i = fanOut2-1;
        depth = 2;
    }
    else
    {
        i = fanOut1;
        depth = 1;
    }

    LE_TEST_INFO("Gathering results for thread %d-%d-%d (item %d)",
                 fanOut1+1, fanOut2, fanOut3, item);
    LE_TEST_OK(TestResults[item].createOk,
               "thread %d-%d-%d created", fanOut1+1, fanOut2, fanOut3);
    if (IsThreadJoinable(depth, i))
    {
        LE_TEST_OK(TestResults[item].joinOk,
                   "joinable thread %d-%d-%d joined", fanOut1+1, fanOut2, fanOut3);
        LE_TEST_OK(TestResults[item].expectedJoin == TestResults[item].actualJoin,
                   "join return: expected %p, actual %p",
                   TestResults[item].expectedJoin, TestResults[item].actualJoin);
    }
    else
    {
        LE_TEST_OK(!TestResults[item].joinOk,
                   "non-joinable thread %d-%d-%d failed join", fanOut1+1, fanOut2, fanOut3);
    }
}

// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the Create/Join/Mutex tests.
 */
// -------------------------------------------------------------------------------------------------
void fjm_CheckResults
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    int i, j, k;
    int depth = DEPTH;

    le_sem_Wait(CounterSemRef);

    Lock();

    LE_TEST_OK(Counter == ExpectedCounterValue, "Counter value (%u) should be %u.",
               Counter,
               ExpectedCounterValue);
    for (i = 0; depth >= 1 && i < FAN_OUT; i++)
    {
        fjm_CheckSingleResult(i, 0, 0);
        for (j = 1; depth >= 2 && j <= FAN_OUT; j++)
        {
            fjm_CheckSingleResult(i, j, 0);
            for (k = 1; depth >= 3 && k <= FAN_OUT; k++)
            {
                fjm_CheckSingleResult(i, j, k);
            }
        }
    }

    Unlock();

    le_mutex_Delete(MutexRef);
    le_sem_Delete(CounterSemRef);
}
