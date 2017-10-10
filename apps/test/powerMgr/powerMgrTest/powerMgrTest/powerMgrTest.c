 /**
  * This module implements the le_pm_ API limit staturation tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Delay in { seconds,nano-seconds } to wait after a le_pm_StayAwake() or le_pm_Relax() call.
 *
 */
//--------------------------------------------------------------------------------------------------
#define POWER_TEST_SLEEP_TIME { 0, 200000000 }

//--------------------------------------------------------------------------------------------------
/**
 * Number of wakeup source to create and manage in this test.
 *
 */
//--------------------------------------------------------------------------------------------------
#define POWER_TEST_MAX_WS 200

//--------------------------------------------------------------------------------------------------
/**
 * Array of wakeup source to create and manage in this test.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_pm_WakeupSourceRef_t WakeupSource[POWER_TEST_MAX_WS];

//--------------------------------------------------------------------------------------------------
/**
 * Handler function called by exit(3) meaning that expected result is successful.
 *
 */
//--------------------------------------------------------------------------------------------------
void PowerMgrTestExit
(
    void
)
{
    LE_INFO("OK. Process killed");
    // Use _exit(2) directly here
    _exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int i;
    char wsStr[10];  // pwt + 3 + 1
    le_result_t rc;
    struct timespec tv = POWER_TEST_SLEEP_TIME;

    LE_INFO("Starting powerMgr Tests");
    rc = le_pm_ForceRelaxAndDestroyAllWakeupSource();
    // This service should be rejected as it not expected to have already reached the limit
    // of wakeup sources.
    if (LE_NOT_PERMITTED != rc)
    {
        LE_ERROR("Unable to kill all PM clients: %d", rc);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < POWER_TEST_MAX_WS; i++)
    {
        // Create a new wakeup source
        snprintf(wsStr, sizeof(wsStr), "pwt%d", i);
        WakeupSource[i] = le_pm_NewWakeupSource(0, wsStr);
        LE_INFO("Wakeup Source %s", wsStr);

        // Acquire the wakeup source: prevent the module to enter into sleep mode.
        rc = le_pm_StayAwake(WakeupSource[i]);
        if (LE_NO_MEMORY == rc)
        {
            // Maximum limit reached. So the test will request a kill of all clients to
            // Power Manager. By this, all wakeup sources will be released and destroyed
            // by the daemon.
            LE_ERROR("StayAwake() -> NO_MEMORY. Wait 5s before killing all PM clients");
            sleep(5);
            // Register our "successful" exit(3) handler
            atexit(PowerMgrTestExit);
            rc = le_pm_ForceRelaxAndDestroyAllWakeupSource();
            // Should never return
            LE_ERROR("Unable to kill all PM clients: %d", rc);
            // Call _exit(2) to prevent our handler to be run
            _exit(3);
        }
        nanosleep(&tv, NULL);

        if (4 <= i)
        {
            // Release the wakeup source.
            (void)le_pm_Relax(WakeupSource[i]);
            nanosleep(&tv, NULL);
        }
    }

    LE_INFO("NB wakeup sources: %d\n", i);
    exit(4);
}
