//-------------------------------------------------------------------------------------------------
/**
 * @file cmodem.c
 *
 * Cellular Modem Utility for command line control of the modem
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_mrc.h"
#include "cm_sim.h"
#include "cm_data.h"
#include "cm_sms.h"
#include "cm_info.h"
#include "cm_temp.h"
#include "cm_common.h"
#include "cm_adc.h"
#include "cm_ips.h"
#include "cm_rtc.h"
#include "cm_mdmCfg.h"

//--------------------------------------------------------------------------------------------------
/**
 * Static list of services supported by this tool.
 */
//--------------------------------------------------------------------------------------------------
const cm_Service_t Services[] = {

    /* SMS */
    {
        .serviceNamePtr = "sms",
        .defaultCommandPtr = "help",
        .helpHandler = cm_sms_PrintSmsHelp,
        .commandHandler = cm_sms_ProcessSmsCommand,
    },

    /* Radio */
    {
        .serviceNamePtr = "radio",
        .defaultCommandPtr = "status",
        .helpHandler = cm_mrc_PrintRadioHelp,
        .commandHandler = cm_mrc_ProcessRadioCommand,
    },

    /* Data */
    {
        .serviceNamePtr = "data",
        .defaultCommandPtr = "info",
        .helpHandler = cm_data_PrintDataHelp,
        .commandHandler = cm_data_ProcessDataCommand,
    },

    /* SIM */
    {
        .serviceNamePtr = "sim",
        .defaultCommandPtr = "status",
        .helpHandler = cm_sim_PrintSimHelp,
        .commandHandler = cm_sim_ProcessSimCommand,
    },

    /* Info */
    {
        .serviceNamePtr = "info",
        .defaultCommandPtr = "all",
        .helpHandler = cm_info_PrintInfoHelp,
        .commandHandler = cm_info_ProcessInfoCommand,
    },

    /* Temperature */
    {
        .serviceNamePtr = "temp",
        .defaultCommandPtr = "all",
        .helpHandler = cm_temp_PrintTempHelp,
        .commandHandler = cm_temp_ProcessTempCommand,
    },
    /* ADC */
    {
        .serviceNamePtr = "adc",
        .defaultCommandPtr = "help",
        .helpHandler = cm_adc_PrintAdcHelp,
        .commandHandler = cm_adc_ProcessAdcCommand
    },
    /* IPS */
    {
        .serviceNamePtr = "ips",
        .defaultCommandPtr = "read",
        .helpHandler = cm_ips_PrintIpsHelp,
        .commandHandler = cm_ips_ProcessIpsCommand
    },
    /* RTC */
    {
        .serviceNamePtr = "rtc",
        .defaultCommandPtr = "read",
        .helpHandler = cm_rtc_PrintRtcHelp,
        .commandHandler = cm_rtc_ProcessRtcCommand
    },
    /* mdmCfg */
    {
        .serviceNamePtr = "mdmCfg",
        .defaultCommandPtr = "help",
        .helpHandler = cm_mdmCfg_PrintHelp,
        .commandHandler = cm_mdmCfg_ProcessCommand
    },
};

//--------------------------------------------------------------------------------------------------
/**
 * Prints all the help text to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintAllHelp()
{
    int serviceIdx;

    for(serviceIdx = 0; serviceIdx < NUM_ARRAY_MEMBERS(Services); serviceIdx++)
    {
        const cm_Service_t * servicePtr = &Services[serviceIdx];

        if (NULL == servicePtr->helpHandler)
        {
            printf("No help for service '%s'\n", servicePtr->serviceNamePtr);
        }
        else
        {
            servicePtr->helpHandler();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Execute given command for the specfied service.
 */
//--------------------------------------------------------------------------------------------------
static void ExecuteCommand
(
    const char * serviceNamePtr,   ///< [IN] Service to address
    const char * commandPtr,       ///< [IN] Command to execute (NULL = run default command)
    uint32_t numArgs
)
{
    int serviceIdx;

    for(serviceIdx = 0; serviceIdx < NUM_ARRAY_MEMBERS(Services); serviceIdx++)
    {
        const cm_Service_t * servicePtr = &Services[serviceIdx];

        if (strcmp(servicePtr->serviceNamePtr, serviceNamePtr) == 0)
        {
            LE_FATAL_IF( (NULL == servicePtr->commandHandler),
                "No command handler for service '%s'", servicePtr->serviceNamePtr);

            if (commandPtr == NULL)
            {
                LE_FATAL_IF( (NULL == servicePtr->defaultCommandPtr),
                    "No default command for service '%s'", servicePtr->serviceNamePtr);

                servicePtr->commandHandler(servicePtr->defaultCommandPtr, numArgs);
            }
            else
            {
                servicePtr->commandHandler(commandPtr, numArgs);
            }

            return;
        }
    }

    fprintf(stderr, "Service '%s' does not exist.\n", serviceNamePtr);
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    // help menu
    if (le_arg_NumArgs() == 0)
    {
        PrintAllHelp();
        exit(EXIT_SUCCESS);
    }
    // handle service commands
    else
    {
        const char* service = le_arg_GetArg(0);
        const char* command = le_arg_GetArg(1); // Note: could return NULL.

        if ( (0 == strcmp(service, "help")) ||
             (0 == strcmp(service, "--help")) ||
             (0 == strcmp(service, "-h")) )
        {
            PrintAllHelp();
            exit(EXIT_SUCCESS);
        }
        else
        {
            ExecuteCommand(service, command, le_arg_NumArgs());
        }
    }
}

