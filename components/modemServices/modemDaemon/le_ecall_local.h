/**
 * @file le_ecall_local.h
 *
 * Local Modem eCall Definitions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_ECALL_LOCAL_INCLUDE_GUARD
#define LEGATO_ECALL_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the eCall service
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_Init
(
    void
);


#endif // LEGATO_ECALL_LOCAL_INCLUDE_GUARD

