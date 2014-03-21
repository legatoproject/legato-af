//--------------------------------------------------------------------------------------------------
/** @file thread.h
 *
 * This module contains initialization functions for the legato thread system that should be called
 * by the build system.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 */
#ifndef THREAD_INCLUDE_GUARD
#define THREAD_INCLUDE_GUARD

#include "eventLoop.h"
#include "mutex.h"
#include "semaphores.h"
#include "timer.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the thread system.  This function must be called before any other thread functions
 * are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void thread_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Perform thread specific initialization for the current thread
 */
//--------------------------------------------------------------------------------------------------
void thread_InitThread
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's mutex record.
 */
//--------------------------------------------------------------------------------------------------
mutex_ThreadRec_t* thread_GetMutexRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's semaphore record.
 */
//--------------------------------------------------------------------------------------------------
sem_ThreadRec_t* thread_GetSemaphoreRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's event record.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetEventRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets another thread's event record.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetOtherEventRecPtr
(
    le_thread_Ref_t threadRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's timer record.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* thread_GetTimerRecPtr
(
    void
);


#endif  // THREAD_INCLUDE_GUARD
