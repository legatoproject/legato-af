/**
 * @file thread.h
 *
 * Platform adaptor for threads.  Since Legato assumes a POSIX-like pthread API, this
 * just defines an init function to initialize an RTOS pthread adaptation layer.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific initialization.
 */
//--------------------------------------------------------------------------------------------------
void fa_thread_Init
(
    void
);
