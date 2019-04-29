/**
 * @file timer.h
 *
 * Timer module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Specific definitions for linux implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LIBLEGATO_LINUX_TIMER_H_INCLUDE_GUARD
#define LEGATO_LIBLEGATO_LINUX_TIMER_H_INCLUDE_GUARD

#include "fa/timer.h"

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


#endif /* end LEGATO_LIBLEGATO_LINUX_TIMER_H_INCLUDE_GUARD */
