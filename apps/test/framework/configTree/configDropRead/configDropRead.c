
#include "legato.h"
#include "interfaces.h"




COMPONENT_INIT
{
    LE_INFO("----  Creating and leaving open a READ transaction.  -------------------------------");
    le_cfg_CreateReadTxn("/");

    // Check to see if we were instructed to stay running or not.  If we were, just wait until the
    // timeout expires.
    char argName[40] = "";

    if (   (le_arg_GetArg(0, argName, sizeof(argName)) != LE_OK)
        || (strcmp(argName, "hangAround") != 0))
    {
        exit(EXIT_SUCCESS);
    }
}
