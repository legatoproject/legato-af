//--------------------------------------------------------------------------------------------------
/** @file clock.c
 *
 * This file contains linux specific implementations of the le_clk APIs
 *
 * Copyright (C) Sierra Wireless Inc.
 */
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
)
{
    LE_UNUSED(timezoneOffsetSeconds);
    LE_UNUSED(dstOffsetHours);
    return LE_UNSUPPORTED;
}
