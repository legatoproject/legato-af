#include "legato.h"
#include "interfaces.h"

/*
 * This watchdog test begins calls le_wdog_Timeout with TIMEOUT_NEVER and then waits for a minute
 * which is not technically never but should be long enough to demonstrate the timer has stopped
 * if the default timeout is less than that.
 * After a minute it calls le_wdog_Timeout with TIMEOUT_NOW and the watchdog timer should timeout
 * immediately.
 */

COMPONENT_INIT
{
    const int millisecondLimit = 60000; // one minute
    struct timespec sleepTime = { .tv_sec = (millisecondLimit / 1000), .tv_nsec = 0 };
    struct timespec endTestSleepTime = { .tv_sec = 1, .tv_nsec = 0 };

    LE_INFO("Watchdog test starting");

    // Get the process name.
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s' Test ========", procName);

    LE_INFO("calling le_wdog_Timeout(TIMEOUT_NEVER)");
    le_wdog_Timeout(LE_WDOG_TIMEOUT_NEVER);

    // sleep for much longer than regular timeout
    nanosleep(&sleepTime, NULL);

    LE_INFO("calling le_wdog_Timeout(TIMEOUT_NOW)");
    le_wdog_Timeout(LE_WDOG_TIMEOUT_NOW);

    // Sleep for a second. We should get killed in our sleep
    nanosleep(&endTestSleepTime, NULL);

    // We should never get here
    LE_INFO("FAIL");
}
