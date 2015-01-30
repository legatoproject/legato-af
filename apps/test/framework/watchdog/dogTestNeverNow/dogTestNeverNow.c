#include "legato.h"
#include "interfaces.h"
#include <time.h>

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

    LE_INFO("Watchdog test starting");

    // Get the process name.
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s' Test ========", procName);

    LE_INFO("calling le_wdog_Timeout(TIMEOUT_NEVER)");
    le_wdog_Timeout(LE_WDOG_TIMEOUT_NEVER);

    // sleep for much longer than regular timeout
    usleep(millisecondLimit * 1000);

    LE_INFO("calling le_wdog_Timeout(TIMEOUT_NOW)");
    le_wdog_Timeout(LE_WDOG_TIMEOUT_NOW);

    // Sleep for a second. We should get killed in our sleep
    usleep(1000000);

    // We should never get here
    LE_INFO("FAIL");
}
