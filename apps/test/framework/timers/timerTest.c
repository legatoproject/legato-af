/**
 * This module is for unit testing the le_timer module in the legato
 * runtime library.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"

// Include macros for printing out values
#include "le_print.h"


// Number is usec ticks for one msec
#define ONE_MSEC 1000


// Tolerance to use when deciding whether the timer expired at the expected time.
//  - On desktop linux, tolerance of 2 ms seems to be sufficient
//  - On the AR7 linux target, tolerance needs to be much larger.  If the command output is
//    redirected to a file on the AR7, then 11 ms seems to be sufficient.  If the command output
//    is sent to the terminal (i.e. adb), then about 30 ms is necessary.
static le_clk_Time_t TimerTolerance = { 0, 11*ONE_MSEC };


typedef struct
{
    le_clk_Time_t interval;                  ///< Interval
    uint32_t repeatCount;                    ///< Number of times the timer will repeat

}
TimerTestData;


TimerTestData TimerTestDataArray[] =
{
    { {  5,            0 },  1 },
    { { 10,            0 },  2 },
    { { 15,            0 },  1 },

    { {  5, 100*ONE_MSEC },  1 },
    { { 10, 100*ONE_MSEC },  1 },
    { { 15, 100*ONE_MSEC },  1 },

    { {  4, 500*ONE_MSEC },  1 },
    { {  9, 500*ONE_MSEC },  1 },
    { { 14, 500*ONE_MSEC },  1 },

    // Start three timers all with the same time.  They will hopefully all expire on a
    // single timerFD expiry
    { { 12,            0 },  1 },
    { { 12,            0 },  1 },
    { { 12,            0 },  1 },

    { {  3,            0 },  8 },
};

#define NUM_TEST_TIMERS NUM_ARRAY_MEMBERS(TimerTestDataArray)


// Thread-local data key for the start time.
static pthread_key_t StartTimeKey;


// Reference to the main thread.  Used to check if a function is running in the main thread or
// the child thread.
static le_thread_Ref_t MainThread;

// Reference to the child thread.
static le_thread_Ref_t ChildThread;

// Mutex used to prevent races between the threads.
static le_mutex_Ref_t Mutex;
#define LOCK le_mutex_Lock(Mutex);
#define UNLOCK le_mutex_Unlock(Mutex);


static void ShortTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_FATAL("TEST FAILED: short timer expired");
}


static void LongTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_INFO("\n ======================================");
    // Verify that longTimer expired 6 seconds after StartTime

    le_clk_Time_t diffTime;
    le_clk_Time_t expectedInterval = { 6, 0 };

    // Since StartTime was determined before the timer was first started, diffTime should always
    // be greater than the expected interval, so no need to worry about negative time values.
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    diffTime = le_clk_Sub( le_clk_GetRelativeTime(), *startTimePtr);
    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    if ( le_clk_GreaterThan(subTime, TimerTolerance) )
    {
        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);

        LE_CRIT("TEST FAILED: Timer expiry does not match: subTime %li.%06li",
                 subTime.sec, subTime.usec);
    }
    else
    {
        LE_INFO("Long Timer expired in expected interval: TEST PASSED");
    }


    LE_INFO("\n ======================================");

    // Verify that mediumTimer (passed in as contextPtr) expired exactly once

    le_timer_Ref_t mediumTimer = le_timer_GetContextPtr(timerRef);
    uint32_t expiryCount = le_timer_GetExpiryCount(mediumTimer);

    if ( expiryCount == 1 )
    {
        LE_INFO("TEST PASSED: Medium timer expired once");
    }
    else
    {
        LE_FATAL("TEST FAILED: Medium timer expired %i times", expiryCount);
    }

    // All tests are now done, so exit
    if (le_thread_GetCurrent() == MainThread)
    {
        // Main thread joins with the child before exiting the process.
        void* threadResult;
        le_thread_Join(ChildThread, &threadResult);

        LE_INFO("ALL TESTS COMPLETE");
        exit(0);
    }
    else
    {
        // Child thread just exits so the main thread can join with it.
        le_thread_Exit(NULL);
    }
}

static void VeryShortTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    // The interval was changed to 500 ms after sleeping for 1 second, so
    // we expect expiry at 1 second.
    le_clk_Time_t expectedInterval = { 1, 0 };
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    le_clk_Time_t diffTime = le_clk_Sub( le_clk_GetRelativeTime(), *startTimePtr);
    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    if ( le_clk_GreaterThan(subTime, TimerTolerance) )
    {
        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);

        LE_CRIT("TEST FAILED: Timer expiry does not match: subTime %li.%06li",
                 subTime.sec, subTime.usec);
    }
    else
    {
        LE_INFO("TEST PASSED: Very short timer expired in expected interval.");
    }
}


static void MediumTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    // The interval was changed to 4 seconds after sleeping for 1 second, so
    // we expect expiry at 4 seconds.
    le_clk_Time_t expectedInterval = { 4, 0 };
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    le_clk_Time_t diffTime = le_clk_Sub( le_clk_GetRelativeTime(), *startTimePtr);
    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    if ( le_clk_GreaterThan(subTime, TimerTolerance) )
    {
        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);

        LE_CRIT("TEST FAILED: Timer expiry does not match: subTime %li.%06li",
                 subTime.sec, subTime.usec);
    }
    else
    {
        LE_INFO("TEST PASSED: Medium timer expired in expected interval.");
    }

    // Verify that veryShortTimer (passed in as contextPtr) expired exactly once

    le_timer_Ref_t veryShortTimer = le_timer_GetContextPtr(timerRef);
    uint32_t expiryCount = le_timer_GetExpiryCount(veryShortTimer);

    if ( expiryCount == 1 )
    {
        LE_INFO("TEST PASSED: Very short timer expired once");
    }
    else
    {
        LE_FATAL("TEST FAILED: Very short timer expired %i times", expiryCount);
    }
}


static void AdditionalTests
(
    le_timer_Ref_t oldTimer
)
{
    le_result_t result;
    le_timer_Ref_t shortTimer;
    le_timer_Ref_t mediumTimer;
    le_timer_Ref_t veryShortTimer;
    le_timer_Ref_t longTimer;
    le_clk_Time_t oneSecInterval = { 1, 0 };

    LE_INFO("\n ======================================");
    // The old timer is not running, so stop should return an error
    result = le_timer_Stop(oldTimer);
    if ( result == LE_FAULT )
    {
        LE_INFO("Stopping non-active timer TEST PASSED");
    }
    else
    {
        LE_INFO("Stopping non-active timer TEST FAILED");
    }

    LE_INFO("\n ======================================");
    // Delete the old timer, and create a new timer.  The just released timer pool block should
    // be re-used, so the timer pool should not be expanded.
    le_timer_Delete(oldTimer);

    LE_INFO("Started creating new timers");
    shortTimer = le_timer_Create("short timer from default");
    le_clk_Time_t shortTimerInterval = le_clk_Multiply(oneSecInterval, 3);
    le_timer_SetInterval( shortTimer, shortTimerInterval );
    LE_ASSERT(le_clk_Equal(le_timer_GetInterval(shortTimer), shortTimerInterval));
    le_timer_SetHandler(shortTimer, ShortTimerExpiryHandler);

    veryShortTimer = le_timer_Create("very short timer");
    le_clk_Time_t veryShortTimerInterval = {2, 500000}; // 2 s + 500000 us = 2.5 s)
    le_timer_SetInterval( veryShortTimer, veryShortTimerInterval );
    LE_ASSERT(le_timer_GetMsInterval(veryShortTimer) == 2500);
    le_timer_SetHandler(veryShortTimer, VeryShortTimerExpiryHandler);

    mediumTimer = le_timer_Create("medium timer");
    le_timer_SetInterval( mediumTimer, le_clk_Multiply(oneSecInterval, 2) );
    le_timer_SetHandler(mediumTimer, MediumTimerExpiryHandler);
    le_timer_SetContextPtr(mediumTimer, veryShortTimer); // checks that very short timer expired.

    longTimer = le_timer_Create("long timer from default");
    le_timer_SetMsInterval( longTimer, 5000 );
    le_timer_SetHandler(longTimer, LongTimerExpiryHandler);
    le_timer_SetContextPtr(longTimer, mediumTimer); // checks that medium timer expired.
    LE_ASSERT(le_timer_GetMsInterval(longTimer) == 5000);

    LE_INFO("Finished creating new timers; verify that default pool was not expanded");

    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    *startTimePtr = le_clk_GetRelativeTime();
    le_timer_Start(shortTimer);
    le_timer_Start(mediumTimer);
    le_timer_Start(veryShortTimer);
    le_timer_Start(longTimer);

    // Sleep 1 second for testing purpose only
    sleep(1);

    // Test the "get time remaining" functionality.
    le_clk_Time_t currentTime = le_clk_GetRelativeTime();
    le_clk_Time_t timeRemaining = le_timer_GetTimeRemaining(shortTimer);
    le_clk_Time_t elapsedTime = le_clk_Sub(currentTime, *startTimePtr);
    // Note: timeRemaining is fetched after currentTime, so it should be less than the
    //       timer's interval minus elapsedTime.
    if (le_clk_GreaterThan(le_clk_Sub(le_clk_Sub(shortTimerInterval, elapsedTime), timeRemaining),
                           TimerTolerance))
    {
        LE_FATAL("TEST FAILED: Time remaining is out of tolerance.");
    }
    else
    {
        LE_INFO("TEST PASSED: Time remaining was within tolerance.");
    }

    le_timer_Stop(shortTimer);
    le_clk_Time_t zero = {0, 0};
    if (le_clk_Equal(le_timer_GetTimeRemaining(shortTimer), zero))
    {
        LE_INFO("TEST PASSED: Time remaining is zero for a stopped timer.");
    }
    else
    {
        LE_FATAL("TEST FAILED: Time remaining is not zero for a stopped timer.");
    }

    // Restart the long timer so we can verify that restarting the timer will cause
    // it to expire one second later
    le_timer_Restart(longTimer);

    // Change the intervals of the veryShortTimer and the mediumTimer while they are
    // running, to ensure that they expire at the appropriate times.
    le_timer_SetMsInterval(veryShortTimer, 500); // In the past, so should expire immediately.
    le_timer_SetInterval(mediumTimer, le_clk_Multiply(oneSecInterval, 4));
}


static void TimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    le_clk_Time_t diffTime;
    le_clk_Time_t expectedInterval;
    TimerTestData* testDataPtr = le_timer_GetContextPtr(timerRef);

    // Keep the test statistics over multiple invocations
    static int testCount = 0;
    static int testPassed = 0;

    // Each timer expiry is a separate test
    LOCK
    testCount++;
    UNLOCK

    LE_INFO("\n ======================================");
    LE_INFO("Timer expired");
    LE_INFO("Expiry Count = %u", le_timer_GetExpiryCount(timerRef));

    // Note that diffTime will always be greater than the timer interval, since StartTime is
    // captured before any timers are started.  This means that the second le_clk_Sub() will
    // always return a positive time.
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    diffTime = le_clk_Sub( le_clk_GetRelativeTime(), *startTimePtr);
    expectedInterval = le_clk_Multiply(testDataPtr->interval, le_timer_GetExpiryCount(timerRef));
    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    if ( le_clk_GreaterThan(subTime, TimerTolerance) )
    {
        LE_PRINT_VALUE("%li", testDataPtr->interval.sec);
        LE_PRINT_VALUE("%li", testDataPtr->interval.usec);
        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);

        LE_CRIT("TEST FAILED: Timer expiry does not match: subTime %li.%06li",
                 subTime.sec, subTime.usec);
    }
    else
    {
        LE_INFO("TEST PASSED: %li.%06li", subTime.sec, subTime.usec);
        LOCK
        testPassed++;
        UNLOCK
    }

    // If the last timer expired the required number of times, then the expiry tests are finished
    if ( (testDataPtr == &TimerTestDataArray[NUM_TEST_TIMERS-1]) &&
         (le_timer_GetExpiryCount(timerRef) == testDataPtr->repeatCount) )
    {
        LE_INFO("EXPIRY TEST COMPLETE: %i of %i tests passed", testPassed, testCount);

        LOCK
        bool isTestFailed = (testPassed != testCount);
        UNLOCK
        if (isTestFailed)
        {
            LE_CRIT("TESTS FAILED");
        }

        // Continue with additional tests
        AdditionalTests(timerRef);
    }
}


static void TimerEventLoopTest()
{
    le_timer_Ref_t newTimer;
    int i;

    le_clk_Time_t* startTimePtr = malloc(sizeof(le_clk_Time_t));
    LE_ASSERT(startTimePtr != NULL);

    *startTimePtr = le_clk_GetRelativeTime();

    LE_ASSERT(pthread_setspecific(StartTimeKey, startTimePtr) == 0);

    for (i=0; i<NUM_TEST_TIMERS; i++)
    {
        newTimer = le_timer_Create("new timer");

        le_timer_SetInterval(newTimer, TimerTestDataArray[i].interval);
        le_timer_SetRepeat(newTimer, TimerTestDataArray[i].repeatCount);
        le_timer_SetContextPtr(newTimer, &TimerTestDataArray[i]);
        le_timer_SetHandler(newTimer, TimerExpiryHandler);

        le_timer_Start(newTimer);
    }
}

static void* ThreadMain(void* unused)
{

    TimerEventLoopTest();

    le_event_RunLoop();

    return NULL;
}


COMPONENT_INIT
{
    LE_INFO("\n");
    LE_INFO("====  Unit test for le_timer module. ====");

    MainThread = le_thread_GetCurrent();

    ChildThread = le_thread_Create("Timer Test", ThreadMain, NULL);

    Mutex = le_mutex_CreateNonRecursive("mutex");

    LE_ASSERT(pthread_key_create(&StartTimeKey, NULL) == 0);

    le_thread_SetJoinable(ChildThread);
    le_thread_Start(ChildThread);

    TimerEventLoopTest();

    LE_INFO("==== Timer Tests Started ====\n");
}

