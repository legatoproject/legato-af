/**
 * @file timer.h
 *
 * Timer module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_TIMER_H_INCLUDE_GUARD
#define LEGATO_SRC_TIMER_H_INCLUDE_GUARD

#include "limit.h"

//--------------------------------------------------------------------------------------------------
/**
 * Timer type codes.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TIMER_NON_WAKEUP = 0,                   ///< Non wakeup timer
    TIMER_WAKEUP,                           ///< Wake up timer
    TIMER_TYPE_COUNT,                       ///< Number of timer types
}
timer_Type_t;


//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_TIMER_NAMES_ENABLED
#   define  TIMER_NAME(var) (var)
#else
#   define  TIMER_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Timer object.  Created by le_timer_Create().
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Settable attributes
#if LE_CONFIG_TIMER_NAMES_ENABLED
    char name[LIMIT_MAX_TIMER_NAME_BYTES];   ///< The timer name
#endif
    le_timer_ExpiryHandler_t handlerRef;     ///< Expiry handler function
    le_clk_Time_t interval;                  ///< Interval
    uint32_t repeatCount;                    ///< Number of times the timer will repeat
    void* contextPtr;                        ///< Context for timer expiry

    // Internal State
    le_dls_Link_t link;                      ///< For adding to the timer list
    bool isActive;                           ///< Is the timer active/running?
    le_clk_Time_t expiryTime;                ///< Time at which the timer should expire
    uint32_t expiryCount;                    ///< Number of times the counter has expired
    le_timer_Ref_t safeRef;                  ///< For the API user to refer to this timer by
    bool isWakeupEnabled;                    ///< Will system be woken up from suspended timer.
                                             ///  Default behaviour will be set to true.
}
Timer_t;


//--------------------------------------------------------------------------------------------------
/**
 * Timer Thread Record.
 *
 * This structure is to be stored as a member in each Thread object.  The timer module uses the
 * function thread_GetTImerRecPtr() to fetch a pointer to one of these records for a given thread.
 *
 * @warning
 *      No code outside of the timer module (timer.c) should ever access members of this structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t activeTimerList;      ///< Linked list of running legato timers for this thread
    Timer_t* firstTimerPtr;             ///< Pointer to the timer on the active list that is
                                        ///  associated with the currently running timerFD,
                                        ///  or NULL if there are no timers on the active list.
                                        ///  This is normally the first timer on the list.
}
timer_ThreadRec_t;


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the timer list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** timer_GetTimerListChgCntRef
(
    void
);


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
);


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
timer_ThreadRec_t*  timer_InitThread
(
    timer_Type_t timerType
);


//--------------------------------------------------------------------------------------------------
/**
 * Accessor for clock type negotiated between clock and timerfd routines.
 *
 * Used by clock functions to ensure clock coherence.
 */
//--------------------------------------------------------------------------------------------------
int timer_GetClockType
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Check all timers on the active list to ensure they have not expired
 *
 * @return true if all active timers are set to expire in the future, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool timer_CheckExpiry
(
    void
);


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
);

//--------------------------------------------------------------------------------------------------
/**
 * Handler for timer expiry
 */
//--------------------------------------------------------------------------------------------------
void timer_Handler
(
    timer_ThreadRec_t* threadRecPtr    ///< [IN] Thread timer object
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Destruct timer platform-specific resources for a given thread.
 *
 * This function must be called exactly once at thread shutdown, and before the Thread object is
 * deleted.
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_DestructThread
(
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
);

//--------------------------------------------------------------------------------------------------
/**
 * Get thread timer record depending on what is specified by the timer and what is supported by the
 * device.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* fa_timer_GetThreadTimerRec
(
    Timer_t* timerPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_StopTimer
(
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
);

//--------------------------------------------------------------------------------------------------
/**
 * Arm and (re)start the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_RestartTimer
(
    timer_ThreadRec_t* threadRecPtr,      ///< [IN] thread timer object
    struct itimerspec* timerIntervalPtr   ///< [IN] timer interval to set
);

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
    Timer_t* timerPtr,               ///< [IN] Start this timer object
    timer_ThreadRec_t* threadRecPtr  ///< [IN] Thread timer object
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the timer for a given thread.
 *
 * This function must be called exactly once at thread start-up, before any other timer module
 * or Timer API functions (other than timer_Init() ) are called by that thread.  The
 * return pointer must be stored in the thread's thread-specific info.
 *
 * @note    The process main thread must call timer_Init() first, then timer_CreatePerThreadInfo().
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* timer_CreateThreadInfo
(
    void
);

#endif /* LEGATO_SRC_TIMER_H_INCLUDE_GUARD */
