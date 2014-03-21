/**
 * @file timer.h
 *
 * Timer module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SRC_TIMER_H_INCLUDE_GUARD
#define LEGATO_SRC_TIMER_H_INCLUDE_GUARD


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
    int timerFD;                       ///< System timer used by the thread.
    le_dls_List_t activeTimerList;     ///< Linked list of running legato timers for this thread
    le_timer_Ref_t firstTimerPtr;      ///< Pointer to the first timer on the active list
                                       ///< or NULL if there are no timers on the active list
}
timer_ThreadRec_t;


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


#endif /* LEGATO_SRC_TIMER_H_INCLUDE_GUARD */
