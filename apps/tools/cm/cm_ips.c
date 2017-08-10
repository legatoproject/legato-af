//-------------------------------------------------------------------------------------------------
/**
 * @file cm_ips.c
 *
 * Handle IPS (input power supply) related functionality.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_ips.h"
#include "cm_common.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Maximum Number of arguments for the CM tool IPS usage.
 */
//--------------------------------------------------------------------------------------------------
#define  CM_MAX_ARGUMENTS_FOR_IPS_HELP          2
#define  CM_MAX_ARGUMENTS_FOR_IPS_READ          2
#define  CM_MAX_ARGUMENTS_FOR_IPS_THRESHOLDS    2

//-------------------------------------------------------------------------------------------------
/**
 * Print the IPS help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_ips_PrintIpsHelp
(
    void
)
{
    printf("IPS usage\n"
           "==========\n\n"
           "To read and print information about the power supply "
           "(voltage, power source, battery level):\n"
           "\tcm ips\n"
           "\tcm ips read\n\n"
           "To read and print the input voltage thresholds:\n"
           "\tcm ips thresholds\n"
           );
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the input voltage.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_ips_ReadAndPrintVoltage
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t value;

    result = le_ips_GetInputVoltage(&value);
    if (result == LE_OK)
    {
        printf("Voltage: %u mV\n", value);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the power source and the battery level.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_ips_ReadAndPrintPowerSourceAndBatteryLevel
(
    void
)
{
    le_result_t result = LE_FAULT;
    le_ips_PowerSource_t powerSource;

    result = le_ips_GetPowerSource(&powerSource);
    if (LE_OK != result)
    {
        return result;
    }

    switch (powerSource)
    {
        case LE_IPS_POWER_SOURCE_EXTERNAL:
            printf("Powered by an external source\n");
            break;

        case LE_IPS_POWER_SOURCE_BATTERY:
        {
            uint8_t batteryLevel;

            printf("Powered by a battery\n");

            result = le_ips_GetBatteryLevel(&batteryLevel);
            if (LE_OK != result)
            {
                return result;
            }

            printf("\tBattery level: %d%%\n", batteryLevel);
        }
        break;

        default:
            printf("Unknown power source\n");
            break;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the input voltage thresholds.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t cm_ips_ReadAndPrintInputVoltageThresholds
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint16_t criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt;

    result = le_ips_GetVoltageThresholds( &criticalInVolt, &warningInVolt,
            &normalInVolt, &hiCriticalInVolt);

    if (result == LE_OK)
    {
        printf("criticalInVolt %u mV, warningInVolt %u mV,"
            " normalInVolt %u mV, hiCriticalInVolt %u mV\n",
            criticalInVolt, warningInVolt,
            normalInVolt, hiCriticalInVolt);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Process commands for IPS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_ips_ProcessIpsCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    // True if the command contains extra arguments.
    bool extraArguments = true;

    if (strcmp(command, "help") == 0)
    {
        if (numArgs <= CM_MAX_ARGUMENTS_FOR_IPS_HELP)
        {
            extraArguments = false;
            cm_ips_PrintIpsHelp();
        }
    }
    else if (strcmp(command, "read") == 0)
    {
        if (numArgs <= CM_MAX_ARGUMENTS_FOR_IPS_READ)
        {
            extraArguments = false;
            if (LE_OK != cm_ips_ReadAndPrintVoltage())
            {
                printf("Voltage read failed.\n");
                exit(EXIT_FAILURE);
            }
            if (LE_OK != cm_ips_ReadAndPrintPowerSourceAndBatteryLevel())
            {
                printf("Power source and battery level read failed.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (strcmp(command, "thresholds") == 0)
    {
        if (numArgs <= CM_MAX_ARGUMENTS_FOR_IPS_THRESHOLDS)
        {
            extraArguments = false;
            if (LE_OK != cm_ips_ReadAndPrintInputVoltageThresholds())
            {
               printf("Read Input Voltage thresholds failed.\n");
               exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        printf("Invalid command for IPS service.\n");
        exit(EXIT_FAILURE);
    }

    if (true == extraArguments)
    {
        printf("Invalid command for IPS service.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }

}
