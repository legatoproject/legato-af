/**
 * This module implements the unit tests for RI Pin API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_riPin_local.h"
#include "pa_riPin.h"
#include "pa_riPin_simu.h"


//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    bool amIOwner = false;

    pa_riPin_Init();
    pa_riPinSimu_SetReturnCode(LE_OK);
    pa_riPinSimu_SetAmIOwnerOfRingSignal(false);
    le_riPin_Init();

    LE_INFO("======== Start UnitTest of RI PIN API ========");

    LE_INFO("======== Test failed return code ========");

    pa_riPinSimu_SetReturnCode(LE_FAULT);

    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_FAULT);
    LE_ASSERT(le_riPin_TakeRingSignal() == LE_FAULT);
    LE_ASSERT(le_riPin_ReleaseRingSignal() == LE_FAULT);

    LE_INFO("======== Test correct return code ========");

    pa_riPinSimu_SetReturnCode(LE_OK);
    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_OK);
    LE_ASSERT(amIOwner == false);
    pa_riPinSimu_SetAmIOwnerOfRingSignal(true);
    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_OK);
    LE_ASSERT(amIOwner == true);
    LE_ASSERT(le_riPin_ReleaseRingSignal() == LE_OK);
    pa_riPinSimu_CheckAmIOwnerOfRingSignal(false);
    LE_ASSERT(le_riPin_TakeRingSignal() == LE_OK);
    pa_riPinSimu_CheckAmIOwnerOfRingSignal(true);

    LE_INFO("======== Test Pulse function ========");
    le_riPin_PulseRingSignal(500);
    LE_ASSERT(pa_riPinSimu_Get() == 1);
    LE_ASSERT(pa_riPinSimu_Get() == 0);

    LE_INFO("======== UnitTest of RI PIN API ends with SUCCESS ========");

    exit(0);
}


