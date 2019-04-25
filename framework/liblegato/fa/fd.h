/**
 * @file fd.h
 *
 * File descriptor interface that must be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_FD_H_INCLUDE_GUARD
#define FA_FD_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Framework adaptor-specific initialization.
 *
 * @note On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void fa_fd_Init
(
    void
);

#endif /* end FA_FD_H_INCLUDE_GUARD */
