//--------------------------------------------------------------------------------------------------
/**
 * @file le_rtc.c
 *
 * This file contains the source code of the high level API to set the user time.
 * The user time is available to store a time that will be updated with the battery backed up
 * (VCOIN) clock.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_rtc.h"

//--------------------------------------------------------------------------------------------------
/**
 * Set the user time offset.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rtc_SetUserTime
(
    uint64_t   millisecondsPastGpsEpoch ///< [IN]
)
{
    return pa_rtc_SetUserTime(millisecondsPastGpsEpoch);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the user time offset.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 *      - LE_UNAVAILABLE   The function succeeded but no valid user time was retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rtc_GetUserTime
(
    uint64_t*   millisecondsPastGpsEpochPtr ///< [OUT]
)
{
    return pa_rtc_GetUserTime(millisecondsPastGpsEpochPtr);
}
