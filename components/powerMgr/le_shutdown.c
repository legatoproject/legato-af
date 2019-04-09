/**
 * @file le_shutdown.c
 *
 * This file contains the source code of the top level shutdown API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_shutdown.h"

//--------------------------------------------------------------------------------------------------
// Public Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Issue a fast shutdown.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_FAULT         Internal error
 *  - LE_UNSUPPORTED   Feature not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_shutdown_IssueFast
(
    void
)
{
    return pa_shutdown_IssueFast();
}
