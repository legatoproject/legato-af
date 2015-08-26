/** @file pa_rtc_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PARTCLOCAL_INCLUDE_GUARD
#define LEGATO_PARTCLOCAL_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the RTC.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_Init
(
    void
);

#endif // LEGATO_PARTCLOCAL_INCLUDE_GUARD
