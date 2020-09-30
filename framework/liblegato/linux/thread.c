/**
 * @file thread.c
 *
 * Platform adaptor for threads.  Since Legato assumes a POSIX-like pthread API, this
 * just defines an init function to initialize an RTOS pthread adaptation layer.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fa/thread.h"

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific initialization.
 */
//--------------------------------------------------------------------------------------------------
void fa_thread_Init
(
    void
)
{
    // No platform initialization needed on Linux
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific priority mapping.
 * @return Mapped priority.
 */
//--------------------------------------------------------------------------------------------------
le_thread_Priority_t fa_thread_Priority
(
    le_thread_Priority_t priority   /// [IN] Thread priority
)
{
    // No mapping needed on Linux
    return priority;
}