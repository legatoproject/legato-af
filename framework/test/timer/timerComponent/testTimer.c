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

#ifndef CONFIG_LINUX
#   define CONFIG_LINUX 0
#endif


// Number is usec ticks for one msec
#define ONE_MSEC 1000

// One test per timer, plus some additional tests after
#define TESTS_PER_TIMER 1
#define ADDITIONAL_TEST_COUNT 18

// Format and log time values
#define LOG_TIME_MSG(msg, tm) \
    LE_TEST_INFO("%20s %ld.%03ld s", (msg), (tm).sec, (tm).usec / ONE_MSEC);
#define LOG_TIME(tm) LOG_TIME_MSG(#tm, (tm))

// Tolerance to use when deciding whether the timer expired at the expected time.
//  - On desktop linux, tolerance of 2 ms seems to be sufficient
//  - On the AR7 linux target, tolerance needs to be much larger.  If the command output is
//    redirected to a file on the AR7, then 11 ms seems to be sufficient.  If the command output
//    is sent to the terminal (i.e. adb), then about 30 ms is necessary.
//  - On VIRT system 100ms may be needed
//  - On RTOS system 200 ms is sometimes needed
static le_clk_Time_t TimerTolerance = { 0, 200*ONE_MSEC };


typedef struct
{
    le_clk_Time_t interval;                  ///< Interval
    uint32_t repeatCount;                    ///< Number of times the timer will repeat
    le_clk_Time_t offset;                    ///< Clock offset from base time
}
TimerTestData;


static TimerTestData TimerTestDataArray[] =
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

    { { 25,            0 },  1 },
    { { 25,            0 },  1 },
    { { 25,            0 },  1 },
    { { 25,            0 },  1 },
    { { 25,            0 },  1 },
    { { 25,            0 },  3 },
    { { 25,            0 },  1 },
    { { 25,            0 },  1 },
};

#define NUM_TEST_TIMERS NUM_ARRAY_MEMBERS(TimerTestDataArray)

// Test counts
static int Expired;
static int Total;
static int Passed;

// Thread-local data key for the start time.
static pthread_key_t StartTimeKey;

// Reference to the main thread.  Used to check if a function is running in the main thread or
// the child thread.
static le_thread_Ref_t MainThread;

// Reference to the child thread.
static le_thread_Ref_t ChildThread;

// Mutex used to prevent races between the threads.
static le_mutex_Ref_t Mutex;
#define LOCK()      le_mutex_Lock(Mutex);
#define UNLOCK()    le_mutex_Unlock(Mutex);

static void ShortTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_TEST_FATAL("TEST FAILED: short timer expired");
}


static void LongTimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    // Verify that longTimer expired 6 seconds after StartTime

    le_clk_Time_t diffTime;
    le_clk_Time_t expectedInterval = { 6, 0 };

    // Since StartTime was determined before the timer was first started, diffTime should always
    // be greater than the expected interval, so no need to worry about negative time values.
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    diffTime = le_clk_Sub( le_clk_GetRelativeTime(), *startTimePtr);
    LE_TEST_INFO("\n ======================================");
    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    bool testFailed = le_clk_GreaterThan(subTime, TimerTolerance);

    LE_TEST_OK(!testFailed, "timer accuracy within tolerance");
    if ( testFailed )
    {
        LOG_TIME(expectedInterval);
        LOG_TIME(diffTime);
        LOG_TIME(subTime);
    }


    // Verify that mediumTimer (passed in as contextPtr) expired exactly once

    le_timer_Ref_t mediumTimer = le_timer_GetContextPtr(timerRef);
    uint32_t expiryCount = le_timer_GetExpiryCount(mediumTimer);

    LE_TEST_OK(expiryCount == 1, "Medium timer expired once (expired %"PRIu32" times)",
               expiryCount);

    // All tests are now done, so exit
    LE_TEST_INFO("Tests ended");
    LE_TEST_EXIT;
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
    bool testFailed = le_clk_GreaterThan(subTime, TimerTolerance);
    LE_TEST_OK(!testFailed, "very short timer accuracy within tolerance");
    if ( testFailed )
    {
        LOG_TIME(expectedInterval);
        LOG_TIME(diffTime);
        LOG_TIME(subTime);
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
    bool testFailed = le_clk_GreaterThan(subTime, TimerTolerance);
    LE_TEST_OK(!testFailed, "medium timer accuracy within tolerance");
    if ( testFailed )
    {
        LOG_TIME(expectedInterval);
        LOG_TIME(diffTime);
        LOG_TIME(subTime);
    }

    // Verify that veryShortTimer (passed in as contextPtr) expired exactly once

    le_timer_Ref_t veryShortTimer = le_timer_GetContextPtr(timerRef);
    uint32_t expiryCount = le_timer_GetExpiryCount(veryShortTimer);

    LE_TEST_OK(expiryCount == 1, "Very short timer expired once (expired %"PRIu32" times)",
               expiryCount);
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

    LE_TEST_INFO("\n ==================== Additional Tests =================");
    // The old timer is not running, so stop should return an error
    result = le_timer_Stop(oldTimer);
    LE_TEST_OK(result == LE_FAULT, "Stopping non-active timer");

    // Delete the old timer, and create a new timer.  The just released timer pool block should
    // be re-used, so the timer pool should not be expanded.
    le_timer_Delete(oldTimer);

    shortTimer = le_timer_Create("short timer from default");
    LE_TEST_ASSERT(shortTimer, "created new short timer by modifying started 1 s timer");
    le_clk_Time_t shortTimerInterval = le_clk_Multiply(oneSecInterval, 3);
    le_timer_SetInterval( shortTimer, oneSecInterval );
    le_timer_SetHandler(shortTimer, ShortTimerExpiryHandler);
    le_timer_Start(shortTimer);
    // Check changing timer interval when only one timer is running.
    // This tests for a regression on LE-10200
    LE_TEST_OK(le_timer_SetInterval(shortTimer, shortTimerInterval) == LE_OK,
               "Set time on running short timer");
    LE_TEST_OK(le_clk_Equal(le_timer_GetInterval(shortTimer), shortTimerInterval),
               "short timer interval set");

    veryShortTimer = le_timer_Create("very short timer");
    LE_TEST_ASSERT(veryShortTimer, "created new very short timer");
    le_clk_Time_t veryShortTimerInterval = {2, 500000}; // 2 s + 500000 us = 2.5 s)
    le_timer_SetInterval( veryShortTimer, veryShortTimerInterval );
    LE_TEST_OK(le_timer_GetMsInterval(veryShortTimer) == 2500, "set very short timer interval");
    le_timer_SetHandler(veryShortTimer, VeryShortTimerExpiryHandler);

    mediumTimer = le_timer_Create("medium timer");
    LE_TEST_ASSERT(mediumTimer, "created medium timer");
    le_timer_SetInterval( mediumTimer, le_clk_Multiply(oneSecInterval, 2) );
    le_timer_SetHandler(mediumTimer, MediumTimerExpiryHandler);
    le_timer_SetContextPtr(mediumTimer, veryShortTimer); // checks that very short timer expired.

    longTimer = le_timer_Create("long timer from default");
    LE_TEST_ASSERT(longTimer, "created long timer");
    le_timer_SetMsInterval( longTimer, 5000 );
    le_timer_SetHandler(longTimer, LongTimerExpiryHandler);
    le_timer_SetContextPtr(longTimer, mediumTimer); // checks that medium timer expired.
    LE_TEST_OK(le_timer_GetMsInterval(longTimer) == 5000, "set long timer interval");
    LE_TEST_INFO("Finished creating new timers; verify that default pool was not expanded");

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
    bool testFailed = le_clk_GreaterThan(le_clk_Sub(le_clk_Sub(shortTimerInterval, elapsedTime), timeRemaining),
                                         TimerTolerance);
    LE_TEST_OK(!testFailed, "Time remaining was within tolerance.");
    if (testFailed)
    {
        LOG_TIME(timeRemaining);
        LOG_TIME(elapsedTime);
        LOG_TIME(TimerTolerance);
    }

    le_timer_Stop(shortTimer);
    le_clk_Time_t zero = {0, 0};
    LE_TEST_OK(le_clk_Equal(le_timer_GetTimeRemaining(shortTimer), zero),
               "Time remaining is zero for a stopped timer.");

    // Restart the long timer so we can verify that restarting the timer will cause
    // it to expire one second later
    le_timer_Restart(longTimer);

    // Change the intervals of the veryShortTimer and the mediumTimer while they are
    // running, to ensure that they expire at the appropriate times.

    // Set veryShortTimer in the past, so should expire immediately.
    LE_TEST_OK(le_timer_SetMsInterval(veryShortTimer, 500) == LE_OK,
               "Setting veryShortTimer to 500 ms");

    // Set medium timer to 4s.
    LE_TEST_OK(le_timer_SetInterval(mediumTimer, le_clk_Multiply(oneSecInterval, 4)) == LE_OK,
               "Setting mediumTimer to 4 s");
}


static void TimerExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    bool             isMain = (le_thread_GetCurrent() == MainThread);
    intptr_t         testDataIndex = (intptr_t) le_timer_GetContextPtr(timerRef);
    le_clk_Time_t    diffTime;
    le_clk_Time_t    expectedInterval;
    le_clk_Time_t    relativeTime = le_clk_GetRelativeTime();
    le_clk_Time_t    tolerance;
    TimerTestData   *testDataPtr = &TimerTestDataArray[testDataIndex];
    uint32_t         expiryCount = le_timer_GetExpiryCount(timerRef);

    le_clk_Time_t    startTime = le_clk_GetRelativeTime();

    // Keep the previous expected interval and the last handler execution time
    static le_clk_Time_t prevInterval;
    static le_clk_Time_t prevExecTime;

    LE_TEST_INFO("\n ======================================");
    LE_TEST_INFO("%s timer %d expired", (isMain ? "Main" : "Child"), (int) testDataIndex);
    LE_TEST_INFO("Expiry Count = %" PRIu32, expiryCount);

    // Note that diffTime will always be greater than the timer interval, since StartTime is
    // captured before any timers are started.  This means that the second le_clk_Sub() will
    // always return a positive time.
    le_clk_Time_t* startTimePtr = pthread_getspecific(StartTimeKey);
    LE_ASSERT(startTimePtr != NULL);
    diffTime = le_clk_Sub( relativeTime, le_clk_Add(*startTimePtr, testDataPtr->offset));
    expectedInterval = le_clk_Multiply(testDataPtr->interval, expiryCount);

    // increase the tolerance when there are several timers which expire at the same time
    // this is needed because the handler can take long time to execute depending on the
    // log system
    tolerance = TimerTolerance;
    if (le_clk_Equal(prevInterval, expectedInterval))
    {
        tolerance = le_clk_Add(tolerance, prevExecTime);
    }

    le_clk_Time_t subTime = le_clk_Sub(diffTime, expectedInterval);
    bool testFailed = le_clk_GreaterThan(subTime, tolerance);

    LOCK();
    LE_TEST_OK(!testFailed, "%s %d timer accuracy within tolerance",
        (isMain ? "Main" : "Child"), (int) testDataIndex);
    if ( testFailed )
    {
        LOG_TIME_MSG("testDataPtr", testDataPtr->interval);
        LOG_TIME(expectedInterval);
        LOG_TIME(diffTime);
        LOG_TIME(subTime);
        LOG_TIME(tolerance);
    }
    else
    {
        __atomic_add_fetch(&Passed, 1, __ATOMIC_RELAXED);
    }
    UNLOCK();

    // If the last timer expired the required number of times, then the expiry tests are finished
    if (__atomic_add_fetch(&Expired, 1, __ATOMIC_RELAXED) == Total)
    {
        LE_TEST_INFO("EXPIRY TEST COMPLETE: %i of %i tests passed", Passed, Total);
        if (Passed != Total)
        {
            LE_TEST_INFO("%i TESTS FAILED", Total - Passed);
        }

        if (isMain)
        {
#if CONFIG_LINUX
            // Child thread just exits so the main thread can join with it.
            le_thread_Cancel(ChildThread);

            LE_TEST_INFO("Waiting for child thread to join...");

            // Main thread joins with the child before continuing with additional tests.
            void* threadResult;
            le_thread_Join(ChildThread, &threadResult);
#endif /* end CONFIG_LINUX */

            // Continue with additional tests
            AdditionalTests(timerRef);
        }
        else
        {
            // Child thread just exits so the main thread can join with it.
            le_thread_Exit(NULL);
        }
    }

    le_clk_Time_t execTime = le_clk_Sub(le_clk_GetRelativeTime(), startTime);
    if (le_clk_Equal(prevInterval, expectedInterval))
    {
        prevExecTime = le_clk_Add(execTime, prevExecTime);
    }
    else
    {
        prevInterval = expectedInterval;
    }
}


static void TimerEventLoopTest(void)
{
    le_timer_Ref_t newTimer;
    int i;

    le_clk_Time_t* startTimePtr = malloc(sizeof(le_clk_Time_t));
    LE_ASSERT(startTimePtr != NULL);
    LE_ASSERT(pthread_setspecific(StartTimeKey, startTimePtr) == 0);

    for (i = 0; i < NUM_TEST_TIMERS; ++i)
    {
        LE_TEST_INFO("Starting %s %ld.%03ld s timer %d (%" PRIu32 " repeats)",
            (le_thread_GetCurrent() == MainThread ? "main" : "child"),
            TimerTestDataArray[i].interval.sec, TimerTestDataArray[i].interval.usec / ONE_MSEC, i,
            TimerTestDataArray[i].repeatCount - 1);
    }

    *startTimePtr = le_clk_GetRelativeTime();
    for (i=0; i<NUM_TEST_TIMERS; i++)
    {
        newTimer = le_timer_Create("new timer");

        le_timer_SetInterval(newTimer, TimerTestDataArray[i].interval);
        le_timer_SetRepeat(newTimer, TimerTestDataArray[i].repeatCount);
        le_timer_SetContextPtr(newTimer, (void *) (intptr_t) i);
        le_timer_SetHandler(newTimer, TimerExpiryHandler);

        le_clk_Time_t thisStartTime = le_clk_GetRelativeTime();
        le_timer_Start(newTimer);
        TimerTestDataArray[i].offset = le_clk_Sub(thisStartTime, *startTimePtr);
    }

}

#if CONFIG_LINUX
static void* ThreadMain(void* unused)
{
    TimerEventLoopTest();

    le_event_RunLoop();

    return NULL;
}
#endif /* end CONFIG_LINUX */


COMPONENT_INIT
{
    int i;

    Expired = 0;
    Total = 0;
    Passed = 0;

    LE_TEST_INFO("====  Unit test for le_timer module. ====");
    LOG_TIME_MSG("TimerTolerance is configured to", TimerTolerance);

    for (i = 0; i < NUM_TEST_TIMERS; ++i)
    {
        Total += TimerTestDataArray[i].repeatCount;
    }

    MainThread = le_thread_GetCurrent();

#if CONFIG_LINUX
    ChildThread = le_thread_Create("Timer Test", ThreadMain, NULL);
#else
    ChildThread = NULL;
#endif

    LE_TEST_PLAN(2 * Total * TESTS_PER_TIMER + ADDITIONAL_TEST_COUNT);

    Mutex = le_mutex_CreateNonRecursive("mutex");

    LE_ASSERT(pthread_key_create(&StartTimeKey, NULL) == 0);

    // Skip child timer tests on RTOS as timer accuracy isn't good enough
    LE_TEST_BEGIN_SKIP(!CONFIG_LINUX, Total);
#if CONFIG_LINUX
    le_thread_SetJoinable(ChildThread);
    le_thread_Start(ChildThread);
#endif
    LE_TEST_END_SKIP();

    TimerEventLoopTest();
    LE_TEST_INFO("==== Timer Tests Started ====\n");

}
