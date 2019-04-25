/**
 * @file fd.c
 *
 * Implementation of the @ref c_fileDescriptor framework adaptor for Linux.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific file descriptor module.
 *
 * This function must be called exactly once at process start-up before any other timer module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void fa_fd_Init
(
    void
)
{
    // Nothing to do on Linux.
}
