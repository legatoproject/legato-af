 /**
  * This module implements the le_mon's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

/*
 * Instruction to execute this test
 * 1) install application test.
 * 2) Start log trace 'logread -f | grep 'INFO'
 * 3) Start application 'app start monTest'
 * 4) check trace for the following INFO  trace:
 *     "======== Test Monitoring Services implementation Test SUCCESS ========"
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetInputVoltage()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetInputVoltage
(
)
{
    uint32_t voltage = 0;
    LE_ASSERT(le_ips_GetInputVoltage(&voltage) == LE_OK);
    LE_ASSERT(voltage != 0);
    LE_INFO("le_ips_GetInputVoltage return %d mV => %d,%03d V",
        voltage, voltage/1000, voltage%1000);
    printf("le_mon_GetInputVoltage return %d mV => %d,%03d V",
            voltage, voltage/1000, voltage%1000);
}


COMPONENT_INIT
{
    LE_INFO("======== Start Mon implementation Test========");

    LE_INFO("======== GetStateAndQual Test ========");
    Testle_ips_GetInputVoltage();
    LE_INFO("======== GetStateAndQual Test PASSED ========");

    LE_INFO("======== Test Mon implementation Test SUCCESS ========");
    exit(EXIT_SUCCESS);
}

