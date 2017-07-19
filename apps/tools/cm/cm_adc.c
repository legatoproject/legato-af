//-------------------------------------------------------------------------------------------------
/**
 * @file cm_adc.c
 *
 * Handle adc related functionality. ADC channels are under the control of the modem as the adc
 * is muxed with the antennae inputs and used by the modem for antenna diagnostics. Therefore
 * we have to use modem services to read thos inputs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_adc.h"
#include "cm_common.h"



//-------------------------------------------------------------------------------------------------
/**
 * Print the adc help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_adc_PrintAdcHelp
(
    void
)
{
    printf("ADC usage\n"
           "==========\n\n"
           "To print known adc channels:\n"
           "\tcm adc read channel\n"
           "\t\twhere \"channel\" is one of the ADC name\n"
           );
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the value form a named adc channel.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_adc_ReadAndPrintValue
(
    const char* channelName
)
{
    le_result_t result = LE_FAULT;
    int32_t value;

    result = le_adc_ReadValue(channelName, &value);

    if (result == LE_OK)
    {
        printf("%s:%d\n", channelName, value);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for ADC service.
 */
//--------------------------------------------------------------------------------------------------
void cm_adc_ProcessAdcCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_adc_PrintAdcHelp();
    }
    else if (strcmp(command, "read") == 0)
    {
        const char* channelName;

        if(numArgs < 3)
        {
            printf("adc read requires a channel name\n");
            exit(EXIT_FAILURE);
        }
        else if (numArgs > 3)
        {
            printf("adc read extra arguments will be ignored\n");
        }

        channelName = le_arg_GetArg(2);
        if (NULL == channelName)
        {
            LE_ERROR("channelName is NULL");
            exit(EXIT_FAILURE);
        }

        if(LE_OK != cm_adc_ReadAndPrintValue(channelName))
        {
            printf("Read %s failed.\n", channelName);
            exit(EXIT_FAILURE);
        }

    }
    else
    {
        printf("Invalid command for adc service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
