/**
 * This module implements the unit tests for RTC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_rtc_simu.h"


//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint64_t millisecondsPastGpsEpoch=0x1234567887654321;
    uint64_t millisecondsPastGpsEpochTmp=0;

    LE_INFO("======== Start UnitTest of RTC API ========");

    LE_INFO("======== Test failed return code ========");
    pa_rtcSimu_SetReturnCode(LE_FAULT);
    LE_ASSERT(le_rtc_SetUserTime(millisecondsPastGpsEpoch)==LE_FAULT);
    LE_ASSERT(le_rtc_GetUserTime(&millisecondsPastGpsEpochTmp)==LE_FAULT);

    LE_INFO("======== Test correct return code ========");
    pa_rtcSimu_SetReturnCode(LE_OK);
    LE_ASSERT(le_rtc_SetUserTime(millisecondsPastGpsEpoch)==LE_OK);
    pa_rtcSimu_CheckTime(millisecondsPastGpsEpoch);
    LE_ASSERT(le_rtc_GetUserTime(&millisecondsPastGpsEpochTmp)==LE_OK);
    LE_ASSERT(millisecondsPastGpsEpochTmp==millisecondsPastGpsEpoch);

    LE_INFO("======== UnitTest of RTC API ends with SUCCESS ========");

    exit(0);
}


