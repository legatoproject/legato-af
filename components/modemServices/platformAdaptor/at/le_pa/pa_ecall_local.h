/** @file pa_ecall_local.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef LEGATO_PAECALLLOCAL_INCLUDE_GUARD
#define LEGATO_PAECALLLOCAL_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ecall module
 *
 * @return LE_NOT_POSSIBLE  The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ecall_Init
(
    void
);

#endif // LEGATO_PAECALLLOCAL_INCLUDE_GUARD
