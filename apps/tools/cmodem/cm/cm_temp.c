//-------------------------------------------------------------------------------------------------
/**
 * @file cm_temp.c
 *
 * Handle temperature related functionality
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    TEMP_SOURCE_RADIO,
    TEMP_SOURCE_PLATFORM,
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
    printf("Temperature usage\n"
           "==========\n\n"
           "To print all known temperatures:\n"
           "\tcm temp\n"
           "\tcm temp all\n\n"
           "To print all thresholds:\n"
           "\tcm temp thresholds\n\n"
           "To print the radio temperature:\n"
           "\tcm temp radio\n\n"
           "To print the platform temperature:\n"
           "\tcm temp platform\n\n"
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
        case TEMP_SOURCE_RADIO:
            sourceStr = "Radio temperature";
            res = le_temp_GetRadioTemperature(&temp);
            break;
        case TEMP_SOURCE_PLATFORM:
            sourceStr = "Platform temperature";
            res = le_temp_GetPlatformTemperature(&temp);
            break;
        default:
            LE_FATAL("Invalid source %d", source);
            break;
    }

    if (res != LE_OK)
    {
        printf("Unable to get threshold for source=%d", source);
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
    le_result_t res;
    int32_t lowCriticalTemp = -1, lowWarningTemp = -1;
    int32_t hiWarningTemp, hiCriticalTemp;
    const char * sourceStr;

    switch(source)
    {
        case TEMP_SOURCE_RADIO:
            sourceStr = "Radio";
            res = le_temp_GetRadioThresholds(&hiWarningTemp,
                                             &hiCriticalTemp);
            break;
        case TEMP_SOURCE_PLATFORM:
            sourceStr = "Platform";
            res = le_temp_GetPlatformThresholds(&lowCriticalTemp,
                                                &lowWarningTemp,
                                                &hiWarningTemp,
                                                &hiCriticalTemp);
            break;
        default:
            LE_FATAL("Invalid source %d", source);
            break;
    }

    if (res != LE_OK)
    {
        printf("Unable to get threshold for source=%d", source);
        exit(EXIT_FAILURE);
    }

    printf("%s temperature thresholds:\n", sourceStr);

    if (lowWarningTemp != -1)
        printf(" - Warning low:    %3d C\n", lowWarningTemp);

    printf(" - Warning high:   %3d C\n", hiWarningTemp);

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
        cm_temp_PrintTemp(true, TEMP_SOURCE_RADIO);
        cm_temp_PrintTemp(true, TEMP_SOURCE_PLATFORM);
    }
    else if (strcmp(command, "radio") == 0)
    {
        cm_temp_PrintTemp(false, TEMP_SOURCE_RADIO);
    }
    else if (strcmp(command, "platform") == 0)
    {
        cm_temp_PrintTemp(false, TEMP_SOURCE_PLATFORM);
    }
    else if (strcmp(command, "thresholds") == 0)
    {
        cm_temp_PrintThreshold(TEMP_SOURCE_RADIO);
        cm_temp_PrintThreshold(TEMP_SOURCE_PLATFORM);
    }
    else
    {
        printf("Invalid command for temp service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
