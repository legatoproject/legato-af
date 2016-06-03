/**
 * @file pa_rtc.c
 *
 * AT implementation of RTC API - stub.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_rtc.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function is a stub
 *
 * @return
 * - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_SetUserTime
(
    uint64_t  millisecondsPastGpsEpoch ///< [IN]
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------

/**
 * This function is a stub
 *
 * @return
 * - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_GetUserTime
(
    uint64_t*  millisecondsPastGpsEpochPtr ///< [OUT]
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub init function
 *
 **/
//--------------------------------------------------------------------------------------------------

le_result_t pa_rtc_Init
(
    void
)
{
    LE_INFO("AT pa_rtc init - stub");
    return LE_OK;
}
