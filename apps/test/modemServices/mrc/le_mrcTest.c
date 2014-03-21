 /**
  * This module implements the le_mrc's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "le_mrc.h"
#include "main.h"


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Network Registration Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestNetRegHandler(le_mrc_NetRegState_t state, void* contextPtr)
{
    LE_INFO("New Network Registration state: %d", state);

    if (state == LE_MRC_REG_NONE)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_NONE.");
    }
    else if (state == LE_MRC_REG_HOME)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_HOME.");
    }
    else if (state == LE_MRC_REG_SEARCHING)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_SEARCHING.");
    }
    else if (state == LE_MRC_REG_DENIED)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_DENIED.");
    }
    else if (state == LE_MRC_REG_ROAMING)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_ROAMING.");
    }
    else if (state == LE_MRC_REG_UNKNOWN)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_UNKNOWN.");
    }
    else
    {
        LE_INFO("Check NetRegHandler failed, bad Network Registration state.");
    }
}



//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Radio Power Management.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_Power()
{
    le_result_t   res;
    le_onoff_t    onoff;

    res=le_mrc_SetRadioPower(LE_OFF);
    CU_ASSERT_EQUAL(res, LE_OK);

sleep(3);

    res=le_mrc_GetRadioPower(&onoff);
    CU_ASSERT_EQUAL(res, LE_OK);
    CU_ASSERT_EQUAL(onoff, LE_OFF);

    res=le_mrc_SetRadioPower(LE_ON);
    CU_ASSERT_EQUAL(res, LE_OK);

sleep(3);

    res=le_mrc_GetRadioPower(&onoff);
    CU_ASSERT_EQUAL(res, LE_OK);
    CU_ASSERT_EQUAL(onoff, LE_ON);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Network Registration State + Signal Quality.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_GetStateAndQual()
{
    le_result_t           res;
    le_mrc_NetRegState_t  state;
    uint32_t              quality;

    res=le_mrc_GetNetRegState(&state);
    CU_ASSERT_EQUAL(res, LE_OK);
    if (res == LE_OK)
    {
        CU_ASSERT_TRUE((state>=LE_MRC_REG_NONE) && (state<=LE_MRC_REG_UNKNOWN));
    }

    res=le_mrc_GetSignalQual(&quality);
    CU_ASSERT_EQUAL(res, LE_OK);
    if (res == LE_OK)
    {
        CU_ASSERT_TRUE((quality>=0) && (quality<=5));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Network Registration notification handling.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_NetRegHdlr()
{
    le_mrc_NetRegStateHandlerRef_t testHdlrRef;

    testHdlrRef=le_mrc_AddNetRegStateHandler(TestNetRegHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(testHdlrRef);
}


