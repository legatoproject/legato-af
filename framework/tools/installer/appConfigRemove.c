//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the appCfgRemove program, which removes an application's configuration
 * from the Configuration Tree.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "components/appConfig/inc/configInstaller.h"

COMPONENT_INIT
{
    char argBuff[256];

    // Get the command-line argument (there should only be one).
    if (le_arg_NumArgs() > 1)
    {
        LE_FATAL("Too many arguments.\n");
    }
    switch (le_arg_GetArg(0, argBuff, sizeof(argBuff)))
    {
        case LE_OVERFLOW:
            LE_FATAL("App name too long (longer than %zu bytes)", sizeof(argBuff) - 1);

        case LE_NOT_FOUND:
            LE_FATAL("App name required.");

        case LE_OK:
            break;

        default:
            LE_FATAL("Unexpected return code from le_arg_GetArg().");
    }

    // Do the work.
    cfgInstall_Remove(argBuff);

    exit(EXIT_SUCCESS);
}
