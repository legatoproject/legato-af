/**
 * @file fa_timer.h
 *
 * Timer module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Specific definitions for linux implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_FA_TIMER_H_INCLUDE_GUARD
#define LEGATO_SRC_FA_TIMER_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Timer Thread Record for Linux
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    timer_ThreadRec_t portableThreadRec; ///< portable timer structure
    int timerFD;                         ///< System timer used by the thread.
}
timer_LinuxThreadRec_t;


#endif /* LEGATO_SRC_FA_TIMER_H_INCLUDE_GUARD */
