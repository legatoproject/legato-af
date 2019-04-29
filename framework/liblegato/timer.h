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

#include "fa/timer.h"
#include "limit.h"

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
