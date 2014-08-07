#include "legato.h"
#include "interfaces.h"
#include <time.h>

/*
 * This watchdog test begins by calling for a timeout longer than the usual timeout from a kick.
 * It sleeps for a time longer than the configured timeout but less than the current timeout
 * value. It should live.
 * Then it will do a standard le_wdog_Kick() and try to sleep for the same time as previously.
 * This time the watchdog should timeout and this test should be terminated.
 * The arguments are the long timeout time in milliseconds and the long sleep time.
 * The long sleep time should be less than the long timeout time.
 * The configured watchdogTimeout should be less than both of these.
 */

COMPONENT_INIT
{
    LE_INFO("Watchdog test starting");

    // Get the process name.
    char procName[100];
    LE_ASSERT(le_arg_GetProgramName(procName, sizeof(procName), NULL) == LE_OK);

    LE_INFO("======== Start '%s' Test ========", procName);

    // Buffer for the arguments in milliseconds
    char millisecondsStr[100] = {'\0'};

    int numArgs = le_arg_NumArgs();
    if (numArgs < 2)
    {
        LE_CRIT("Expected 2 arguments, got %d", numArgs);
    }

    LE_ASSERT(le_arg_GetArg(0, millisecondsStr, sizeof(millisecondsStr)) == LE_OK);
    int millisecondLongTimeout = atoi(millisecondsStr);

    LE_ASSERT(le_arg_GetArg(1, millisecondsStr, sizeof(millisecondsStr)) == LE_OK);
    int millisecondLongSleep = atoi(millisecondsStr);

    LE_INFO("Starting timeout %d milliseconds then sleep for %d", millisecondLongTimeout, millisecondLongSleep);
    le_wdog_Timeout(millisecondLongTimeout);
    usleep(millisecondLongSleep * 1000);
    // We should still be alive
    LE_INFO("Kicking with configured timeout then sleep for %d", millisecondLongSleep);
    le_wdog_Kick();
    usleep(millisecondLongSleep * 1000);
    // We should never get here
    LE_INFO("FAIL");
}
