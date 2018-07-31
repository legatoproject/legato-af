//-------------------------------------------------------------------------------------------------
/**
 * @file cm_temp.c
 *
 * Handle temperature related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_temp.h"
#include "cm_common.h"

//-------------------------------------------------------------------------------------------------
/**
 * Temperature source.
 */
//-------------------------------------------------------------------------------------------------
typedef enum {
    TEMP_SOURCE_PA,
    TEMP_SOURCE_PC,
}
TemperatureSource_t;

//-------------------------------------------------------------------------------------------------
/**
 * Print the info help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_temp_PrintTempHelp
(
    void
)
{
    printf("Temperature usage \n"
           "==========\n\n"
           "To print all known temperatures:\n"
           "\tcm temp\n"
           "\tcm temp all\n\n"
           "To print all thresholds:(applicable for AR755x, AR8652, and WP8548 platforms only)\n"
           "\tcm temp thresholds\n\n"
           "To print the Power Amplifier temperature:\n"
           "\tcm temp pa\n\n"
           "To print the Power Controller temperature:\n"
           "\tcm temp pc\n\n"
           );
}

//-------------------------------------------------------------------------------------------------
/**
 * Print temperature specified by @ref source.
 */
//-------------------------------------------------------------------------------------------------
void cm_temp_PrintTemp
(
    bool withHeaders,
    TemperatureSource_t source
)
{
    le_result_t res;
    int32_t temp;
    const char * sourceStr;
    char tempStr[20];

    switch(source)
    {
        case TEMP_SOURCE_PA:
            sourceStr = "Power Amplifier temperature";
            le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
            res = le_temp_GetTemperature(paSensorRef, &temp);
            break;
        case TEMP_SOURCE_PC:
            sourceStr = "Power Controller temperature";
            le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
            res = le_temp_GetTemperature(pcSensorRef, &temp);
            break;
        default:
            LE_FATAL("Invalid source %d", source);
            break;
    }

    if (res != LE_OK)
    {
        printf("Unable to get temperature for source=%d", source);
        exit(EXIT_FAILURE);
    }

    snprintf(tempStr, sizeof(tempStr), "%d", temp);

    if(withHeaders)
    {
        cm_cmn_FormatPrint(sourceStr, tempStr);
    }
    else
    {
        printf("%s\n", tempStr);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Print all thresholds.
 */
//-------------------------------------------------------------------------------------------------
void cm_temp_PrintThreshold
(
    TemperatureSource_t source
)
{
    int32_t lowCriticalTemp = -1, lowNormalTemp = -1;
    int32_t hiNormalTemp, hiCriticalTemp;
    const char * sourceStr;

    switch(source)
    {
        case TEMP_SOURCE_PA:
            sourceStr = "Power Amplifier";
            le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
            if ((le_temp_GetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", &hiNormalTemp) != LE_OK) ||
                (le_temp_GetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", &hiCriticalTemp) != LE_OK))
            {
                printf("Unable to get threshold for source=%d", source);
                exit(EXIT_FAILURE);
            }
            break;
        case TEMP_SOURCE_PC:
            sourceStr = "Power Controller";
            le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
            if ((le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", &lowCriticalTemp) != LE_OK) ||
                (le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &lowNormalTemp) != LE_OK) ||
                (le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &hiNormalTemp) != LE_OK) ||
                (le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &hiCriticalTemp) != LE_OK))
            {
                printf("Unable to get threshold for source=%d", source);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            LE_FATAL("Invalid source %d", source);
            break;
    }

    printf("%s temperature thresholds:\n", sourceStr);

    if (lowNormalTemp != -1)
        printf(" - Warning low:    %3d C\n", lowNormalTemp);

    printf(" - Warning high:   %3d C\n", hiNormalTemp);

    if (lowCriticalTemp != -1)
        printf(" - Critical low:   %3d C\n", lowCriticalTemp);

    printf(" - Critical high:  %3d C\n", hiCriticalTemp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for temp service.
 */
//--------------------------------------------------------------------------------------------------
void cm_temp_ProcessTempCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (strcmp(command, "help") == 0)
    {
        cm_temp_PrintTempHelp();
    }
    else if (strcmp(command, "all") == 0)
    {
        cm_temp_PrintTemp(true, TEMP_SOURCE_PA);
        cm_temp_PrintTemp(true, TEMP_SOURCE_PC);
    }
    else if (strcmp(command, "pa") == 0)
    {
        cm_temp_PrintTemp(false, TEMP_SOURCE_PA);
    }
    else if (strcmp(command, "pc") == 0)
    {
        cm_temp_PrintTemp(false, TEMP_SOURCE_PC);
    }
    else if (strcmp(command, "thresholds") == 0)
    {
        cm_temp_PrintThreshold(TEMP_SOURCE_PA);
        cm_temp_PrintThreshold(TEMP_SOURCE_PC);
    }
    else
    {
        printf("Invalid command for temp service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
