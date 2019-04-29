/**
 * @file imer.c
 *
 * Implementation of the @ref c_timer framework adaptor for Linux.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "fileDescriptor.h"
#include "thread.h"
#include "timer.h"

// Include macros for printing out values
#include "le_print.h"

#include <sys/timerfd.h>

//--------------------------------------------------------------------------------------------------
/**
 * Clocks to be used by timerfd and clock routines.
 * Defaults to CLOCK_MONOTONIC. Initialized in timer_Init().
 */
//--------------------------------------------------------------------------------------------------
static int TimerClockType = CLOCK_MONOTONIC;


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether the target supports suspended system wake up using timers.
 */
//--------------------------------------------------------------------------------------------------
static bool IsWakeupSupported = false;


//--------------------------------------------------------------------------------------------------
/**
 * Static timer thread-local data pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LinuxThreadRec,
                          LE_CONFIG_MAX_TIMER_POOL_SIZE,
                          sizeof(timer_LinuxThreadRec_t));

//--------------------------------------------------------------------------------------------------
/**
 * Timer thread-local data pool reference.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t LinuxThreadRecPoolRef;


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

    timer_Handler(threadRecPtr);
}

// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific Timer module.
 *
 * This function must be called exactly once at process start-up before any other timer module
 * functions are called.
 *
 * @return
 *      the clock type
 */
//--------------------------------------------------------------------------------------------------
int fa_timer_Init
(
    void
)
{
    struct timespec tS;
    int timerFd;
    int clockClockType = CLOCK_MONOTONIC;

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
            clockClockType = CLOCK_BOOTTIME;
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
                TimerClockType = clockClockType = CLOCK_BOOTTIME;
            }
            // Else fall through to use default CLOCK_MONOTONIC.
        }
    }

    if (CLOCK_MONOTONIC == clockClockType)
    {
        // Nice try, warn that we're using CLOCK_MONOTONIC for both.
        LE_WARN("Using CLOCK_MONOTONIC: no alarm wakeups, timer stops in low power mode.");
    }

    // Initialize memory pool for thread records
    LinuxThreadRecPoolRef = le_mem_InitStaticPool(LinuxThreadRec,
                                                  LE_CONFIG_MAX_TIMER_POOL_SIZE,
                                                  sizeof(timer_LinuxThreadRec_t));


    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("timers");

    return clockClockType;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific parts of the timer module.
 *
 * This function must be called once by each thread when it starts, before any other timer module
 * functions are called by that thread.
 *
 * @return
 *     pointer on an initialized timer reference
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* fa_timer_InitThread
(
    timer_Type_t timerType
)
{
    timer_LinuxThreadRec_t* localThreadRecPtr =  le_mem_ForceAlloc(LinuxThreadRecPoolRef);

    localThreadRecPtr->timerFD = -1;

    return &localThreadRecPtr->portableThreadRec;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destruct timer thread-specific resources for a given thread.
 *
 * This function must be called exactly once at thread shutdown, and before the Thread object is
 * deleted.
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_DestructThread
(
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
)
{
    timer_LinuxThreadRec_t* localThreadRecPtr =
        CONTAINER_OF(threadRecPtr,
                     timer_LinuxThreadRec_t,
                     portableThreadRec);

    // Close the file descriptor
    if (localThreadRecPtr->timerFD != -1)
    {
        fd_Close(localThreadRecPtr->timerFD);
    }
    le_mem_Release(threadRecPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get thread timer record depending on what is specified by the timer and what is supported by the
 * device.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* fa_timer_GetThreadTimerRec
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
 * Stop the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_StopTimer
(
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
)
{
    timer_LinuxThreadRec_t* localThreadRecPtr =
        CONTAINER_OF(threadRecPtr,
                     timer_LinuxThreadRec_t,
                     portableThreadRec);
    struct itimerspec timerInterval;

    // Setting all values to zero will stop the timerFD
    timerInterval.it_value.tv_sec = 0;
    timerInterval.it_value.tv_nsec = 0;
    timerInterval.it_interval.tv_sec = 0;
    timerInterval.it_interval.tv_nsec = 0;

    // Stop the actual timerFD
    if ( timerfd_settime(localThreadRecPtr->timerFD, TFD_TIMER_ABSTIME, &timerInterval, NULL) < 0 )
    {
        // Since we were able to create the timerFD, this should always work.
        LE_FATAL("timerfd_settime() failed with errno = %d (%m)", errno);
    }

    TRACE("timerFD=%i stopped", localThreadRecPtr->timerFD);
}


//--------------------------------------------------------------------------------------------------
/**
 * Arm and (re)start the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_RestartTimer
(
    timer_ThreadRec_t* threadRecPtr,      ///< [IN] thread timer object
    struct itimerspec* timerIntervalPtr   ///< [IN] timer interval to set
)
{
    timer_LinuxThreadRec_t* localThreadRecPtr =
        CONTAINER_OF(threadRecPtr,
                     timer_LinuxThreadRec_t,
                     portableThreadRec);

    // Start the actual timerFD
    if (timerfd_settime(localThreadRecPtr->timerFD, TFD_TIMER_ABSTIME, timerIntervalPtr, NULL) < 0 )
    {
        // Since we were able to create the timerFD, this should always work.
        LE_FATAL("timerfd_settime() failed with errno = %d (%m)", errno);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the timer, platform-specific part
 *
 * Start the given timer. The timer must not be currently running.
 *
 * @note
 *      If an invalid timer object is given, the process exits
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_Start
(
    Timer_t* timerRef,               ///< [IN] Start this timer object
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
)
{
    timer_LinuxThreadRec_t* localThreadRecPtr =
        CONTAINER_OF(threadRecPtr,
                     timer_LinuxThreadRec_t,
                     portableThreadRec);

    // If the current thread does not already have a timerFD, then create a new one.
    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", localThreadRecPtr->timerFD);
    }
    if ( localThreadRecPtr->timerFD == -1 )
    {
        // We want a non-blocking FD (TFD_NONBLOCK), because sometimes the expiry handler is called
        // even though there is nothing to read from the FD, e.g. race condition where timer is
        // stopped after it expired but before the handler was called.
        // We also want the FD to close on exec (TFD_CLOEXEC) so that the FD is not inherited by
        // any child processes.
        if (timerRef->isWakeupEnabled && IsWakeupSupported)
        {
            localThreadRecPtr->timerFD = timerfd_create(TimerClockType, TFD_NONBLOCK | TFD_CLOEXEC);
        }
        else
        {
            // If wakeup enabled is set to off, we should not wake up the system if our timer
            // expires.
            localThreadRecPtr->timerFD =
                timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        }

        if (0 > localThreadRecPtr->timerFD)
        {
            // Should have succeeded if checks in timer_Init() passed.
            LE_FATAL("timerfd_create() failed with errno = %d (%m)", errno);
        }

        LE_PRINT_VALUE("%i", localThreadRecPtr->timerFD);
        threadRecPtr->firstTimerPtr = NULL;

        // Register the timerFD with the event loop.
        // It will not be triggered until the timer is actually started
        le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("Timer",
                                                           localThreadRecPtr->timerFD,
                                                           TimerFdHandler,
                                                           POLLIN);
        le_fdMonitor_SetContextPtr(fdMonitor, threadRecPtr);
    }
}
