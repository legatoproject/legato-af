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
 * Timer object.  Created by le_timer_Create().
 */
//--------------------------------------------------------------------------------------------------
typedef struct
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
    int timerFD;                        ///< System timer used by the thread.
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
 */
//--------------------------------------------------------------------------------------------------
void timer_InitThread
(
    void
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

#endif /* LEGATO_SRC_TIMER_H_INCLUDE_GUARD */
