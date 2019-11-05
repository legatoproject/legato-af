//--------------------------------------------------------------------------------------------------
/** @file main.c
 *
 * Unit test for verifying watchdog configuration behavior
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
COMPONENT_INIT
{
    LE_INFO("Watchdog test starting");
    // Get the process name.
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s' Test ========", procName);

    const char* argumentStr;
    le_result_t result;
    int watchdogTimeout;
    int maxWatchdogTimeout;
    uint64_t timeoutMs;

    int numArgs = le_arg_NumArgs();
    LE_INFO("numArgs = %d", numArgs);
    if (numArgs < 1)
    {
        LE_FATAL("Expected 1 arguments, got %d", numArgs);
    }

    le_wdog_Kick();

    argumentStr = le_arg_GetArg(0);
    LE_ASSERT(argumentStr != NULL);
    result = le_utf8_ParseInt(&watchdogTimeout, argumentStr);
    LE_FATAL_IF(result != LE_OK,
                "Invalid expected watchdogTimeout (%s). le_utf8_ParseInt() returned %s.",
                argumentStr,
                LE_RESULT_TXT(result));

    le_wdog_GetWatchdogTimeout(&timeoutMs);
    LE_INFO("WatchdogTimeout expected %d, got %u", watchdogTimeout, (uint32_t)timeoutMs);

    argumentStr = le_arg_GetArg(1);
    LE_ASSERT(argumentStr != NULL);
    result = le_utf8_ParseInt(&maxWatchdogTimeout, argumentStr);
    LE_FATAL_IF(result != LE_OK,
                "Invalid expected maxWatchdogTimeout to sleep (%s). le_utf8_ParseInt() returned %s.",
                argumentStr,
                LE_RESULT_TXT(result));

    result = le_wdog_GetMaxWatchdogTimeout(&timeoutMs);
    if (maxWatchdogTimeout == -1 && result == LE_NOT_FOUND)
    {
        LE_INFO("Got expected maxWatchdogTimeout");
    }
    else
    {
        LE_INFO("WatchdogTimeout expected %d, got %u", maxWatchdogTimeout, (uint32_t)timeoutMs);
    }
}
