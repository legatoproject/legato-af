/** @file pa_adc_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAADCLOCAL_INCLUDE_GUARD
#define LEGATO_PAADCLOCAL_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the sms module.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_adc_Init
(
    void
);

#endif // LEGATO_PAADCLOCAL_INCLUDE_GUARD
