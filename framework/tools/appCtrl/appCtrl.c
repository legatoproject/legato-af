/** @file appCtrl.c
 *
 * Control Legato applications.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "le_sup_interface.h"


COMPONENT_INIT
{
    le_sup_StartClient(LE_SUPERVISOR_API);

    char argBuf[LIMIT_MAX_APP_NAME_BYTES];

    // Get the first argument which is the command.
    LE_ASSERT(le_arg_GetArg(0, argBuf, sizeof(argBuf)) == LE_OK);

    // Process the command.
    if (strcmp(argBuf, "start") == 0)
    {
        // Get the application name.
        LE_ASSERT(le_arg_GetArg(1, argBuf, sizeof(argBuf)) == LE_OK);

        // Start the application.
        switch (le_sup_StartApp(argBuf))
        {
            case LE_OK:
                exit(EXIT_SUCCESS);

            case LE_DUPLICATE:
                printf("Application '%s' is already running.\n", argBuf);
                exit(EXIT_FAILURE);

            case LE_NOT_FOUND:
                printf("Application '%s' is not installed.\n", argBuf);
                exit(EXIT_FAILURE);

            default:
                printf("There was an error.  Application '%s' could not be started.\n", argBuf);
                exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(argBuf, "stop") == 0)
    {
        // Get the application name.
        LE_ASSERT(le_arg_GetArg(1, argBuf, sizeof(argBuf)) == LE_OK);

        // Stop the application.
        switch (le_sup_StopApp(argBuf))
        {
            case LE_OK:
                exit(EXIT_SUCCESS);

            case LE_NOT_FOUND:
                printf("Application '%s' was not running.\n", argBuf);
                exit(EXIT_FAILURE);

            default:
                LE_FATAL("Unexpected response from the Supervisor.");
        }
    }
    else if (strcmp(argBuf, "stopLegato") == 0)
    {
        // Stop the framework.
        switch (le_sup_StopLegato())
        {
            case LE_OK:
                exit(EXIT_SUCCESS);

            case LE_NOT_FOUND:
                printf("Legato is being stopped by someone else.\n");
                exit(EXIT_SUCCESS);

            default:
                LE_FATAL("Unexpected response from the Supervisor.");
        }
    }

    LE_FATAL("Unknown command.");
}
