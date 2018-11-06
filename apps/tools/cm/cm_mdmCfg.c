//-------------------------------------------------------------------------------------------------
/**
 * @file cm_mdmCfg.c
 *
 * Handle MDM configuration related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_mdmCfg.h"
#include "cm_common.h"

//-------------------------------------------------------------------------------------------------
/**
 * Print the mdmCfg help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_mdmCfg_PrintHelp
(
    void
)
{
    printf("mdmCfg usage\n"
           "==========\n\n"
           "To store the modem current configurations :\n"
           "\tcm mdmCfg save\n"
           "To restore the modem saved configurations :\n"
           "\tcm mdmCfg restore\n"
           );
}

//-------------------------------------------------------------------------------------------------
/**
 * Store the modem current configurations
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_mdmCfg_StoreCurrentConfiguration
(
    void
)
{
    return le_mdmCfg_StoreCurrentConfiguration();
}

//-------------------------------------------------------------------------------------------------
/**
 * Restore previously saved the modem configuration.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_mdmCfg_RestoreSavedConfiguration
(
    void
)
{
    return le_mdmCfg_RestoreSavedConfiguration();
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for mdmCfg service.
 */
//--------------------------------------------------------------------------------------------------
void cm_mdmCfg_ProcessCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_mdmCfg_PrintHelp();
    }
    else if (strcmp(command, "save") == 0)
    {
        if ( LE_OK == cm_mdmCfg_StoreCurrentConfiguration())
        {
            printf("Succeeded\n");
        }
        else
        {
            printf("Failed\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "restore") == 0)
    {
        if ( LE_OK == cm_mdmCfg_RestoreSavedConfiguration())
        {
            printf("Succeeded\n");
        }
        else
        {
            printf("Failed\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cm_mdmCfg_PrintHelp();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
