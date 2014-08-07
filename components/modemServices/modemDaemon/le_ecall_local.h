/**
 * @file le_ecall_local.h
 *
 * Local Modem eCall Definitions
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef LEGATO_ECALL_LOCAL_INCLUDE_GUARD
#define LEGATO_ECALL_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the eCall service
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_Init
(
    void
);


#endif // LEGATO_ECALL_LOCAL_INCLUDE_GUARD

