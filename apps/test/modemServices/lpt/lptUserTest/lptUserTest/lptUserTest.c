 /**
  * This module implements the LPT user tests.
  *
  * The test can be run with:
  *     app runProc lptUserTest --exe=lptUserTest -- setEDrxState <rat> <on|off>
  *     app runProc lptUserTest --exe=lptUserTest -- setEDrxParams <rat> <eDrxValue>
  *     app runProc lptUserTest --exe=lptUserTest -- getEDrxParams <rat>
  *
  * Examples:
  *     Enable eDRX for LTE M1:
  *         app runProc lptUserTest --exe=lptUserTest -- setEDrxState 4 on
  *     Set eDRX value for LTE NB1:
  *         app runProc lptUserTest --exe=lptUserTest -- setEDrxParams 5 7
  *     Get eDRX parameters for UMTS:
  *         app runProc lptUserTest --exe=lptUserTest -- getEDrxParams 3
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Base 10 definition for strtoul.
 */
//--------------------------------------------------------------------------------------------------
#define BASE10  10


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Print help.
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    printf("\nUsage of lptUserTest application:\n"
           "\tEnable or disable the eDRX feature for the given RAT:\n"
           "\t\tapp runProc lptUserTest --exe=lptUserTest -- setEDrxState <rat> <on|off>\n"
           "\tSet the requested eDRX value for the given RAT:\n"
           "\t\tapp runProc lptUserTest --exe=lptUserTest -- setEDrxParams <rat> <eDrxValue>\n"
           "\tRetrieve the eDRX parameters (requested eDRX value, network-provided eDRX value and"
           "\tPaging Time Window) for the given RAT:\n"
           "\t\tapp runProc lptUserTest --exe=lptUserTest -- getEDrxParams <rat>\n\n"
           "<rat> can take the following values:\n"
           "\t- 1: EC-GSM-IoT (A/Gb mode)\n"
           "\t- 2: GSM (A/Gb mode)\n"
           "\t- 3: UTRAN (Iu mode)\n"
           "\t- 4: E-UTRAN (WB-S1 mode)\n"
           "\t- 5: E-UTRAN (NB-S1 mode)\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Radio Access Technology to string.
 *
 * @return RAT string.
 */
//--------------------------------------------------------------------------------------------------
static const char* ConvertRat
(
    le_lpt_EDrxRat_t rat    ///< [IN] Radio Access Technology.
)
{
    const char* str;

    switch (rat)
    {
        case LE_LPT_EDRX_RAT_EC_GSM_IOT:
            str = "EC-GSM-IoT (A/Gb mode)";
            break;

        case LE_LPT_EDRX_RAT_GSM:
            str = "GSM (A/Gb mode)";
            break;

        case LE_LPT_EDRX_RAT_UTRAN:
            str = "UTRAN (Iu mode)";
            break;

        case LE_LPT_EDRX_RAT_LTE_M1:
            str = "E-UTRAN (WB-S1 mode)";
            break;

        case LE_LPT_EDRX_RAT_LTE_NB1:
            str = "E-UTRAN (NB-S1 mode)";
            break;

        case LE_LPT_EDRX_RAT_UNKNOWN:
        default:
            str = "unknown";
            break;
    }

    return str;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check number of arguments and display an error if the number is not correct.
 *
 * @return
 *  - LE_OK     Correct number of arguments.
 *  - LE_FAULT  Incorrect number of arguments.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckArgNumber
(
    int actualArgNb,        ///< [IN] Actual number of arguments.
    int expectedArgNb       ///< [IN] Expected number of arguments.
)
{
    if (actualArgNb != expectedArgNb)
    {
        printf("Incorrect number of arguments: %d but %d expected\n", actualArgNb, expectedArgNb);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int numArgs = le_arg_NumArgs();
    const char* actionStr = le_arg_GetArg(0);
    const char* param1Str;
    const char* param2Str;
    le_lpt_EDrxRat_t rat;
    uint8_t eDrxValue;
    uint8_t ptw;
    le_result_t result;

    if ((!numArgs) || (!actionStr))
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    if (0 == strcmp(actionStr, "setEDrxState"))
    {
        if (LE_OK != CheckArgNumber(numArgs, 3))
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Set eDRX state
        param1Str = le_arg_GetArg(1);
        param2Str = le_arg_GetArg(2);
        if ((!param1Str) || (!param2Str))
        {
           PrintUsage();
           exit(EXIT_FAILURE);
        }
        rat = strtoul(param1Str, NULL, BASE10);

        if (0 == strcmp("on", param2Str))
        {
            result = le_lpt_SetEDrxState(rat, LE_ON);
            if (LE_OK != result)
            {
                printf("Failed to enable eDRX for RAT %s (%s)\n",
                       ConvertRat(rat), LE_RESULT_TXT(result));
            }
            else
            {
                printf("Successfully enabled eDRX for RAT %s\n", ConvertRat(rat));
            }
        }
        else if (0 == strcmp("off", param2Str))
        {
            result = le_lpt_SetEDrxState(rat, LE_OFF);
            if (LE_OK != result)
            {
                printf("Failed to disable eDRX for RAT %s (%s)\n",
                       ConvertRat(rat), LE_RESULT_TXT(result));
            }
            else
            {
                printf("Successfully disabled eDRX for RAT %s\n", ConvertRat(rat));
            }
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }
    else if (0 == strcmp(actionStr, "setEDrxParams"))
    {
        if (LE_OK != CheckArgNumber(numArgs, 3))
        {
            PrintUsage();
        }

        // Set eDRX parameters
        param1Str = le_arg_GetArg(1);
        param2Str = le_arg_GetArg(2);
        if ((!param1Str) || (!param2Str))
        {
           PrintUsage();
           exit(EXIT_FAILURE);
        }
        rat = strtoul(param1Str, NULL, BASE10);
        eDrxValue = strtoul(param2Str, NULL, BASE10);

        result = le_lpt_SetRequestedEDrxValue(rat, eDrxValue);
        if (LE_OK != result)
        {
            printf("Failed to set requested eDRX value %d for RAT %s (%s)\n",
                   eDrxValue, ConvertRat(rat), LE_RESULT_TXT(result));
        }
        else
        {
            printf("Successfully set requested eDRX value %d for RAT %s\n",
                   eDrxValue, ConvertRat(rat));
        }
    }
    else if (0 == strcmp(actionStr, "getEDrxParams"))
    {
        if (LE_OK != CheckArgNumber(numArgs, 2))
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Get eDRX parameters
        param1Str = le_arg_GetArg(1);
        if (!param1Str)
        {
           PrintUsage();
           exit(EXIT_FAILURE);
        }
        rat = strtoul(param1Str, NULL, BASE10);

        printf("eDRX parameters for RAT %s:\n", ConvertRat(rat));
        result = le_lpt_GetRequestedEDrxValue(rat, &eDrxValue);
        if (LE_OK != result)
        {
            printf("Failed to get requested eDRX value (%s)\n", LE_RESULT_TXT(result));
        }
        else
        {
            printf("Requested eDRX value: %d\n", eDrxValue);
        }
        result = le_lpt_GetNetworkProvidedEDrxValue(rat, &eDrxValue);
        if (LE_OK != result)
        {
            printf("Failed to get network-provided eDRX value (%s)\n", LE_RESULT_TXT(result));
        }
        else
        {
            printf("Network-provided eDRX value: %d\n", eDrxValue);
        }
        result = le_lpt_GetNetworkProvidedPagingTimeWindow(rat, &ptw);
        if (LE_OK != result)
        {
            printf("Failed to get network-provided PTW (%s)\n", LE_RESULT_TXT(result));
        }
        else
        {
            printf("Network-provided PTW: %d\n", ptw);
        }
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
