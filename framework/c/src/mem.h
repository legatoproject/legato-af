//--------------------------------------------------------------------------------------------------
/** @file mem.h
 *
 * This module contains initialization functions for the legato memory management system that should
 * be called by the build system.
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 *
 */
#ifndef MEM_INCLUDE_GUARD
#define MEM_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the memory pool system.  This function must be called before any other memory pool
 * functions are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void mem_Init
(
    void
);

#endif  // MEM_INCLUDE_GUARD
