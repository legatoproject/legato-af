/**
 * @file fd.c
 *
 * Framework adaptor wrappers for File Descriptor API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fa/fd.h"


//--------------------------------------------------------------------------------------------------
/**
 * FD module initialization.
 */
//--------------------------------------------------------------------------------------------------
void fd_Init
(
    void
)
{
    fa_fd_Init();
}
