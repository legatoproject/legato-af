//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the statically allocated thread test.
 *
 * Spawns two statically allocated threads, performs some work to use the thread stacks, and cancels
 * the threads.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "staticThread.h"

// Base recusion depth.
#define RECURSION_MULTIPLIER    40
// Buffer size for each recursive call.
#define BUFFER_SIZE             100
// Stack size for thread 1.
#define THREAD1_STACK_SIZE      10240
// Stack size for thread 2.
#define THREAD2_STACK_SIZE      (THREAD1_STACK_SIZE * 2)

// Thread references.
static le_thread_Ref_t Thread1;
static le_thread_Ref_t Thread2;

// Allocate stacks for both threads.
LE_THREAD_DEFINE_STATIC_STACK(Thread1Stack, THREAD1_STACK_SIZE);
LE_THREAD_DEFINE_STATIC_STACK(Thread2Stack, THREAD2_STACK_SIZE);

// -------------------------------------------------------------------------------------------------
/**
 * Dummy function to prevent memset optimization.
 *
 *  By referencing the stack buffer from a weak symbol after the memset, the compiler can't optimize
 *  out the memset because this forcing function *might* be overridden later.  In reality it won't
 *  be overridden in this case.
 */
// -------------------------------------------------------------------------------------------------
__attribute__((weak)) void Force
(
    char *buffer    ///< Stack buffer that is being memset.
)
{
    LE_UNUSED(buffer);

    // Do nothing.
}

// -------------------------------------------------------------------------------------------------
/**
 * Use the stack recursively.
 *
 * @return Final recusion depth.
 */
// -------------------------------------------------------------------------------------------------
static unsigned long Recurse
(
    unsigned long number,   ///< Thread number.
    unsigned long count,    ///< Current recursion depth.
    const unsigned long max ///< Maximum recusion depth.
)
{
    char buffer[BUFFER_SIZE];

    LE_TEST_INFO("Thread %lu count: %lu @ %p", number, count, &buffer);
    memset(buffer, count & 0xFF, sizeof(buffer));
    Force(buffer); // This call prevents the memset from being optimized out.
    return (count < max ? Recurse(number, count + 1, max) : count);
}

// -------------------------------------------------------------------------------------------------
/**
 * Thread main function.
 *
 * @return Recursion result value.
 */
// -------------------------------------------------------------------------------------------------
static void *ThreadMainFunction
(
    void *threadNumber  ///< Number of the thread.
)
// -------------------------------------------------------------------------------------------------
{
    unsigned long number = (uintptr_t) threadNumber;
    unsigned long result;

    LE_TEST_INFO("Starting static thread %lu", number);
    result = Recurse(number, 0, number * RECURSION_MULTIPLIER);
    LE_TEST_INFO("Static thread %lu result: %lu", number, result);
    return (void *) result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 */
// -------------------------------------------------------------------------------------------------
void static_Start
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    Thread1 = le_thread_Create("Static1", &ThreadMainFunction, (void *) 1);
    Thread2 = le_thread_Create("Static2", &ThreadMainFunction, (void *) 2);

    LE_TEST_ASSERT(Thread1 != NULL, "Create thread 1: %p", Thread1);
    LE_TEST_ASSERT(Thread2 != NULL, "Create thread 2: %p", Thread2);

    le_thread_SetJoinable(Thread1);
    le_thread_SetJoinable(Thread2);

    LE_TEST_OK(LE_THREAD_SET_STATIC_STACK(Thread1, Thread1Stack) == LE_OK,
        "Set thread 1 stack: %" PRIuS " bytes @ %p (end %p)", sizeof(_thread_stack_Thread1Stack),
        &_thread_stack_Thread1Stack,
        (uint8_t *) &_thread_stack_Thread1Stack + sizeof(_thread_stack_Thread1Stack));
    LE_TEST_OK(LE_THREAD_SET_STATIC_STACK(Thread2, Thread2Stack) == LE_OK,
        "Set thread 2 stack: %" PRIuS " bytes @ %p (end %p)", sizeof(_thread_stack_Thread2Stack),
        &_thread_stack_Thread2Stack,
        (uint8_t *) &_thread_stack_Thread2Stack + sizeof(_thread_stack_Thread2Stack));

    le_thread_Start(Thread1);
    le_thread_Start(Thread2);
}

// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void static_CheckResults
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    void *result1Ptr;
    void *result2Ptr;

    LE_TEST_OK(le_thread_Join(Thread1, &result1Ptr) == LE_OK, "Join thread 1");
    LE_TEST_OK(le_thread_Join(Thread2, &result2Ptr) == LE_OK, "Join thread 2");

    LE_TEST_OK((uintptr_t) result1Ptr == RECURSION_MULTIPLIER * 1, "Thread 1 result: %lu",
        (unsigned long) result1Ptr);
    LE_TEST_OK((uintptr_t) result2Ptr == RECURSION_MULTIPLIER * 2, "Thread 2 result: %lu",
        (unsigned long) result2Ptr);
}
