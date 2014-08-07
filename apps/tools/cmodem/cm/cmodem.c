
// -------------------------------------------------------------------------------------------------
/**
 *  @file cmodem.c
 *
 *  Cellular Modem Utility for command line control of the modem
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_mrc.h"
#include "cm_sim.h"
#include "cm_data.h"

#define MRC_SERVICE                 "radio"
#define SIM_SERVICE                 "sim"
#define DATA_CONNECTION_SERVICE     "data"


//--------------------------------------------------------------------------------------------------
/**
* Prints all the help text to stdout.
*/
//--------------------------------------------------------------------------------------------------
static void PrintAllHelp()
{
    cm_mrc_PrintRadioHelp();
    cm_sim_PrintSimHelp();
    cm_data_PrintDataHelp();
}


//--------------------------------------------------------------------------------------------------
/**
* Verify if enough parameter passed into command. If not, output error message to stdout.
*/
//--------------------------------------------------------------------------------------------------
static bool EnoughCmdParam
(
    size_t requiredParam,   ///< [IN] Required parameters for the command
    size_t numArgs,         ///< [IN] Number of arguments passed into the command line
    const char * errorMsg   ///< [IN] Error message to output if not enough parameters
)
{
    if (requiredParam + 1 < numArgs)
    {
        return true;
    }
    else
    {
        printf("%s\n\n", errorMsg);
        exit(EXIT_FAILURE);
        return false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
* Process commands for radio service.
*/
//--------------------------------------------------------------------------------------------------
static void ProcessRadioCommand
(
    const char * command,   ///< [IN] Radio command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_mrc_PrintRadioHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "on") == 0)
    {
        exit(cm_mrc_SetRadioPower(LE_ON));
    }
    else if (strcmp(command, "off") == 0)
    {
        exit(cm_mrc_SetRadioPower(LE_OFF));
    }
    else if (strcmp(command, "rat") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "RAT value missing. e.g. cm radio rat <CDMA/GSM/UMTS/LTE>"))
        {
            char rat[100];
            le_arg_GetArg(2, rat, sizeof(rat));
            exit(cm_mrc_SetRat(rat));
        }
    }
    else
    {
        printf("Invalid command for radio service.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
* Process commands for sim service.
*/
//--------------------------------------------------------------------------------------------------
static void ProcessSimCommand
(
    const char * command,   ///< [IN] Sim commands
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_sim_PrintSimHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "enterpin") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "PIN code missing. e.g. cm sim enterpin <pin>"))
        {
            char pin[100] = "";
            le_arg_GetArg(2, pin, sizeof(pin));
            exit(cm_sim_EnterPin(pin));
        }
    }
    else if (strcmp(command, "changepin") == 0)
    {
        if (EnoughCmdParam(2, numArgs, "PIN code missing. e.g. cm sim changepin <pin>"))
        {
            char oldPin[10] = "";
            char newPin[10] = "";

            le_arg_GetArg(2, oldPin, sizeof(oldPin));
            le_arg_GetArg(3, newPin, sizeof(newPin));
            exit(cm_sim_ChangePin(oldPin, newPin));
        }
    }
    else if (strcmp(command, "lock") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "PIN code missing. e.g. cm sim lock <pin>"))
        {
            char pin[100] = "";
            le_arg_GetArg(2, pin, sizeof(pin));
            exit(cm_sim_LockSim(pin));
        }
    }
    else if (strcmp(command, "unlock") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "PIN code missing. e.g. cm sim unlock <pin>"))
        {
            char pin[100] = "";
            le_arg_GetArg(2, pin, sizeof(pin));
            exit(cm_sim_UnlockSim(pin));
        }
    }
    else if (strcmp(command, "unblock") == 0)
    {
        if (EnoughCmdParam(2, numArgs, "PUK/PIN code missing. e.g. cm sim unblock <puk> <newpin>"))
        {
            char puk[100] = "";
            char newpin[100] = "";
            le_arg_GetArg(2, puk, sizeof(puk));
            le_arg_GetArg(3, newpin, sizeof(newpin));
            exit(cm_sim_UnblockSim(puk, newpin));
        }
    }
    else if (strcmp(command, "storepin") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "PIN code missing. e.g. cm sim storepin <pin>"))
        {
            char pin[100] = "";
            le_arg_GetArg(2, pin, sizeof(pin));
            exit(cm_sim_StorePin(pin));
        }
    }
    else
    {
        printf("Invalid command for SIM service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
* Process commands for data service.
*/
//--------------------------------------------------------------------------------------------------
static void ProcessDataCommand
(
    const char * command,   ///< [IN] Data commands
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_data_PrintDataHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "profile") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "Profile index missing. e.g. cm data profile <index> (Use cm data list to show you valid indexes)"));
        {
            char profile[10];
            le_arg_GetArg(2, profile, sizeof(profile));
            exit(cm_data_SetProfileInUse(atoi(profile)));
        }
    }
    else if (strcmp(command, "connect") == 0)
    {
        if (numArgs == 2)
        {
            cm_data_StartDataConnection(NULL);
        }
        else if (numArgs == 3)
        {
            char timeout[100];
            le_arg_GetArg(2, timeout, sizeof(timeout));
            cm_data_StartDataConnection(timeout);
        }
        else
        {
            printf("Invalid argument when starting a data connection. e.g. cm data connect <optional timeout (secs)>\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "apn") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "APN name missing. e.g. cm data apn <apn name>"))
        {
            char apn[100];
            le_arg_GetArg(2, apn, sizeof(apn));
            exit(cm_data_SetApnName(apn));
        }
    }
    else if (strcmp(command, "pdp") == 0)
    {
        if (EnoughCmdParam(1, numArgs, "PDP type name missing. e.g. cm data pdp <pdp type>"))
        {
            char pdpType[100];
            le_arg_GetArg(2, pdpType, sizeof(pdpType));
            exit(cm_data_SetPdpType(pdpType));
        }
    }
    else if (strcmp(command, "auth") == 0)
    {
        char type[10];
        char user[100];
        char pwd[100];

        // configure all authentication info
        if (numArgs == 5)
        {
            le_arg_GetArg(2, type, sizeof(type));
            le_arg_GetArg(3, user, sizeof(user));
            le_arg_GetArg(4, pwd, sizeof(pwd));

            exit(cm_data_SetAuthentication(type, user, pwd));
        }
        // for none option
        else if (numArgs == 3)
        {
            le_arg_GetArg(2, type, sizeof(type));
            exit(cm_data_SetAuthentication(type, user, pwd));
        }
        else
        {
            printf("Auth parameters incorrect. e.g. cm data auth <auth type> [<username>] [<password>]\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "list") == 0)
    {
        exit(cm_data_ListProfileName());
    }
    else if (strcmp(command, "watch") == 0)
    {
        cm_data_MonitorDataConnection();
    }
    else
    {
        printf("Invalid command for data service.\n");
        exit(EXIT_FAILURE);
    }
}


COMPONENT_INIT
{
    // help menu
    if (le_arg_NumArgs() == 0)
    {
        PrintAllHelp();
        exit(EXIT_SUCCESS);
    }
    // handle service info
    else if (le_arg_NumArgs() == 1)
    {
        char service[64];

        le_arg_GetArg(0, service, sizeof(service));

        if (strcmp(service, MRC_SERVICE) == 0)
        {
            exit(cm_mrc_GetModemStatus());
        }
        else if (strcmp(service, SIM_SERVICE) == 0)
        {
            exit(cm_sim_GetSimStatus());
        }
        else if (strcmp(service, DATA_CONNECTION_SERVICE) == 0)
        {
            /* @todo Current data connection tool is currently limited to only using internet profile.
            * When dcsDaemon supports the use of another profile, we will enable the feature to allow
            * users to select another profile from cmodem tool.
            */
            exit(cm_data_GetProfileInfo());
        }
        else
        {
            printf("This service does not exist.\n");
            exit(EXIT_FAILURE);
        }
    }
    // handle service commands
    else
    {
        char service[64];
        char command[64];

        le_arg_GetArg(0, service, sizeof(service));
        le_arg_GetArg(1, command, sizeof(command));

        if (strcmp(service, MRC_SERVICE) == 0)
        {
            ProcessRadioCommand(command, le_arg_NumArgs());
        }
        else if (strcmp(service, SIM_SERVICE) == 0)
        {
            ProcessSimCommand(command, le_arg_NumArgs());
        }
        else if (strcmp(service, DATA_CONNECTION_SERVICE) == 0)
        {
            ProcessDataCommand(command, le_arg_NumArgs());
        }
        else
        {
            printf("This service does not exist.\n");
            exit(EXIT_FAILURE);
        }
    }
}

