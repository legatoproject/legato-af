/**
 * @file timer.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "limit.h"
#include "timer.h"
#include "thread.h"
#include <sys/timerfd.h>

// Include macros for printing out values
#include "le_print.h"


#define DEFAULT_POOL_NAME "Default Timer Pool"
#define DEFAULT_POOL_INITIAL_SIZE 1


//--------------------------------------------------------------------------------------------------
/**
 * Timer object.  Created by le_timer_Create().
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_timer
{
    // Settable attributes
    char name[LIMIT_MAX_TIMER_NAME_BYTES];   ///< The timer name
    le_timer_ExpiryHandler_t handlerRef;     ///< Expiry handler function
    le_clk_Time_t interval;                  ///< Interval
    uint32_t repeatCount;                    ///< Number of times the timer will repeat
    void* contextPtr;                        ///< Context for timer expiry

    // Internal State
    le_dls_Link_t link;                      ///< For adding to the timer list
    bool isActive;                           ///< Is the timer active/running?
    le_clk_Time_t expiryTime;                ///< Time at which the timer should expire
    uint32_t expiryCount;                    ///< Number of times the counter has expired
}
Timer_t;


//--------------------------------------------------------------------------------------------------
/**
 * The default timer memory pool.  Initialized in timer_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TimerMemPoolRef = NULL;


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



// =============================================
//  PRIVATE FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the named timer with default values.
 */
//--------------------------------------------------------------------------------------------------
static void InitTimer
(
    Timer_t* timerPtr,               ///< [IN] The timer to initialize
    const char* nameStr              ///< [IN] Name of the timer.
)
{
    le_clk_Time_t initTime = { 0, 0 };

    if (le_utf8_Copy(timerPtr->name, nameStr, sizeof(timerPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Timer name '%s' truncated to '%s'.", nameStr, timerPtr->name);
    }

    // Initialize default values
    //  - RepeatCount defaults to one-shot timer.
    //  - All other values are invalid
    timerPtr->handlerRef = NULL;
    timerPtr->interval = initTime;
    timerPtr->repeatCount = 1;
    timerPtr->contextPtr = NULL;
    timerPtr->link = LE_DLS_LINK_INIT;
    timerPtr->isActive = false;
    timerPtr->expiryTime = initTime;
    timerPtr->expiryCount = 0;
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
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE if the timer was not in the list
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveFromTimerList
(
    le_dls_List_t* listPtr,             ///< [IN] The list to look at.
    Timer_t* timerPtr                   ///< [IN] The timer to remove
)
{
    if ( ! timerPtr->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    // Remove the timer from the active list
    timerPtr->isActive = false;
    le_dls_Remove(listPtr, &timerPtr->link);

    return LE_OK;
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
 * Arm and (re)start the timerFD
 */
//--------------------------------------------------------------------------------------------------
static void RestartTimerFD
(
    Timer_t* timerPtr      ///< [IN] (Re)start this timer object
)
{
    timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr();
    struct itimerspec timerInterval;

    // Set the timer to expire at the expiry time of the given timer
    // todo: confirm that this will still work if expiryTime is in the past, as there is a small
    //       window that this could happen.
    timerInterval.it_value.tv_sec = timerPtr->expiryTime.sec;
    timerInterval.it_value.tv_nsec = timerPtr->expiryTime.usec * 1000;

    // The timerFD does not repeat
    timerInterval.it_interval.tv_sec = 0;
    timerInterval.it_interval.tv_nsec = 0;

    // Start the actual timerFD
    if ( timerfd_settime(threadRecPtr->timerFD, TFD_TIMER_ABSTIME, &timerInterval, NULL) < 0 )
        perror("ERROR");
    TRACE("timer '%s' started", timerPtr->name);

    // Store the timer for future reference
    threadRecPtr->firstTimerPtr = timerPtr;
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
    timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr();

    // todo: Should we print out the name of the expired timer here?
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
        expiredTimer->handlerRef(expiredTimer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for timerFD expiry
 */
//--------------------------------------------------------------------------------------------------
static void TimerFdHandler
(
    int fd    ///< The timer FD for reading
)
{
    uint64_t expiry;
    ssize_t numBytes;
    timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr();
    Timer_t* firstTimerPtr;

    //TRACE("timer fd=%i expired", fd);

    numBytes = read(fd, &expiry, sizeof(expiry));
    LE_ERROR_IF(numBytes != 8, "On TimerFD read, unexpected numBytes=%zd", numBytes);
    LE_ERROR_IF(expiry != 1,  "On TimerFD read, unexpected expiry=%u", (unsigned int)expiry);

    // Pop off the first timer from the active list, and make sure it is the expected timer.
    // If it is, then process it.  Also need to reset the expected timer, in case processing
    // the current timer will cause the same timer to be started again, and put back at the
    // start of the active list.
    firstTimerPtr = PopFromTimerList(&threadRecPtr->activeTimerList);
    LE_ASSERT( threadRecPtr->firstTimerPtr == firstTimerPtr );
    threadRecPtr->firstTimerPtr = NULL;
    ProcessExpiredTimer(firstTimerPtr);

    // Check if there are any other timers that have since expired, pop them off the
    // list and process them.
    firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
    while ( firstTimerPtr != NULL &&
            le_clk_GreaterThan(le_clk_GetRelativeTime(), firstTimerPtr->expiryTime) )
    {
        // Pop off the timer and process it
        firstTimerPtr = PopFromTimerList(&threadRecPtr->activeTimerList);
        ProcessExpiredTimer(firstTimerPtr);

        // Try the next timer on the list
        firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
    }

    // If the next timer on the active list exists, then if the timerFD is not running, or it is
    // running a timer that is no longer at the beginning of the active list, then (re)start the
    // timerFD.  The timerFD could be running here, if the expiry handler started a new timer,
    // although it might no longer be at the beginning of the list, if we had multiple timers
    // expire, and one of them is a repetitive timer.
    if ( (firstTimerPtr != NULL) &&
         ( threadRecPtr->firstTimerPtr != firstTimerPtr ) )
    {
        RestartTimerFD(firstTimerPtr);
    }
}



// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================


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
    TimerMemPoolRef = le_mem_CreatePool(DEFAULT_POOL_NAME, sizeof(Timer_t));
    le_mem_ExpandPool(TimerMemPoolRef, DEFAULT_POOL_INITIAL_SIZE);

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
    timer_ThreadRec_t* recPtr = thread_GetTimerRecPtr();

    recPtr->timerFD = -1;
    recPtr->activeTimerList = LE_DLS_LIST_INIT;
    recPtr->firstTimerPtr = NULL;
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
    const char* nameStr                 ///< [IN]  Name of the timer.
)
{
    Timer_t* newTimerPtr;

    newTimerPtr = le_mem_ForceAlloc(TimerMemPoolRef);
    InitTimer(newTimerPtr, nameStr);

    return newTimerPtr;
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
    le_timer_Ref_t timerRef                 ///< [IN] Delete this timer object
)
{
    LE_ASSERT(timerRef != NULL);

    // If the timer is running, stop it first.
    if ( timerRef->isActive )
    {
        le_timer_Stop(timerRef);
    }

    le_mem_Release(timerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the timer expiry handler function
 *
 * If the handler is NULL, then the previous handler will be removed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE if the timer is currently running
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
    LE_ASSERT(timerRef != NULL);

    if ( timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    timerRef->handlerRef = handlerRef;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the timer interval
 *
 * The timer will expire after the interval has elapsed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE if the timer is currently running
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
    LE_ASSERT(timerRef != NULL);

    if ( timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    timerRef->interval = interval;

    return LE_OK;
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
 *      - LE_NOT_POSSIBLE if the timer is currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetRepeat
(
    le_timer_Ref_t timerRef,     ///< [IN] Set interval for this timer object
    uint32_t repeatCount         ///< [IN] Number of times the timer will repeat
)
{
    LE_ASSERT(timerRef != NULL);

    if ( timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    timerRef->repeatCount = repeatCount;

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
 *      - LE_NOT_POSSIBLE if the timer is currently running
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_timer_SetContextPtr
(
    le_timer_Ref_t timerRef,     ///< [IN] Set interval for this timer object
    void* contextPtr             ///< [IN] Context Pointer
)
{
    LE_ASSERT(timerRef != NULL);

    if ( timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    timerRef->contextPtr = contextPtr;

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
    le_timer_Ref_t timerRef      ///< [IN] Set interval for this timer object
)
{
    LE_ASSERT(timerRef != NULL);

    return timerRef->contextPtr;
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
    le_timer_Ref_t timerRef      ///< [IN] Set interval for this timer object
)
{
    LE_ASSERT(timerRef != NULL);

    return timerRef->expiryCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the timer
 *
 * Start the given timer. The timer must not be currently running.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE if the timer is already running
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
    LE_ASSERT(timerRef != NULL);

    if ( timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    // Timer is valid and not active; proceed with starting it.

    TRACE("Starting timer '%s'", timerRef->name);

    timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr();
    Timer_t* firstTimerPtr;
    le_dls_Link_t* linkPtr;

    // todo: verify that the minimum number of fields have been appropriately initialized

    // If the current thread does not already have a timerFD, then create a new one.
    // todo: should we do this when the timer is first created, instead of here?
    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", threadRecPtr->timerFD);
    }
    if ( threadRecPtr->timerFD == -1 )
    {
        le_event_FdMonitorRef_t fdMonitorRef;

        // todo: add error checking
        threadRecPtr->timerFD = timerfd_create(CLOCK_MONOTONIC, 0);
        LE_PRINT_VALUE("%i", threadRecPtr->timerFD);
        threadRecPtr->firstTimerPtr = NULL;

        // Register the timerFD with the event loop.
        // It will not be triggered until the timer is actually started
        fdMonitorRef = le_event_CreateFdMonitor("Timer", threadRecPtr->timerFD);
        le_event_SetFdHandler(fdMonitorRef, LE_EVENT_FD_READABLE, TimerFdHandler);
    }

    // Add the timer to the timer list. This is the only place we reset the expiry count.
    timerRef->expiryCount = 0;
    timerRef->expiryTime = le_clk_Add(le_clk_GetRelativeTime(), timerRef->interval);
    AddToTimerList(&threadRecPtr->activeTimerList, timerRef);
    //PrintTimerList(&threadRecPtr->activeTimerList);

    // Get the first timer from the active list. This is needed to determine whether the timerFD
    // needs to be restarted, in case the new timer was put at the beginning of the list.
    // todo: is there a better way to do this?
    linkPtr = le_dls_Peek(&threadRecPtr->activeTimerList);
    firstTimerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

    // If the timerFD is not running, or it is running a timer that is no longer at the beginning
    // of the active list, then (re)start the timerFD.
    if ( threadRecPtr->firstTimerPtr != firstTimerPtr )
    {
        RestartTimerFD(firstTimerPtr);
    }

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
 *      - LE_NOT_POSSIBLE if the timer is not currently running
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
    LE_ASSERT(timerRef != NULL);

    if ( ! timerRef->isActive )
    {
        return LE_NOT_POSSIBLE;
    }

    // Timer is valid and active; proceed with stopping it.
    le_result_t result;
    Timer_t* firstTimerPtr;

    timer_ThreadRec_t* threadRecPtr = thread_GetTimerRecPtr();

    result = RemoveFromTimerList(&threadRecPtr->activeTimerList, timerRef);
    if (result == LE_OK)
    {
        // If the timer was at the start of the active list, then restart the timerFD using the next
        // timer on the active list, if any.  Otherwise, stop the timerFD.
        if (timerRef == threadRecPtr->firstTimerPtr)
        {
            TRACE("Stopping the first active timer");
            threadRecPtr->firstTimerPtr = NULL;

            firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);
            if (firstTimerPtr != NULL)
            {
                RestartTimerFD(firstTimerPtr);
            }
            else
            {
                struct itimerspec timerInterval;

                // Setting the interval to zero will stop the timerFD
                timerInterval.it_value.tv_sec = 0;
                timerInterval.it_value.tv_nsec = 0;
                timerInterval.it_interval.tv_sec = 0;
                timerInterval.it_interval.tv_nsec = 0;

                // Stop the actual timerFD
                if ( timerfd_settime(threadRecPtr->timerFD, TFD_TIMER_ABSTIME, &timerInterval, NULL) < 0 )
                    perror("ERROR");
                TRACE("timerFD=%i stopped", threadRecPtr->timerFD);
            }
        }
    }

    return result;
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
    LE_ASSERT(timerRef != NULL);

    // Ignore the error if the timer is not currently running
    (void)le_timer_Stop(timerRef);

    // We should not receive any error that the timer is currently running
    le_timer_Start(timerRef);
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
    LE_ASSERT(timerRef != NULL);

    return timerRef->isActive;
}


