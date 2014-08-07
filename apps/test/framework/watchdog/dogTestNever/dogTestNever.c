#include "legato.h"
#include "interfaces.h"

/*
 * This test is to validate that the config parser handles watchdogTimeout: never and that the
 * watchdog honours that config. See the adef
 */

COMPONENT_INIT
{
    LE_INFO("Watchdog test starting");

    // Get the process name.
    char procName[100];
    LE_ASSERT(le_arg_GetProgramName(procName, sizeof(procName), NULL) == LE_OK);

    LE_INFO("======== Start '%s' Test ========", procName);

    LE_INFO("calling le_wdog_Kick()");
    le_wdog_Kick();

    sleep(58);
    LE_INFO("dogTestNever still alive after 58 sec");

    // We should never see a message about this proc timing out
}
