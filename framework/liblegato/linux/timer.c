/**
 * @file timer.c
 *
 * Implementation of the @ref c_timer.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "clock.h"
#include "timer.h"
#include "thread.h"
#include "fileDescriptor.h"
#include <sys/timerfd.h>
#include "fileDescriptor.h"

// Include macros for printing out values
#include "le_print.h"


#define DEFAULT_POOL_NAME "Default Timer Pool"
#define DEFAULT_POOL_INITIAL_SIZE 1
#define DEFAULT_REFMAP_NAME "Default Timer SafeRefs"
#define DEFAULT_REFMAP_MAXSIZE 23


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the timer list.
 */
//--------------------------------------------------------------------------------------------------
static size_t TimerListChangeCount = 0;
static size_t* TimerListChangeCountRef = &TimerListChangeCount;


//--------------------------------------------------------------------------------------------------
/**
 * The default timer memory pool.  Initialized in timer_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TimerMemPoolRef = NULL;
static  le_ref_MapRef_t SafeRefMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Clocks to be used by timerfd and clock routines.
 * Defaults to CLOCK_MONOTONIC. Initialized in timer_Init().
 */
//--------------------------------------------------------------------------------------------------
static int TimerClockType = CLOCK_MONOTONIC;
static int ClockClockType = CLOCK_MONOTONIC;


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether the target supports suspended system wake up using timers.
 */
//--------------------------------------------------------------------------------------------------
static bool IsWakeupSupported = false;


//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 **/
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)

/// Macro used to query current trace state in this module
#define IS_TRACE_ENABLED LE_IS_TRACE_ENABLED(TraceRef)

/// Workaround to CLOCK_BOOTTIME & CLOCK_BOOTTIME_ALARM not being defined on older
/// versions of the glibc.
/// Values are extracted from <linux/time.h> and are provided by <time.h> in more recent glibc.
#ifndef CLOCK_BOOTTIME
# define CLOCK_BOOTTIME         7
#endif
#ifndef CLOCK_BOOTTIME_ALARM
# define CLOCK_BOOTTIME_ALARM   9
#endif


// =============================================
//  PRIVATE FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Allocate and initialize the named timer with default values.
 *
 * @return
 *      A pointer to the timer object.
 */
//--------------------------------------------------------------------------------------------------
static Timer_t* CreateTimer
(
    const char* nameStr              ///< [IN] Name of the timer.
)
{
    Timer_t* timerPtr;

    timerPtr = le_mem_ForceAlloc(TimerMemPoolRef);

    if (le_utf8_Copy(timerPtr->name, nameStr, sizeof(timerPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Timer name '%s' truncated to '%s'.", nameStr, timerPtr->name);
    }

    // Initialize default values
    //  - RepeatCount defaults to one-shot timer.
    //  - All other values are invalid
    timerPtr->handlerRef = NULL;
    timerPtr->interval = (le_clk_Time_t){0, 0};
    timerPtr->repeatCount = 1;
    timerPtr->contextPtr = NULL;
    timerPtr->link = LE_DLS_LINK_INIT;
    timerPtr->isActive = false;
    timerPtr->expiryTime = (le_clk_Time_t){0, 0};
    timerPtr->expiryCount = 0;
    timerPtr->safeRef = NULL;
    timerPtr->safeRef = le_ref_CreateRef(SafeRefMap, timerPtr);
    timerPtr->isWakeupEnabled = true;

    return timerPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the timer record to the given list, sorted according to the timer value
 */
//--------------------------------------------------------------------------------------------------
static void AddToTimerList
(
    le_dls_List_t* listPtr,               ///< [IN] The list to add to.
    Timer_t* newTimerPtr                  ///< [IN] The timer to add
)
{
    Timer_t* timerPtr;
    le_dls_Link_t* linkPtr;

    if ( newTimerPtr->isActive )
    {
        LE_ERROR("Timer '%s' is already active", newTimerPtr->name);
        return;
    }

    // Get the start of the list
    linkPtr = le_dls_Peek(listPtr);

    // Find the timer whose expiry time is greater than the new timer.
    while ( linkPtr != NULL )
    {
        timerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

        if ( le_clk_GreaterThan(timerPtr->expiryTime, newTimerPtr->expiryTime) )
            break;

        linkPtr = le_dls_PeekNext(listPtr, linkPtr);
    }

    TimerListChangeCount++;
    if (linkPtr == NULL)
    {
        // The list is either empty, or the new timer has the largest expiry time.
        // In either case, add the new timer to the end of the list.
        le_dls_Queue(listPtr, &newTimerPtr->link);
    }
    else
    {
        // Found a timer with larger expiry time; insert the new timer before it.
        le_dls_AddBefore(listPtr, linkPtr, &newTimerPtr->link);
    }

    // The new timer is now on the active list
    newTimerPtr->isActive = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Peek at the first timer from the given timer list
 *
 * @return:
 *      - pointer to the first timer on the list
 *      - NULL if the list is empty
 */
//--------------------------------------------------------------------------------------------------
static Timer_t* PeekFromTimerList
(
    le_dls_List_t* listPtr              ///< [IN] The list to look at.
)
{
    le_dls_Link_t* linkPtr;

    linkPtr = le_dls_Peek(listPtr);
    if (linkPtr != NULL)
    {
        return ( CONTAINER_OF(linkPtr, Timer_t, link) );
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Pop the first timer from the given timer list
 *
 * @return:
 *      - pointer to the first timer on the list
 *      - NULL if the list is empty
 */
//--------------------------------------------------------------------------------------------------
static Timer_t* PopFromTimerList
(
    le_dls_List_t* listPtr              ///< [IN] The list to look at.
)
{
    le_dls_Link_t* linkPtr;
    Timer_t* timerPtr;

    linkPtr = le_dls_Pop(listPtr);
    if (linkPtr != NULL)
    {
        TimerListChangeCount++;
        timerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

        // The timer is no longer on the active list
        timerPtr->isActive = false;

        return timerPtr;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the timer from the given timer list
 */
//--------------------------------------------------------------------------------------------------
static void RemoveFromTimerList
(
    le_dls_List_t* listPtr,             ///< [IN] The list to look at.
    Timer_t* timerPtr                   ///< [IN] The timer to remove
)
{
    // Remove the timer from the active list
    timerPtr->isActive = false;
    TimerListChangeCount++;
    le_dls_Remove(listPtr, &timerPtr->link);
}


#if 0
//--------------------------------------------------------------------------------------------------
/**
 * Print the timer list (for testing/logging)
 *
 * todo: This should be integrated with tracing.  Perhaps there should be two levels of tracing.
 *       One that gives general trace info, and then a second detailed trace that will do things
 *       like printing out the timer list.
 */
//--------------------------------------------------------------------------------------------------
static void PrintTimerList
(
    le_dls_List_t* listPtr               ///< [IN] The list to print.
)
{
    Timer_t* timerPtr;
    le_dls_Link_t* linkPtr;

    // Get the start of the list
    linkPtr = le_dls_Peek(listPtr);
    LE_DEBUG("Timer List:");

    // Print out all the timers on the list
    while ( linkPtr != NULL )
    {
        timerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

        LE_PRINT_VALUE("%p", linkPtr)
        LE_PRINT_VALUE("%p", timerPtr->handlerRef);
        LE_PRINT_VALUE("%li", timerPtr->interval.sec);
        LE_PRINT_VALUE("%li", timerPtr->interval.usec);
        LE_PRINT_VALUE("%p", timerPtr->repeatCount);
        LE_PRINT_VALUE("%p", timerPtr->link);
        LE_PRINT_VALUE("%li", timerPtr->expiryTime.sec);
        LE_PRINT_VALUE("%li", timerPtr->expiryTime.usec);

        linkPtr = le_dls_PeekNext(listPtr, linkPtr);
    }
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Get thread timer record depending on what is specified by the timer and what is supported by the
 * device.
 */
//--------------------------------------------------------------------------------------------------
static timer_ThreadRec_t* GetThreadTimerRec
(
    Timer_t* timerPtr
)
{
    if (!timerPtr->isWakeupEnabled || !IsWakeupSupported)
    {
        /* Return non-wake up timer record if specified by user or wake up is not supported by
         * the device. */
        return thread_GetTimerRecPtr(TIMER_NON_WAKEUP);
    }
    else
    {
        return thread_GetTimerRecPtr(TIMER_WAKEUP);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Arm and (re)start the timerFD
 */
//--------------------------------------------------------------------------------------------------
static void RestartTimerFD
(
    Timer_t* timerPtr      ///< [IN] (Re)start this timer object
)
{
    timer_ThreadRec_t* threadRecPtr = GetThreadTimerRec(timerPtr);

    struct itimerspec timerInterval;

    // Set the timer to expire at the expiry time of the given timer
    // There is a small possibility that the time set now will be slightly in the past
    // at this point but it will just cause the timerfd to expire immediately.
    timerInterval.it_value.tv_sec = timerPtr->expiryTime.sec;
    timerInterval.it_value.tv_nsec = timerPtr->expiryTime.usec * 1000;

    // The timerFD does not repeat
    timerInterval.it_interval.tv_sec = 0;
    timerInterval.it_interval.tv_nsec = 0;

    // Start the actual timerFD
    if ( timerfd_settime(threadRecPtr->timerFD, TFD_TIMER_ABSTIME, &timerInterval, NULL) < 0 )
    {
        // Since we were able to create the timerFD, this should always work.
        LE_FATAL("timerfd_settime() failed with errno = %d (%m)", errno);
    }

    TRACE("timer '%s' started", timerPtr->name);

    // Store the timer for future reference
    threadRecPtr->firstTimerPtr = timerPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop the timerFD
 */
//--------------------------------------------------------------------------------------------------
static void StopTimerFD
(
    timer_ThreadRec_t* threadRecPtr
)
{
    struct itimerspec timerInterval;

    // Setting all values to zero will stop the timerFD
    timerInterval.it_value.tv_sec = 0;
    timerInterval.it_value.tv_nsec = 0;
    timerInterval.it_interval.tv_sec = 0;
    timerInterval.it_interval.tv_nsec = 0;

    // Stop the actual timerFD
    if ( timerfd_settime(threadRecPtr->timerFD, TFD_TIMER_ABSTIME, &timerInterval, NULL) < 0 )
    {
        // Since we were able to create the timerFD, this should always work.
        LE_FATAL("timerfd_settime() failed with errno = %d (%m)", errno);
    }

    TRACE("timerFD=%i stopped", threadRecPtr->timerFD);

    // There is no active timer
    threadRecPtr->firstTimerPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Run a given timer, by adding it to the Timer List and restarting the Timer FD, if necessary.
 *
 * @warning The timer must not be currently on the Timer List.
 */
//--------------------------------------------------------------------------------------------------
static void RunTimer
(
    Timer_t* timerPtr   ///< Timer that has its expiryTime member set, but is not on the timer list.
)
{
    TRACE("Starting timer '%s'", timerPtr->name);

    timer_ThreadRec_t* threadRecPtr = GetThreadTimerRec(timerPtr);

    Timer_t* firstTimerPtr;

    AddToTimerList(&threadRecPtr->activeTimerList, timerPtr);
    //PrintTimerList(&threadRecPtr->activeTimerList);

    // Get the first timer from the active list. This is needed to determine whether the timerFD
    // needs to be restarted, in case the new timer was put at the beginning of the list.
    firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);

    // If the timerFD is not running, or it is running a timer that is no longer at the beginning
    // of the active list, then (re)start the timerFD.
    if ( (NULL != firstTimerPtr) && (threadRecPtr->firstTimerPtr != firstTimerPtr) )
    {
        RestartTimerFD(firstTimerPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop a given timer.
 *
 * @warning The timer must be running.
 */
//--------------------------------------------------------------------------------------------------
static void StopTimer
(
    Timer_t* timerPtr
)
{
    timer_ThreadRec_t* threadRecPtr = GetThreadTimerRec(timerPtr);

    RemoveFromTimerList(&threadRecPtr->activeTimerList, timerPtr);

    // If the timer was at the start of the active list, then restart the timerFD using the next
    // timer on the active list, if any.  Otherwise, stop the timerFD.
    if (timerPtr == threadRecPtr->firstTimerPtr)
    {
        TRACE("Stopping the first active timer");
        threadRecPtr->firstTimerPtr = NULL;

        Timer_t* firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
        if (firstTimerPtr != NULL)
        {
            RestartTimerFD(firstTimerPtr);
        }
        else
        {
            StopTimerFD(threadRecPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a single expired timer
 */
//--------------------------------------------------------------------------------------------------
static void ProcessExpiredTimer
(
    Timer_t* expiredTimer
)
{
    timer_ThreadRec_t* threadRecPtr = GetThreadTimerRec(expiredTimer);

    TRACE("Timer '%s' expired", expiredTimer->name);

    // Keep track of the number of times the timer has expired, regardless of whether it repeats.
    expiredTimer->expiryCount++;

    // Handle repeating timers by adding it back to the list; do this before calling the expiry
    // handler to reduce jitter.
    if ( expiredTimer->repeatCount != 1 )
    {
        // Decrement count, if it is not set to repeat indefinitely.
        if ( expiredTimer->repeatCount != 0 )
        {
            expiredTimer->repeatCount--;
        }

        // Increment the expiry time, by adding to the original expiry time, in order to reduce the
        // timer jitter.  If the current relative time is used, the jitter will increase each time
        // the timer is restarted.
        expiredTimer->expiryTime = le_clk_Add(expiredTimer->expiryTime, expiredTimer->interval);

        // Add the timer back to the timer list
        AddToTimerList(&threadRecPtr->activeTimerList, expiredTimer);
        //PrintTimerList(&threadRecPtr->activeTimerList);
    }

    // call the optional expiry handler function
    if ( expiredTimer->handlerRef != NULL )
    {
        expiredTimer->handlerRef(expiredTimer->safeRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for timerFD expiry
 */
//--------------------------------------------------------------------------------------------------
static void TimerFdHandler
(
    int fd,         ///< The timer FD for reading
    short events    ///< The event bit map.
)
{
    uint64_t expiry;
    ssize_t numBytes;
    timer_ThreadRec_t* threadRecPtr = le_fdMonitor_GetContextPtr();
    Timer_t* firstTimerPtr;

    LE_ASSERT((events & ~POLLIN) == 0);

    //TRACE("timer fd=%i expired", fd);

    // Read the timerFD to clear the timer expiry; we don't actually do anything with the value.
    // If there is nothing to read, then we had a stale timer, which can happen sometimes, e.g
    // the timer expires, TimerFdHandler() is queued onto the event loop, and then the timer is
    // stopped before TimerFdHandler() is called.
    numBytes = read(fd, &expiry, sizeof(expiry));
    if ( numBytes == -1 )
    {
        if ( errno == EAGAIN )
        {
            TRACE("Stale timer expired");
            return;
        }
        else
        {
            LE_FATAL("TimerFD read failed with errno = %d (%m)", errno);
        }
    }
    LE_ERROR_IF(numBytes != 8, "On TimerFD read, unexpected numBytes=%zd", numBytes);
    LE_ERROR_IF(expiry != 1,  "On TimerFD read, unexpected expiry=%u", (unsigned int)expiry);

    // Pop off the first timer from the active list, and make sure it is the expected timer.
    firstTimerPtr = PopFromTimerList(&threadRecPtr->activeTimerList);
    LE_ASSERT( NULL != firstTimerPtr);

    LE_ASSERT( threadRecPtr->firstTimerPtr == firstTimerPtr );

    // Need to reset the expected timer, in case processing the current timer will cause the same
    // timer to be started again, and put back at the start of the active list. This is necessary
    // since the timerFD is no longer running, so there is no timer associated with it.
    threadRecPtr->firstTimerPtr = NULL;

    // It is the expected timer so process it.
    ProcessExpiredTimer(firstTimerPtr);

    // Check if there are any other timers that have since expired, pop them off the
    // list and process them.
    firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
    while ( firstTimerPtr != NULL &&
            le_clk_GreaterThan(clk_GetRelativeTime(firstTimerPtr->isWakeupEnabled),
                               firstTimerPtr->expiryTime) )
    {
        // Pop off the timer and process it
        firstTimerPtr = PopFromTimerList(&threadRecPtr->activeTimerList);
        ProcessExpiredTimer(firstTimerPtr);

        // Try the next timer on the list
        firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
    }

    // While processing expired timers in the above loop, it is possible that a timer was started,
    // put in the active list, and expired before the loop completed. If the active list is empty,
    // but the timerFD is still running, then we need to stop it.
    if ( (firstTimerPtr == NULL) && ( threadRecPtr->firstTimerPtr != NULL ) )
    {
        StopTimerFD(threadRecPtr);
    }

    // If the next timer on the active list exists, then if the timerFD is not running, or it is
    // running a timer that is no longer at the beginning of the active list, then (re)start the
    // timerFD.  The timerFD could be running here, if the expiry handler started a new timer,
    // although it might no longer be at the beginning of the list, if we had multiple timers
    // expire, and one of them is a repetitive timer.
    if ( (firstTimerPtr != NULL) && ( threadRecPtr->firstTimerPtr != firstTimerPtr ) )
    {
        RestartTimerFD(firstTimerPtr);
    }
}

// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the timer list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** timer_GetTimerListChgCntRef
(
    void
)
{
    return (&TimerListChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Timer module.
 *
 * This function must be called exactly once at process start-up before any other timer module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void timer_Init
(
    void
)
{
    struct timespec tS;
    int timerFd;

    TimerMemPoolRef = le_mem_CreatePool(DEFAULT_POOL_NAME, sizeof(Timer_t));
    le_mem_ExpandPool(TimerMemPoolRef, DEFAULT_POOL_INITIAL_SIZE);

    SafeRefMap = le_ref_CreateMap(DEFAULT_REFMAP_NAME, DEFAULT_REFMAP_MAXSIZE);

    // Assume CLOCK_MONOTONIC is supported both by timerfd and clock routines.
    // Then, query O/S to see if we could use CLOCK_BOOTTIME/_ALARM.
    if (!clock_gettime(CLOCK_BOOTTIME, &tS))
    {
        // Supported, see if we could use the _ALARM version for timerfd.
        timerFd = timerfd_create(CLOCK_BOOTTIME_ALARM, 0);
        if (0 <= timerFd)
        {
            // Success, use CLOCK_BOOTTIME_ALARM for timerfd and
            // CLOCK_BOOTTIME for clock routines.
            fd_Close(timerFd);
            TimerClockType = CLOCK_BOOTTIME_ALARM;
            ClockClockType = CLOCK_BOOTTIME;
            IsWakeupSupported = true;
        }
        else
        {
            // Fail, try using CLOCK_BOOTTIME for timerfd.
            timerFd = timerfd_create(CLOCK_BOOTTIME, 0);
            if (0 <= timerFd)
            {
                // Success, use CLOCK_BOOTTIME for both and warn.
                fd_Close(timerFd);
                LE_WARN("Using CLOCK_BOOTTIME: alarm wakeups not supported.");
                TimerClockType = ClockClockType = CLOCK_BOOTTIME;
            }
            // Else fall through to use default CLOCK_MONOTONIC.
        }
    }

    if (CLOCK_MONOTONIC == ClockClockType)
        // Nice try, warn that we're using CLOCK_MONOTONIC for both.
        LE_WARN("Using CLOCK_MONOTONIC: no alarm wakeups, timer stops in low power mode.");

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("timers");
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific parts of the timer module.
 *
 * This function must be called once by each thread when it starts, before any other timer module
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void timer_InitThread
(
    void
)
{
    timer_Type_t i;

    for (i = TIMER_NON_WAKEUP; i < TIMER_TYPE_COUNT; i++)
    {
        timer_ThreadRec_t* recPtr = thread_GetTimerRecPtr(i);

        recPtr->timerFD = -1;
        recPtr->activeTimerList = LE_DLS_LIST_INIT;
        recPtr->firstTimerPtr = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Accessor for clock type negotiated between clock and timerfd routines.
 *
 * Used by clock functions to ensure clock coherence.
 */
//--------------------------------------------------------------------------------------------------
int timer_GetClockType(void)
{
    return ClockClockType;
}

//--------------------------------------------------------------------------------------------------
/**
 * Destruct timer resources for a given thread.
 *
 * This function must be called exactly once at thread shutdown, and before the Thread object is
 * deleted.
 */
//--------------------------------------------------------------------------------------------------
void timer_DestructThread
(
    void
)
{
    timer_Type_t i;

    for (i = TIMER_NON_WAKEUP; i < TIMER_TYPE_COUNT; i++)
    {
        timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr(i);

        // Close the file descriptor
        if (threadRecPtr->timerFD != -1)
        {
            fd_Close(threadRecPtr->timerFD);
        }

        le_dls_Link_t* linkPtr;

        // Get the start of the list
        linkPtr = le_dls_Peek(&threadRecPtr->activeTimerList);

        // Release the timer list
        while ( linkPtr != NULL )
        {
            Timer_t* timerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

            linkPtr = le_dls_PeekNext(&threadRecPtr->activeTimerList, linkPtr);

            le_dls_Remove(&threadRecPtr->activeTimerList, &timerPtr->link);

            le_mem_Release(timerPtr);
        }
    }
}


// =============================================
//  PUBLIC API FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Create the timer object.
 *
 * @return
 *      A reference to the timer object.
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t le_timer_Create
(
    const char* nameStr          ///< [IN]  Name of this timer for logging and debug
)
{
    Timer_t* newTimerPtr;

    newTimerPtr = CreateTimer(nameStr);

    return newTimerPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete the timer object
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
void le_timer_Delete
(
    le_timer_Ref_t timerRef      ///< [IN] Delete this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    // If the timer is running, stop it first.
    if ( timerPtr->isActive )
    {
        le_timer_Stop(timerRef);
    }
    le_ref_DeleteRef(SafeRefMap, timerRef);
    le_mem_Release(timerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the timer expiry handler function
 *
 * If the handler is NULL, then the previous handler will be removed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the timer is currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetHandler
(
    le_timer_Ref_t timerRef,                ///< [IN] Set expiry handler for this timer object
    le_timer_ExpiryHandler_t handlerRef     ///< [IN] Handler function to call on expiry
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    timerPtr->handlerRef = handlerRef;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the timer interval
 *
 * Timer will expire after the interval has elapsed since it was last started or restarted.
 *
 * If the timer is running when the interval is changed and the new interval is shorter than the
 * period of time since the timer last (re)started, the timer will expire immediately.
 *
 * @return
 *      - LE_OK on success
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetInterval
(
    le_timer_Ref_t timerRef,     ///< [IN] Set interval for this timer object
    le_clk_Time_t interval       ///< [IN] Timer interval
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( le_clk_Equal(timerPtr->interval, interval))
    {
        return LE_OK;
    }

    if ( timerPtr->isActive )
    {
        // Compute when it should expire with the new interval, as if it was started with this
        // interval.
        le_clk_Time_t expiryTime = le_clk_Add(le_clk_Sub(timerPtr->expiryTime, timerPtr->interval),
                                              interval);

        // Stop it, update its interval and expiry time, and start it running again.
        StopTimer(timerPtr);
        timerPtr->interval = interval;
        timerPtr->expiryTime = expiryTime;
        RunTimer(timerPtr);
    }
    else
    {
        timerPtr->interval = interval;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the timer interval.
 *
 * @return
 *      The timer interval.  If it hasn't been set yet, (le_clk_Time_t){0, 0} will be returned.
 *
 * @note
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_timer_GetInterval
(
    le_timer_Ref_t timerRef      ///< [IN] Timer object.
)
//--------------------------------------------------------------------------------------------------
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return (timerPtr->interval);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the timer interval using milliseconds.
 *
 * Timer will expire after the interval has elapsed since it was last started or restarted.
 *
 * If the timer is running when the interval is changed and the new interval is shorter than the
 * period of time since the timer last (re)started, the timer will expire immediately.
 *
 * @return
 *      - LE_OK on success
 *
 * @note
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetMsInterval
(
    le_timer_Ref_t timerRef,     ///< [IN] Set interval for this timer object.
    uint32_t interval            ///< [IN] Timer interval in milliseconds.
)
{
    time_t seconds = interval / 1000;
    le_clk_Time_t timeStruct;
    timeStruct.sec = seconds;
    timeStruct.usec = (interval - (seconds * 1000)) * 1000;

    return le_timer_SetInterval(timerRef, timeStruct);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the timer interval in milliseconds.
 *
 * @return
 *      The timer interval (ms).  If it hasn't been set yet, 0 will be returned.
 *
 * @note
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_timer_GetMsInterval
(
    le_timer_Ref_t timerRef      ///< [IN] Timer object.
)
//--------------------------------------------------------------------------------------------------
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return ((timerPtr->interval.sec * 1000) + (timerPtr->interval.usec / 1000));
}


//--------------------------------------------------------------------------------------------------
/**
 * Set how many times the timer will repeat
 *
 * The timer will repeat the given number of times.  A value of 0 means repeat indefinitely.
 * The default is 1, so that a one-shot timer is the default.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the timer is currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetRepeat
(
    le_timer_Ref_t timerRef,     ///< [IN] Set repeat count for this timer object
    uint32_t repeatCount         ///< [IN] Number of times the timer will repeat (0 = forever).
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    timerPtr->repeatCount = repeatCount;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Configure if timer expiry will wake up a suspended system.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the timer is currently running
 *
 * @note
 *      The default timer expiry behaviour will wake up the system.
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetWakeup
(
    le_timer_Ref_t timerRef,     ///< [IN] Disable system wake up for this timer object
    bool wakeupEnabled           ///< [IN] Flag to determine timer will wakeup or not
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    timerPtr->isWakeupEnabled = wakeupEnabled;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set context pointer for the timer
 *
 * This can be used to pass data to the timer when it expires
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the timer is currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetContextPtr
(
    le_timer_Ref_t timerRef,     ///< [IN] Set context pointer for this timer object
    void* contextPtr             ///< [IN] Context Pointer
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    timerPtr->contextPtr = contextPtr;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get context pointer for the timer
 *
 * This can be used when the timer expires to retrieve data that was previously set.
 *
 * @return
 *      Context pointer, which could be NULL if it was not set
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
void* le_timer_GetContextPtr
(
    le_timer_Ref_t timerRef      ///< [IN] Get context pointer for this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return timerPtr->contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the expiry count of a timer
 *
 * The count is returned for both currently running and idle timers.  The expiry count
 * is reset every time the timer is (re)started.
 *
 * @return
 *      Expiry count, or zero if the timer has never expired.
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_timer_GetExpiryCount
(
    le_timer_Ref_t timerRef      ///< [IN] Get expiry count for this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return timerPtr->expiryCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the time remaining until the next scheduled expiry.
 *
 * @return
 *      Time remaining (in milliseconds).
 *      (le_clk_Time_t){0, 0} if the timer is stopped or if it has reached its expiry time.
 *
 * @note
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t le_timer_GetTimeRemaining
(
    le_timer_Ref_t timerRef      ///< [IN] Get expiry count for this timer object
)
//--------------------------------------------------------------------------------------------------
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    // If the timer is not running, return 0.
    if (timerPtr->isActive == false)
    {
        return (le_clk_Time_t){0, 0};
    }

    // Compute the time remaining by subtracting the current time from the expiry time.
    le_clk_Time_t timeRemaining = le_clk_Sub(timerPtr->expiryTime,
                                             clk_GetRelativeTime(timerPtr->isWakeupEnabled));

    // If the time remaining is negative, it means this timer has expired and is waiting to
    // have that expiry processed.
    if (timeRemaining.sec < 0)
    {
        return (le_clk_Time_t){0, 0};
    }

    return timeRemaining;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the time remaining (in milliseconds) until the next scheduled expiry.
 *
 * @return
 *      Time remaining (in milliseconds).
 *      0 if the timer is stopped or if it has reached its expiry time.
 *
 * @note
 *      If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_timer_GetMsTimeRemaining
(
    le_timer_Ref_t timerRef      ///< [IN] Get expiry count for this timer object
)
//--------------------------------------------------------------------------------------------------
{
    // Get the time remaining in the form of an le_clk_Time_t.
    le_clk_Time_t timeRemaining = le_timer_GetTimeRemaining(timerRef);

    // Convert to ms and return the result.
    return ((timeRemaining.sec * 1000) + (timeRemaining.usec / 1000));
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the timer
 *
 * Start the given timer. The timer must not be currently running.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY if the timer is already running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_Start
(
    le_timer_Ref_t timerRef      ///< [IN] Start this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    // Timer is valid and not active; proceed with starting it.

    TRACE("Starting timer '%s'", timerPtr->name);

    timer_ThreadRec_t* threadRecPtr = GetThreadTimerRec(timerPtr);

    // If the current thread does not already have a timerFD, then create a new one.
    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", threadRecPtr->timerFD);
    }
    if ( threadRecPtr->timerFD == -1 )
    {
        // We want a non-blocking FD (TFD_NONBLOCK), because sometimes the expiry handler is called
        // even though there is nothing to read from the FD, e.g. race condition where timer is
        // stopped after it expired but before the handler was called.
        // We also want the FD to close on exec (TFD_CLOEXEC) so that the FD is not inherited by
        // any child processes.
        if (timerPtr->isWakeupEnabled && IsWakeupSupported)
        {
            threadRecPtr->timerFD = timerfd_create(TimerClockType, TFD_NONBLOCK | TFD_CLOEXEC);
        }
        else
        {
            // If wakeup enabled is set to off, we should not wake up the system if our timer
            // expires.
            threadRecPtr->timerFD = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        }

        if (0 > threadRecPtr->timerFD)
        {
            // Should have succeeded if checks in timer_Init() passed.
            LE_FATAL("timerfd_create() failed with errno = %d (%m)", errno);
        }

        LE_PRINT_VALUE("%i", threadRecPtr->timerFD);
        threadRecPtr->firstTimerPtr = NULL;

        // Register the timerFD with the event loop.
        // It will not be triggered until the timer is actually started
        le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("Timer", threadRecPtr->timerFD, TimerFdHandler, POLLIN);
        le_fdMonitor_SetContextPtr(fdMonitor, threadRecPtr);
    }

    // Add the timer to the timer list. This is the only place we reset the expiry count.
    timerPtr->expiryCount = 0;
    timerPtr->expiryTime = le_clk_Add(clk_GetRelativeTime(timerPtr->isWakeupEnabled),
                                      timerPtr->interval);
    RunTimer(timerPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop the timer
 *
 * Stop the given timer. The timer must be running.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if the timer is not currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_Stop
(
    le_timer_Ref_t timerRef      ///< [IN] Stop this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( ! timerPtr->isActive )
    {
        return LE_FAULT;
    }

    // Timer is valid and active; proceed with stopping it.
    StopTimer(timerPtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Re-start the timer
 *
 * Start the given timer. If the timer is currently running, it will be stopped and then started.
 * If the timer is not currently running, it will be started.
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
void le_timer_Restart
(
    le_timer_Ref_t timerRef      ///< [IN] (Re)start this timer object
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    // Ignore the error if the timer is not currently running
    (void)le_timer_Stop(timerRef);

    // We should not receive any error that the timer is currently running
    (void)le_timer_Start(timerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Is the timer currently running.
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
bool le_timer_IsRunning
(
    le_timer_Ref_t timerRef      ///< [IN] Is this timer object currently running
)
{
    Timer_t* timerPtr = le_ref_Lookup(SafeRefMap, timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return timerPtr->isActive;
}

