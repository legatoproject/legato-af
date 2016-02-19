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
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s' Test ========", procName);

    int numArgs = le_arg_NumArgs();
    if (numArgs < 2)
    {
        LE_CRIT("Expected 2 arguments, got %d", numArgs);
    }

    const char* millisecondsStr;
    le_result_t result;
    int millisecondLongTimeout;
    int millisecondLongSleep;

    millisecondsStr = le_arg_GetArg(0);
    LE_ASSERT(millisecondsStr != NULL);
    result = le_utf8_ParseInt(&millisecondLongTimeout, millisecondsStr);
    LE_FATAL_IF(result != LE_OK,
                "Invalid number of milliseconds in timeout (%s). le_utf8_ParseInt() returned %s.",
                millisecondsStr,
                LE_RESULT_TXT(result));

    millisecondsStr = le_arg_GetArg(1);
    LE_ASSERT(millisecondsStr != NULL);
    result = le_utf8_ParseInt(&millisecondLongSleep, millisecondsStr);
    LE_FATAL_IF(result != LE_OK,
                "Invalid number of milliseconds to sleep (%s). le_utf8_ParseInt() returned %s.",
                millisecondsStr,
                LE_RESULT_TXT(result));

    LE_INFO("Starting timeout %d milliseconds then sleep for %d", millisecondLongTimeout, millisecondLongSleep);
    le_wdog_Timeout(millisecondLongTimeout);
    usleep(millisecondLongSleep * 1000);
    // We should still be alive
    LE_INFO("Kicking with configured timeout then sleep for %d", millisecondLongSleep);
    le_wdog_Kick();
    usleep(millisecondLongSleep * 1000);
    // We should never get here
    LE_FATAL("FAIL");
}
