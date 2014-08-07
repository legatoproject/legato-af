 /**
  * This module implements the le_ecall's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
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

#include "interfaces.h"
#include "main.h"

#if 0
VIN: WM9VDSVDSYA123456
static uint8_t ImportedMsd[] ={0x01, 0x5C, 0x06, 0x81, 0xD5, 0x49, 0x70, 0xD6, 0x5C, 0x35, 0x97, 0xCA,
                               0x04, 0x20, 0xC4, 0x14, 0x67, 0xF1, 0x03, 0xAD, 0xE6, 0x8A, 0xC5, 0x2E,
                               0x9B, 0xB8, 0x41, 0x3F, 0x14, 0x9C, 0x07, 0x41, 0x4F, 0xB4, 0x14, 0xF6,
                               0x01, 0x01, 0x80, 0x81, 0x3E, 0x82, 0x18, 0x18, 0x23, 0x23, 0x00};
#endif

// VIN: ASDAJNPR1VABCDEFG
static uint8_t ImportedMsd[] ={0x01, 0x4C, 0x07, 0x80, 0xA6, 0x4D, 0x29, 0x25, 0x97, 0x60, 0x17, 0x0A,
                               0x2C, 0xC3, 0x4E, 0x3D, 0x05, 0x1B, 0x18, 0x48, 0x61, 0xEB, 0xA0, 0xC8,
                               0xFF, 0x73, 0x7E, 0x64, 0x20, 0xD1, 0x04, 0x01, 0x3F, 0x81, 0x00};

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for eCall state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyECallEventHandler
(
    le_ecall_State_t    state,
    void*               contextPtr
)
{
    LE_INFO("eCall TEST: New eCall state: %d", state);

    if (state == LE_ECALL_STATE_CONNECTED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_CONNECTED.");
    }
    else if (state == LE_ECALL_STATE_MSD_TX_COMPLETED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
    }
    else if (state == LE_ECALL_STATE_MSD_TX_FAILED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_MSD_TX_FAILED.");
    }
    else if (state == LE_ECALL_STATE_STOPPED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_STOPPED.");
    }
    else if (state == LE_ECALL_STATE_RESET)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_RESET.");
    }
    else if (state == LE_ECALL_STATE_COMPLETED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_COMPLETED.");
    }
    else if (state == LE_ECALL_STATE_FAILED)
    {
        LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_FAILED.");
    }
    else
    {
        LE_INFO("Check MyECallEventHandler failed, unknowm state.");
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Import or set MSD elements.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_LoadMsd()
{
    le_result_t         res = LE_FAULT;
    le_ecall_ObjRef_t   testECallRef;

    LE_INFO("Start Testle_ecall_LoadMsd");

    // Check LE_DUPLICATE on le_ecall_ImportMsd
    testECallRef=le_ecall_Create();
    CU_ASSERT_PTR_NOT_NULL(testECallRef);

    res=le_ecall_SetMsdPosition(testECallRef, true, +48898064, +2218092, 0);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_SetMsdPassengersCount(testECallRef, 3);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_ImportMsd(testECallRef, ImportedMsd, sizeof(ImportedMsd));
    CU_ASSERT_EQUAL(res, LE_DUPLICATE);

    res=le_ecall_StartTest(testECallRef);
    CU_ASSERT_EQUAL(res, LE_OK);

    if (res == LE_OK)
    {
        res=le_ecall_End(testECallRef);
        CU_ASSERT_EQUAL(res, LE_OK);

        le_ecall_Delete(testECallRef);

        sleep (3);

        // Check LE_DUPLICATE on le_ecall_SetMsdPosition and le_ecall_SetMsdPassengersCount
        testECallRef=le_ecall_Create();
        CU_ASSERT_PTR_NOT_NULL(testECallRef);

        res=le_ecall_ImportMsd(testECallRef, ImportedMsd, sizeof(ImportedMsd));
        CU_ASSERT_EQUAL(res, LE_OK);

        res=le_ecall_SetMsdPosition(testECallRef, true, +48070380, -11310000, 45);
        CU_ASSERT_EQUAL(res, LE_DUPLICATE);

        res=le_ecall_SetMsdPassengersCount(testECallRef, 3);
        CU_ASSERT_EQUAL(res, LE_DUPLICATE);

        res=le_ecall_StartTest(testECallRef);
        CU_ASSERT_EQUAL(res, LE_OK);

        if (res == LE_OK)
        {
            res=le_ecall_End(testECallRef);
            CU_ASSERT_EQUAL(res, LE_OK);
        }
    }

    le_ecall_Delete(testECallRef);
    sleep(5);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a manual eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_StartManual()
{
    le_result_t        res = LE_FAULT;
    le_ecall_ObjRef_t  testECallRef;
    le_ecall_State_t   state;

    LE_INFO("Start Testle_ecall_StartManual");

    testECallRef=le_ecall_Create();
    CU_ASSERT_PTR_NOT_NULL(testECallRef);

    res=le_ecall_ImportMsd(testECallRef, ImportedMsd, sizeof(ImportedMsd));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_StartManual(testECallRef);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_StartTest(testECallRef);
    CU_ASSERT_EQUAL(res, LE_DUPLICATE);
    res=le_ecall_StartAutomatic(testECallRef);
    CU_ASSERT_EQUAL(res, LE_DUPLICATE);

    res=le_ecall_End(testECallRef);
    CU_ASSERT_EQUAL(res, LE_OK);

    state=le_ecall_GetState(testECallRef);
    CU_ASSERT_TRUE(((state>=LE_ECALL_STATE_CONNECTED) && (state<=LE_ECALL_STATE_FAILED)));

    le_ecall_Delete(testECallRef);
    sleep(5);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a test eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_StartTest()
{
    le_result_t                        res = LE_FAULT;
    le_ecall_ObjRef_t                  testECallRef;
    le_ecall_State_t                   state;
    le_ecall_StateChangeHandlerRef_t   stateChangeHandlerRef;

    LE_INFO("Start Testle_ecall_StartTest");

    stateChangeHandlerRef = le_ecall_AddStateChangeHandler(MyECallEventHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(stateChangeHandlerRef);

    testECallRef=le_ecall_Create();
    CU_ASSERT_PTR_NOT_NULL(testECallRef);

    res=le_ecall_SetMsdPosition(testECallRef, true, +48898064, +2218092, 0);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_SetMsdPassengersCount(testECallRef, 3);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_StartTest(testECallRef);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_ecall_StartManual(testECallRef);
    CU_ASSERT_EQUAL(res, LE_DUPLICATE);
    res=le_ecall_StartAutomatic(testECallRef);
    CU_ASSERT_EQUAL(res, LE_DUPLICATE);

    state=le_ecall_GetState(testECallRef);
    CU_ASSERT_TRUE(((state>=LE_ECALL_STATE_CONNECTED) && (state<=LE_ECALL_STATE_FAILED)));
}



