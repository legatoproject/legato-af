/** @file pa_mdc_local.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PAMDCLOCAL_INCLUDE_GUARD
#define LEGATO_PAMDCLOCAL_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mdc module
 *
 * @return LE_NOT_POSSIBLE  The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_Init
(
    void
);

#endif // LEGATO_PAMDCLOCAL_INCLUDE_GUARD
