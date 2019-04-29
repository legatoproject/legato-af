/**
 * @file timer.h
 *
 * Timer interface that must be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_TIMER_H_INCLUDE_GUARD
#define FA_TIMER_H_INCLUDE_GUARD

#include "legato.h"
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
    TIMER_TYPE_COUNT                        ///< Number of timer types
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
 * Initialize the platform-specific Timer module.
 *
 * This function must be called exactly once at process start-up before any other timer module
 * functions are called.
 *
 * @return POSIX clock type used for timers.
 */
//--------------------------------------------------------------------------------------------------
int fa_timer_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize timer platform-specific resources for a given thread.
 *
 * This function must be called exactly once at thread creation.
 *
 * @return An initialized timer thread record.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t *fa_timer_InitThread
(
    timer_Type_t timerType ///< Type of timer being initialized.
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
    timer_ThreadRec_t *threadRecPtr ///< [IN] Thread timer object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get thread timer record depending on what is specified by the timer and what is supported by the
 * device.
 *
 * @return Timer thread record.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t *fa_timer_GetThreadTimerRec
(
    Timer_t *timerPtr ///< Timer instance.
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_StopTimer
(
    timer_ThreadRec_t *threadRecPtr ///< [IN] Thread timer object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Arm and (re)start the platform-specific timer
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_RestartTimer
(
    timer_ThreadRec_t   *threadRecPtr,      ///< [IN] Thread timer object.
    struct itimerspec   *timerIntervalPtr   ///< [IN] Timer interval to set.
);

//--------------------------------------------------------------------------------------------------
/**
 * Start the timer, platform-specific part
 *
 * Start the given timer. The timer must not be currently running.
 *
 * @attention If an invalid timer object is given, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void fa_timer_Start
(
    Timer_t             *timerPtr,      ///< [IN] Start this timer object.
    timer_ThreadRec_t   *threadRecPtr   ///< [IN] Thread timer object.
);

#endif /* end FA_TIMER_H_INCLUDE_GUARD */
