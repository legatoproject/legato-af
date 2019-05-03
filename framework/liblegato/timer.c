/**
 * @file timer.c
 *
 * Implementation of the @ref c_timer.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "clock.h"
#include "thread.h"
#include "timer.h"

/// Statically allocated timer pool
LE_MEM_DEFINE_STATIC_POOL(TimerPool, LE_CONFIG_MAX_TIMER_POOL_SIZE, sizeof(Timer_t));

/// Statically allocated safe ref map
LE_REF_DEFINE_STATIC_MAP(TimerSafeRefs, LE_CONFIG_MAX_TIMER_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * A mutex to protect safe ref operations
 */
//--------------------------------------------------------------------------------------------------
LE_MUTEX_DECLARE_REF(TimerMutex);

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
static le_ref_MapRef_t SafeRefMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Clocks to be used by timerfd and clock routines.
 * Defaults to CLOCK_MONOTONIC. Initialized in timer_Init().
 */
//--------------------------------------------------------------------------------------------------
static int ClockClockType = CLOCK_MONOTONIC;


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
// Create definitions for inlineable functions
//
// See le_timer.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_TIMER_NAMES_ENABLED
LE_DEFINE_INLINE le_timer_Ref_t le_timer_Create
(
    const char *nameStr
);
#endif

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
#if LE_CONFIG_TIMER_NAMES_ENABLED
    const char* nameStr              ///< [IN] Name of the timer.
#else
    void
#endif
)
{
    Timer_t* timerPtr;

    timerPtr = le_mem_ForceAlloc(TimerMemPoolRef);

#if LE_CONFIG_TIMER_NAMES_ENABLED
    if (le_utf8_Copy(timerPtr->name, nameStr, sizeof(timerPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Timer name '%s' truncated to '%s'.", nameStr, timerPtr->name);
    }
#endif /* end LE_CONFIG_TIMER_NAMES_ENABLED */

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
    Lock();
    timerPtr->safeRef = le_ref_CreateRef(SafeRefMap, timerPtr);
    Unlock();
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
        LE_ERROR("Timer '%s' is already active", TIMER_NAME(newTimerPtr->name));
        return;
    }

    // Get the start of the list
    linkPtr = le_dls_Peek(listPtr);

    // Find the timer whose expiry time is greater than the new timer.
    while ( linkPtr != NULL )
    {
        timerPtr = CONTAINER_OF(linkPtr, Timer_t, link);

        if ( le_clk_GreaterThan(timerPtr->expiryTime, newTimerPtr->expiryTime) )
        {
            break;
        }

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


//--------------------------------------------------------------------------------------------------
/**
 * Arm and (re)start the timer
 */
//--------------------------------------------------------------------------------------------------
static void RestartTimerPhys
(
    Timer_t* timerPtr      ///< [IN] (Re)start this timer object
)
{
    timer_ThreadRec_t* threadRecPtr = fa_timer_GetThreadTimerRec(timerPtr);

    struct itimerspec timerInterval;

    // Set the timer to expire at the expiry time of the given timer
    // There is a small possibility that the time set now will be slightly in the past
    // at this point but it will just cause the timerfd to expire immediately.
    timerInterval.it_value.tv_sec = timerPtr->expiryTime.sec;
    timerInterval.it_value.tv_nsec = timerPtr->expiryTime.usec * 1000;

    // The timer does not repeat
    timerInterval.it_interval.tv_sec = 0;
    timerInterval.it_interval.tv_nsec = 0;

    // Start the actual timer
    fa_timer_RestartTimer(threadRecPtr, &timerInterval);

    LE_DEBUG("timer '%s' started", TIMER_NAME(timerPtr->name));

    // Store the timer for future reference
    threadRecPtr->firstTimerPtr = timerPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop the timer
 */
//--------------------------------------------------------------------------------------------------
static void StopTimerPhys
(
    timer_ThreadRec_t* threadRecPtr
)
{

    fa_timer_StopTimer(threadRecPtr);

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
    TRACE("Starting timer '%s'", TIMER_NAME(timerPtr->name));

    timer_ThreadRec_t* threadRecPtr = fa_timer_GetThreadTimerRec(timerPtr);

    Timer_t* firstTimerPtr;

    AddToTimerList(&threadRecPtr->activeTimerList, timerPtr);

    // Get the first timer from the active list. This is needed to determine whether the timer
    // needs to be restarted, in case the new timer was put at the beginning of the list.
    firstTimerPtr = PeekFromTimerList(&threadRecPtr->activeTimerList);

    // If the timer is not running, or it is running a timer that is no longer at the beginning
    // of the active list, then (re)start the timer.
    if ( (NULL != firstTimerPtr) && (threadRecPtr->firstTimerPtr != firstTimerPtr) )
    {
        RestartTimerPhys(firstTimerPtr);
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
    timer_ThreadRec_t* threadRecPtr = fa_timer_GetThreadTimerRec(timerPtr);

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
            RestartTimerPhys(firstTimerPtr);
        }
        else
        {
            StopTimerPhys(threadRecPtr);
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
    timer_ThreadRec_t* threadRecPtr = fa_timer_GetThreadTimerRec(expiredTimer);

    LE_DEBUG("Timer '%s' expired", TIMER_NAME(expiredTimer->name));

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
 * Get a timer from a safe reference
 *
 * Gets a timer from a safe reference.  Use this instead of le_ref_Lookup() directly as this locks.
 */
//--------------------------------------------------------------------------------------------------
static Timer_t *GetTimer
(
    le_timer_Ref_t timerRef      ///< [IN] Timer reference
)
{
    Timer_t* retVal;
    Lock();
    retVal = le_ref_Lookup(SafeRefMap, timerRef);
    Unlock();
    return retVal;
}

// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Handler for timer expiry
 */
//--------------------------------------------------------------------------------------------------
void timer_Handler
(
    timer_ThreadRec_t* threadRecPtr    ///< [IN] Thread timer object
)
{
    Timer_t* firstTimerPtr;

    // Pop off the first timer from the active list, and make sure it is the expected timer.
    firstTimerPtr = PopFromTimerList(&threadRecPtr->activeTimerList);
    LE_ASSERT( NULL != firstTimerPtr);

    LE_ASSERT( threadRecPtr->firstTimerPtr == firstTimerPtr );

    // Need to reset the expected timer, in case processing the current timer will cause the same
    // timer to be started again, and put back at the start of the active list. This is necessary
    // since the timer is no longer running, so there is no timer associated with it.
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
    // but the timer is still running, then we need to stop it.
    if ( (firstTimerPtr == NULL) && ( threadRecPtr->firstTimerPtr != NULL ) )
    {
        StopTimerPhys(threadRecPtr);
    }

    // If the next timer on the active list exists, then if the timer is not running, or it is
    // running a timer that is no longer at the beginning of the active list, then (re)start the
    // timer.  The timer could be running here, if the expiry handler started a new timer,
    // although it might no longer be at the beginning of the list, if we had multiple timers
    // expire, and one of them is a repetitive timer.
    if ( (firstTimerPtr != NULL) &&
         ( threadRecPtr->firstTimerPtr != firstTimerPtr ) )
    {
        RestartTimerPhys(firstTimerPtr);
    }
}


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
    TimerMemPoolRef = le_mem_InitStaticPool(TimerPool, LE_CONFIG_MAX_TIMER_POOL_SIZE,
        sizeof(Timer_t));

    SafeRefMap = le_ref_InitStaticMap(TimerSafeRefs, LE_CONFIG_MAX_TIMER_POOL_SIZE);

    TimerMutex = le_mutex_CreateNonRecursive("TimerMutex");

    ClockClockType = fa_timer_Init();

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("timers");
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific parts of the timer module.
 *
 * This function must be called once by each thread when it starts, before any other timer module
 * functions are called by that thread.
 *
 * @return
 *     pointer on an initialized timer reference
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* timer_InitThread
(
    timer_Type_t timerType
)
{
    timer_ThreadRec_t* threadRecPtr = fa_timer_InitThread(timerType);

    threadRecPtr->activeTimerList = LE_DLS_LIST_INIT;
    threadRecPtr->firstTimerPtr = NULL;

    return threadRecPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Accessors for clock type negotiated between clock and timerfd routines.
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
        fa_timer_DestructThread(threadRecPtr);
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
#if LE_CONFIG_TIMER_NAMES_ENABLED
le_timer_Ref_t le_timer_Create
(
    const char* nameStr          ///< [IN]  Name of this timer for logging and debug
)
#else
le_timer_Ref_t _le_timer_Create
(
    void
)
#endif
{
    Timer_t* newTimerPtr;

#if LE_CONFIG_TIMER_NAMES_ENABLED
    newTimerPtr = CreateTimer(nameStr);
#else
    newTimerPtr = CreateTimer();
#endif

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
    Timer_t* timerPtr = GetTimer(timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    // If the timer is running, stop it first.
    if ( timerPtr->isActive )
    {
        le_timer_Stop(timerRef);
    }
    Lock();
    le_ref_DeleteRef(SafeRefMap, timerRef);
    le_mem_Release(timerPtr);
    Unlock();
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    if ( timerPtr->isActive )
    {
        return LE_BUSY;
    }

    // Timer is valid and not active; proceed with starting it.

    LE_DEBUG("Starting timer '%s'", TIMER_NAME(timerPtr->name));

    timer_ThreadRec_t* threadRecPtr = fa_timer_GetThreadTimerRec(timerPtr);

    fa_timer_Start(timerPtr, threadRecPtr);

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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
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
    Timer_t* timerPtr = GetTimer(timerRef);
    LE_FATAL_IF(NULL == timerPtr, "Invalid timer reference %p.", timerRef);

    return timerPtr->isActive;
}
