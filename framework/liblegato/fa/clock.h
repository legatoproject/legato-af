/**
 * @file clock.h
 *
 * Clock interfaces to be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_CLK_H_INCLUDE_GUARD
#define FA_CLK_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Sets the timezone file
 *
 * @return
 *      - LE_OK if the time is correctly set
 *      - LE_FAULT if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t fa_clk_SetTimezoneFile
(
    const int32_t timezoneOffsetSeconds, ///< [IN] Timezone offset in seconds
    const uint8_t dstOffsetHours         ///< [IN] Daylight savings adjustment in hours
);

#endif /* FA_CLK_H_INCLUDE_GUARD */
