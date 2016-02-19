/**
 * @file pa_rtc_simu.c
 *
 * simu implementation of RTC API - stub.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_rtc.h"

static le_result_t ReturnCode = LE_FAULT;
static uint64_t MillisecondsPastGpsEpochPtr = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Set the stub return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_rtcSimu_SetReturnCode
(
    le_result_t res
)
{
    ReturnCode = res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the current time.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_rtcSimu_CheckTime
(
    uint64_t  milliseconds
)
{
    LE_ASSERT(MillisecondsPastGpsEpochPtr == milliseconds);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is a stub
 *
 * @return
 * - LE_FAULT         The function failed.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_GetUserTime
(
    uint64_t*  millisecondsPastGpsEpochPtr ///< [OUT]
)
{
    if (ReturnCode == LE_OK)
    {
        *millisecondsPastGpsEpochPtr = MillisecondsPastGpsEpochPtr;
    }

    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a stub
 *
 * @return
 * - LE_FAULT         The function failed.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rtc_SetUserTime
(
    uint64_t  millisecondsPastGpsEpoch ///< [IN]
)
{
    if (ReturnCode == LE_OK)
    {
        MillisecondsPastGpsEpochPtr = millisecondsPastGpsEpoch;
    }

    return ReturnCode;
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
    LE_INFO("simulation pa_rtc init - stub");

    return LE_OK;
}
