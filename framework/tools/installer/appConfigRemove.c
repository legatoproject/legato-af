//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the appCfgRemove program, which removes an application's configuration
 * from the Configuration Tree.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "components/appConfig/inc/configInstaller.h"

COMPONENT_INIT
{
    // Get the command-line argument (there should only be one).
    if (le_arg_NumArgs() > 1)
    {
        LE_FATAL("Too many arguments.\n");
    }
    const char* appName = le_arg_GetArg(0);

    if (appName == NULL)
    {
        fprintf(stderr, "App name required.\n");
        LE_FATAL("App name required.");
    }

    // Do the work.
    cfgInstall_Remove(appName);

    exit(EXIT_SUCCESS);
}
