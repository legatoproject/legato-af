/**
 * This program tests some fixes for timer expiry related problems
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "le_print.h"


void banner(char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}


static le_clk_Time_t TimerOneInterval = { 0, 300000 };
static le_clk_Time_t TimerTwoInterval = { 5, 0 };
static le_timer_Ref_t TimerOneRef;
static le_timer_Ref_t TimerTwoRef;


static void FourthQueuedFunction
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Fourth queued function called: test passes !!!");
}


static void ThirdQueuedFunction
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Third queued function called");
    LE_INFO("result=%i", le_timer_Stop(TimerOneRef));
    LE_INFO("timer one stopped");

    // Queue one final function; before the fix, this would never get called.
    le_event_QueueFunction(FourthQueuedFunction, NULL, NULL);
}


static void SecondQueuedFunction
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Second queued function called");
    LE_INFO("result=%i", le_timer_Stop(TimerOneRef));
    LE_INFO("timer one stopped");

    // Repeat the test, with only one running timer, but start the timer from a queued function,
    // and not a timer expiry handler function.
    LE_INFO("Starting timer one");
    le_timer_Start(TimerOneRef);

    /*
     * This is a similar situation to that described in TimerTest(), but before the fix this would
     * block indefinitely because there is no second timer to unblock TimerFdHandler().
     */
    LE_INFO("starting sleep");
    le_event_QueueFunction(ThirdQueuedFunction, NULL, NULL);
    sleep(2);
    LE_INFO("finished sleep");
}


static void QueuedFunction
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Queued function called");
    LE_INFO("result=%i", le_timer_Stop(TimerOneRef));
    LE_INFO("timer one stopped");
}


void TimerOneExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_INFO("timer one expired");
    LE_PRINT_VALUE("%i", le_timer_GetExpiryCount(timerRef));
}


void TimerTwoExpiryHandler
(
    le_timer_Ref_t timerRef    ///< This timer has expired
)
{
    LE_INFO("timer two expired");
    LE_PRINT_VALUE("%i", le_timer_GetExpiryCount(timerRef));

    // Repeat the test, but this time only have one running timer
    LE_INFO("Starting timer one");
    le_timer_Start(TimerOneRef);

    /*
     * This is different than the case below, because this is called from timer.c/TimerFdHandler(),
     * and TimerFdHandler() will process all expired timers. As before, timerOne will be expired
     * by the time this function returns due to the sleep(2) call, but the expiry handler will
     * be called first, before the queued function is called.  Before the fix, the timerFD
     * associated with timerOne would still be running, and so TimerFdHandler() would be called
     * after the timerOne expiry time, and an assert would happen.
     */
    LE_INFO("starting sleep");
    le_event_QueueFunction(SecondQueuedFunction, NULL, NULL);
    sleep(2);
    LE_INFO("finished sleep");
}


void TimerTest(void)
{
    // Perform the test with two running timers

    TimerOneRef = le_timer_Create("timer one");
    le_timer_SetInterval(TimerOneRef, TimerOneInterval);
    le_timer_SetRepeat(TimerOneRef, 1);
    le_timer_SetHandler(TimerOneRef, TimerOneExpiryHandler);
    le_timer_Start(TimerOneRef);

    TimerTwoRef = le_timer_Create("timer two");
    le_timer_SetInterval(TimerTwoRef, TimerTwoInterval);
    le_timer_SetRepeat(TimerTwoRef, 1);
    le_timer_SetHandler(TimerTwoRef, TimerTwoExpiryHandler);
    le_timer_Start(TimerTwoRef);

    LE_PRINT_VALUE("%p", TimerOneRef);
    LE_PRINT_VALUE("%p", TimerTwoRef);

    /*
     * The queued function will get put onto the eventLoop before timerOne has expired, but due
     * to the sleep(2) call, by the time the queued function runs and stops timerOne, it will
     *  already be expired, and the call to timer.c/TimerFdHandler() will have been put on the
     *  eventLoop. Before the fix, this would then block until timerTwo expired.
     */
    LE_INFO("starting sleep");
    le_event_QueueFunction(QueuedFunction, NULL, NULL);
    sleep(2);
    LE_INFO("finished sleep");
}


COMPONENT_INIT
{
    TimerTest();
}


