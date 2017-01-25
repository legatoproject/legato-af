/**
 * @file pa_rtc_default.c
 *
 * Default implementation of @ref c_pa_rtc.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_rtc.h"

//--------------------------------------------------------------------------------------------------
/**
 * Reads the time from the RTC.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_UNAVAILABLE   QMI succeeded but the user time was not retrieved.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_GetUserTime
(
    uint64_t*  millisecondsPastGpsEpochPtr ///< [OUT]
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Writes the time to the RTC.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_SetUserTime
(
    uint64_t  millisecondsPastGpsEpoch ///< [IN]
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
