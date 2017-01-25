// -------------------------------------------------------------------------------------------------
// Implementation of the thread creating and joining tests.
//
// At initialization time, spawns a single thread and records its thread reference.
// Each thread that runs to completion increments a mutex-protected counter variable.
// If everything goes as expected, at the end the counter should be set to the correct value
// and the completion check function should be able to join with that first thread that was
// created, and the thread's result value should be its own thread reference.
//
// See the comment for ThreadMainFunction() for details on how the rest of this test works.
//
// Copyright (C) Sierra Wireless Inc.
// -------------------------------------------------------------------------------------------------

/* NOTE:
 *  If a thread starts and then gets cancelled before it gets to register its destructor function,
 *  is that going to cause a problem?  For example, if I increment a reference count on an object
 *  and pass it to a thread, expecting that thread to release that object, is it possible that
 *  the thread gets cancelled before it has a chance to register a destructor for itself that
 *  will release the object?
 */

#include "legato.h"
#include "forkJoinMutex.h"

#define FAN_OUT 7
#define DEPTH 3     // Note: the process main thread at the top level is not counted.

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
    size_t result = 0;
    int i;

    for (i = 1; i <= DEPTH; i++)
    {
        int j;
        size_t numThreadsAtThisDepth = 1;

        for (j = 1; j <= i; j++)
        {
            numThreadsAtThisDepth *= FAN_OUT;
        }

        result += numThreadsAtThisDepth;
    }

    LE_INFO("Expecting %zu threads to be created in total.", result);

    return result;
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static size_t Counter = 0;

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static size_t ExpectedCounterValue;

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

    void*  completionObjPtr;// Ptr to the object whose reference count is used to terminate the test.
                            // This must have its reference count incremented before being passed
                            // to a new child thread, and every thread must release its reference
                            // when it terminates.
}
Context_t;


// -------------------------------------------------------------------------------------------------
/** Memory pool used to hold thread context blocks.
 */
// -------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ContextPoolRef;


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

    LE_INFO("Thread '%s' incremented counter to %zu.", le_thread_GetMyName(), Counter);

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

    LE_INFO("Thread '%s' destructor running.", le_thread_GetMyName());

    le_mem_Release(contextPtr->completionObjPtr);
    le_mem_Release(contextPtr);
}


static void* ThreadMainFunction(void* objPtr);

// -------------------------------------------------------------------------------------------------
/**
 * Spawns FAN_OUT children, some of which are joinable and some of which are not, then tries to
 * join with all of them and checks the results.
 */
// -------------------------------------------------------------------------------------------------
static void SpawnChildren
(
    size_t depth,           // Indicates what nesting level the thread is at.
                            // 1 = children of the process main thread.

    void*  completionObjPtr // Ptr to the object whose ref count is used to terminate the test.
)
// -------------------------------------------------------------------------------------------------
{
    int i;
    char childName[32];
    le_thread_Ref_t children[FAN_OUT];

    // Create and start all the children.
    for (i = 0; i < FAN_OUT; i++)
    {
        le_thread_Ref_t threadRef;

        Context_t* contextPtr = le_mem_ForceAlloc(ContextPoolRef);
        contextPtr->depth = depth;
        contextPtr->completionObjPtr = completionObjPtr;
        le_mem_AddRef(completionObjPtr);

        snprintf(childName, sizeof(childName), "%s-%d", le_thread_GetMyName(), i + 1);

        LE_INFO("Spawning thread '%s'.", childName);

        threadRef = le_thread_Create(childName, ThreadMainFunction, contextPtr);

        LE_INFO("Thread '%s' created.", childName);

        // Create a thread destructor that will release the Context object and the
        // Test Completion Object that we are going to pass to the child.
        le_thread_AddChildDestructor(threadRef, ThreadDestructor, contextPtr);

        LE_INFO("Thread '%s' destructor added.", childName);

        // Make every third child thread non-joinable and the rest joinable.
        if (((i + 1) % 3) != 0)
        {
            le_thread_SetJoinable(threadRef);
        }

        LE_INFO("Thread '%s' joinability set.", childName);

        // Start the child thread.
        le_thread_Start(threadRef);

        LE_INFO("Thread '%s' started.", childName);

        // Remember the child's thread reference for later join attempt.
        children[i] = threadRef;
    }

    // Join with all the children.
    for (i = 0; i < FAN_OUT; i++)
    {
        void* threadReturnValue;

        snprintf(childName, sizeof(childName), "%s-%d", le_thread_GetMyName(), i + 1);
        LE_INFO("Joining with thread '%s'.", childName);

        le_result_t result = le_thread_Join(children[i], &threadReturnValue);

        if (result != LE_OK)
        {
            LE_INFO("Failed to join with thread '%s'.", childName);
            LE_FATAL_IF(((i + 1) % 3) != 0, "Failed to join with joinable thread '%s'!", childName);
        }
        else
        {
            LE_INFO("Successfully joined with thread '%s', which returned %p.",
                    childName,
                    threadReturnValue);
            LE_FATAL_IF(((i + 1) % 3) == 0, "Joined with non-joinable thread '%s'!", childName);
            LE_FATAL_IF(threadReturnValue != completionObjPtr,
                        "Thread returned strange value %p.  Expected %p.",
                        threadReturnValue,
                        completionObjPtr);
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

    LE_INFO("Thread '%s' started.", le_thread_GetMyName());

    IncrementCounter();

    if (contextPtr->depth < DEPTH)
    {
        LE_INFO("Thread '%s' spawning children.", le_thread_GetMyName());

        SpawnChildren(contextPtr->depth + 1, contextPtr->completionObjPtr);
    }

    LE_INFO("Thread '%s' terminating.", le_thread_GetMyName());

    return contextPtr->completionObjPtr;
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
    void* completionObjPtr  ///< [in] Pointer to the object whose reference count is used to signal
                            ///       the completion of the test.
)
// -------------------------------------------------------------------------------------------------
{
    // Compute the expected ending counter value.
    ExpectedCounterValue = GetExpectedCounterValue();

    // Create the mutex.
    MutexRef = le_mutex_CreateNonRecursive("fork-join-mutex-test");

    LE_INFO("completionObjPtr = %p.", completionObjPtr);

    // Create the Context Pool.
    ContextPoolRef = le_mem_CreatePool("FJM-ContextPool", sizeof(Context_t));
    le_mem_ExpandPool(ContextPoolRef, ExpectedCounterValue);

    // Spawn the first child thread.
    SpawnChildren(1, completionObjPtr);
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
    Lock();

    if (Counter != ExpectedCounterValue)
    {
        LE_FATAL("**** FAILED - Counter value %zu should have been %zu.",
                 Counter,
                 ExpectedCounterValue);
    }

    Unlock();
}
