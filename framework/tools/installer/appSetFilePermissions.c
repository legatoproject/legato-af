//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the appSetFilePermissions program, which sets the permissions and SMACK labels
 * for an app's installed files according to an application's configuration settings in the
 * Configuration Tree.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "components/filePermissions/filePermissions.h"

COMPONENT_INIT
{
    // Get the command-line argument (there should only be one).
    if (le_arg_NumArgs() > 1)
    {
        LE_FATAL("Too many arguments.\n");
    }

    const char* appNamePtr = le_arg_GetArg(0);

    if (appNamePtr == NULL)
    {
        fprintf(stderr, "App name required.\n");
        LE_FATAL("App name required.");
    }

    // Do the work.
    filePermissions_Set(appNamePtr);

    exit(EXIT_SUCCESS);
}
