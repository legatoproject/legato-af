// -------------------------------------------------------------------------------------------------
/**
 *  Utility to work with apn from the command line.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_cfg_interface.h"

#define CFG_MODEMSERVICE_MDC_PATH "/modemServices/modemDataConnection"
#define CFG_NODE_APN "accessPointName"


// -------------------------------------------------------------------------------------------------
/**
 *  Print the help text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void HelpText
(
    void
)
{
    printf("Usage:\n\n"
            "To get APN name:\n"
            "\tapn get\n\n"
            "To set APN name:\n"
            "\tapn set <apn name>\n\n"
            );
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will set the APN name.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int GetApnName()
{
    int exitValue = EXIT_SUCCESS;
    char configPath[512];
    char apnName[512];
    le_result_t res;

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_MDC_PATH, "internet");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(configPath);

    // check if node exist
    if (le_cfg_IsEmpty(iteratorRef, ""))
    {
        printf("No APN configuration set for internet profile.\n");
        return EXIT_FAILURE;
    }

    res = le_cfg_GetString(iteratorRef, CFG_NODE_APN, apnName, sizeof(apnName), "");

    if (res != LE_OK)
    {
        exitValue = EXIT_FAILURE;
        printf("Error getting APN name for internet profile.\n");
    }
    else
    {
        printf("%s\n", apnName);
    }

    le_cfg_CancelTxn(iteratorRef);

    return exitValue;
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to set the APN name.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int SetApnName
(
    char * apn
)
{
    char configPath[512];

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_MDC_PATH, "internet");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(configPath);
    le_cfg_SetString(iteratorRef, CFG_NODE_APN, apn);
    le_cfg_CommitTxn(iteratorRef);

    return EXIT_SUCCESS;
}


COMPONENT_INIT
{
    // Make sure that our connection to the config tree is initialized.
    le_cfg_Initialize();

    int i;
    char command[256] = "";

    // No arguments
    if (le_arg_NumArgs() == 0)
    {
        HelpText();
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < le_arg_NumArgs(); i++)
    {
        le_arg_GetArg(i, command, sizeof(command));

        if (strcmp(command, "get") == 0)
        {
            exit(GetApnName());
        }
        else if (strcmp(command, "set") == 0)
        {
            if (i + 2 <= le_arg_NumArgs())
            {
                char apnName[100] = "";
                le_arg_GetArg(i+1, apnName, sizeof(apnName));
                exit(SetApnName(apnName));
            }
            else
            {
                printf("APN name missing. e.g. apn set <apn name>\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // if none of the conditions have been met, it means they entered an invalid command
    printf("Invalid command. Please try again.\n");
    HelpText();
    exit(EXIT_FAILURE);
}
