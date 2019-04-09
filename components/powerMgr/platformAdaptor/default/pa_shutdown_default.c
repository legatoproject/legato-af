/**
 * @file pa_shutdown_default.c
 *
 * Default implementation of @ref c_pa_shutdown.
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
le_result_t pa_shutdown_IssueFast
(
    void
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init PA component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}