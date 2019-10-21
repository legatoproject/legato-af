/**
 * @file thread.h
 *
 * Framework adaptor for threads.  Since Legato assumes a POSIX-like pthread API, this
 * just defines an init function to initialize an RTOS pthread adaptation layer.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_THREAD_H_INCLUDE_GUARD
#define FA_THREAD_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific initialization.
 */
//--------------------------------------------------------------------------------------------------
void fa_thread_Init
(
    void
);

#endif /* FA_THREAD_H_INCLUDE_GUARD */
