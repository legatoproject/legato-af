//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the appUserAdd program, which installs a user account to the target system
 * according to an application's configuration settings in the Configuration Tree.
 *
 * Copyright 2014, Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "components/userAdderRemover/userAdderRemover.h"

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
#ifdef ADD_USER
    userAddRemove_Add(argBuff);
#else
    #ifdef REMOVE_USER
        userAddRemove_Remove(argBuff);
    #else
        #error MUST define either ADD_USER or REMOVE_USER.
    #endif
#endif

    exit(EXIT_SUCCESS);
}
