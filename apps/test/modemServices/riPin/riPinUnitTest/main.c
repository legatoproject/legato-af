/**
 * This module implements the unit tests for RI Pin API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_riPin_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    bool amIOwner = false;
    uint32_t duration = 0x12345678;

    LE_INFO("======== Start UnitTest of RI PIN API ========");

    LE_INFO("======== Test failed return code ========");

    pa_riPinSimu_SetReturnCode(LE_FAULT);

    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_FAULT);
    LE_ASSERT(le_riPin_TakeRingSignal() == LE_FAULT);
    LE_ASSERT(le_riPin_ReleaseRingSignal() == LE_FAULT);

    LE_INFO("======== Test correct return code ========");

    pa_riPinSimu_SetReturnCode(LE_OK);
    pa_riPinSimu_SetAmIOwnerOfRingSignal(true);
    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_OK);
    LE_ASSERT(amIOwner == true);
    pa_riPinSimu_SetAmIOwnerOfRingSignal(false);
    LE_ASSERT(le_riPin_AmIOwnerOfRingSignal(&amIOwner) == LE_OK);
    LE_ASSERT(amIOwner == false);
    LE_ASSERT(le_riPin_TakeRingSignal() == LE_OK);
    pa_riPinSimu_CheckAmIOwnerOfRingSignal(true);
    LE_ASSERT(le_riPin_ReleaseRingSignal() == LE_OK);
    pa_riPinSimu_CheckAmIOwnerOfRingSignal(false);
    le_riPin_PulseRingSignal(duration);
    pa_riPinSimu_CheckPulseRingSignalDuration(duration);

    LE_INFO("======== UnitTest of RI PIN API ends with SUCCESS ========");

    return 0;
}


