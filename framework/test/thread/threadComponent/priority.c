//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the thread priority test.
 *
 * At initialization time, spawns a joinable thread at each non-real-time priority level, and
 * then joins with it.
 * Real-time priority levels are not tested because those require root privileges.
 * An on-target test could be created for that.
 *
 * Each thread simply asks the kernel for its own scheduling policy to make sure it's correct.
 * If an error is detected, the test aborts immediately, so no check at the end is really needed.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "priority.h"


// -------------------------------------------------------------------------------------------------
/**
 * Thread main function.
 *
 * @return NULL.
 */
// -------------------------------------------------------------------------------------------------
static void* ThreadMainFunction
(
    void* expectedPolicy  ///< The expected Linux scheduling policy (SCHED_IDLE or SCHED_OTHER).
)
// -------------------------------------------------------------------------------------------------
{
    LE_INFO("Checking scheduling policy...");

    int schedPolicy = sched_getscheduler(0);

    if (schedPolicy == -1)
    {
        LE_FATAL("Failed to fetch scheduling policy (error %d).", errno);
    }

    if (expectedPolicy != (void*)(size_t)schedPolicy)
    {
        LE_FATAL("Expected policy %p.  Got %p.", expectedPolicy, (void*)(size_t)schedPolicy);
    }
    else
    {
        LE_INFO("Policy correct.");
    }

    return NULL;
}


// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 *
 * Increments the use count on a given memory pool object, then releases it when the test is
 * complete.
 */
// -------------------------------------------------------------------------------------------------
void prio_Start
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Note, we actually don't need to increment and decrement the memory block reference count
    // because we don't return until the test is complete.

#if LE_CONFIG_LINUX
    le_thread_Ref_t idleThread = le_thread_Create("idle", ThreadMainFunction, (void*)SCHED_IDLE);
#endif
#if LE_CONFIG_THREAD_REALTIME_ONLY
    const ssize_t expectedSched = SCHED_RR;
#else
    const ssize_t expectedSched = SCHED_OTHER;
#endif
    le_thread_Ref_t normalThread = le_thread_Create("norm", ThreadMainFunction, (void*)expectedSched);

#if LE_CONFIG_LINUX
    le_thread_SetJoinable(idleThread);
#endif
    le_thread_SetJoinable(normalThread);

#if LE_CONFIG_LINUX
    LE_ASSERT(LE_OK == le_thread_SetPriority(idleThread, LE_THREAD_PRIORITY_IDLE));
#endif
    LE_ASSERT(LE_OK == le_thread_SetPriority(normalThread, LE_THREAD_PRIORITY_NORMAL));

#if LE_CONFIG_LINUX
    le_thread_Start(idleThread);
#endif
    le_thread_Start(normalThread);

    void* unused;
    LE_ASSERT(LE_OK == le_thread_Join(normalThread, &unused));
#if LE_CONFIG_LINUX
    LE_ASSERT(LE_OK == le_thread_Join(idleThread, &unused));
#endif
}


// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void prio_CheckResults
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Actually, we don't need to do anything here.
}
