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


// Start of the test
static le_clk_Time_t StartTime;


void LongTimerExpiryHandler
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
    diffTime = le_clk_Sub( le_clk_GetRelativeTime(), StartTime);
    if ( le_clk_GreaterThan(le_clk_Sub(diffTime, expectedInterval), TimerTolerance) )
    {
        LE_ERROR("TEST FAILED: Timer expiry does not match");

        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);
    }
    else
    {
        LE_INFO("Long Timer expired in expected interval: TEST PASSED");
    }


    LE_INFO("\n ======================================");
    // Verify that shortTimer (passed in as contextPtr) did not expire

    le_timer_Ref_t shortTimer = le_timer_GetContextPtr(timerRef);
    uint32_t expiryCount = le_timer_GetExpiryCount(shortTimer);

    if ( expiryCount == 0 )
    {
        LE_INFO("Short timer did not expire: TEST PASSED");
    }
    else
    {
        LE_INFO("Short timer expired %i times: TEST FAILED", expiryCount);
    }

    // All tests are now done, so exit
    LE_INFO("ALL TESTS COMPLETE");
    exit(0);

}

void AdditionalTests
(
    le_timer_Ref_t oldTimer
)
{
    le_result_t result;
    le_timer_Ref_t shortTimer;
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

    LE_INFO("Started creating new timer");
    shortTimer = le_timer_Create("short timer from default");
    le_timer_SetInterval( shortTimer, le_clk_Multiply(oneSecInterval, 3) );
    LE_INFO("Finished creating new timer; verify that default pool was not expanded");

    longTimer = le_timer_Create("long timer from default");
    le_timer_SetInterval( longTimer, le_clk_Multiply(oneSecInterval, 5) );
    le_timer_SetHandler(longTimer, LongTimerExpiryHandler);
    le_timer_SetContextPtr(longTimer, shortTimer);

    StartTime = le_clk_GetRelativeTime();
    le_timer_Start(shortTimer);
    le_timer_Start(longTimer);

    // Sleep 1 second for testing purpose only, to verify that restarting the timer will cause
    // it to expire one second later.
    sleep(1);
    le_timer_Stop(shortTimer);
    le_timer_Restart(longTimer);

}


void TimerExpiryHandler
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
    testCount++;

    LE_INFO("\n ======================================");
    LE_INFO("Timer expired");
    LE_INFO("Expiry Count = %u", le_timer_GetExpiryCount(timerRef));

    // Note that diffTime will always be greater than the timer interval, since StartTime is
    // captured before any timers are started.  This means that the second le_clk_Sub() will
    // always return a positive time.
    diffTime = le_clk_Sub( le_clk_GetRelativeTime(), StartTime);
    expectedInterval = le_clk_Multiply(testDataPtr->interval, le_timer_GetExpiryCount(timerRef));
    if ( le_clk_GreaterThan(le_clk_Sub(diffTime, expectedInterval), TimerTolerance) )
    {
        LE_ERROR("TEST FAILED: Timer expiry does not match");

        LE_PRINT_VALUE("%li", testDataPtr->interval.sec);
        LE_PRINT_VALUE("%li", testDataPtr->interval.usec);
        LE_PRINT_VALUE("%li", expectedInterval.sec);
        LE_PRINT_VALUE("%li", expectedInterval.usec);
        LE_PRINT_VALUE("%li", diffTime.sec);
        LE_PRINT_VALUE("%li", diffTime.usec);
    }
    else
    {
        LE_INFO("TEST PASSED");
        testPassed++;
    }

    // If the last timer expired the required number of times, then the expiry tests are finished
    if ( (testDataPtr == &TimerTestDataArray[NUM_TEST_TIMERS-1]) &&
         (le_timer_GetExpiryCount(timerRef) == testDataPtr->repeatCount) )
    {
        LE_INFO("EXPIRY TEST COMPLETE: %i of %i tests passed", testPassed, testCount);

        // Continue with additional tests
        AdditionalTests(timerRef);
    }

}


void timerEventLoopTest(void)
{
    le_timer_Ref_t newTimer;
    int i;

    StartTime = le_clk_GetRelativeTime();

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


COMPONENT_INIT
{
    LE_INFO("\n");
    LE_INFO("====  Unit test for le_timer module. ====");

    timerEventLoopTest();

    LE_INFO("==== Timer Tests Started ====\n");
}

