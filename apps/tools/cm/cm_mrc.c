//-------------------------------------------------------------------------------------------------
/**
 * @file cm_radio.c
 *
 * Handle radio control related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_mrc.h"
#include "cm_common.h"


//-------------------------------------------------------------------------------------------------
/**
 * Print the radio help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_mrc_PrintRadioHelp
(
    void
)
{
    printf("Radio usage\n"
            "===========\n\n"
            "To get modem status:\n"
            "\tcm radio\n"
            "\tcm radio status\n\n"
            "To enable/disable radio:\n"
            "\tcm radio <on/off>\n\n"
            "To set radio access technologies prefererences\n"
            "\tcm radio rat <[CDMA] [GSM] [UMTS] [LTE] [TDSCDMA]>\n\n"
            "To get radio access technologies prefererences\n"
            "\tcm radio getRAT \n\n"
            "To resume automatic RAT selection.\n"
            "\tcm radio rat AUTO\n\n"
            );
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the current network name.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetCurrentNetworkName
(
    void
)
{
    char homeNetwork[CMODEM_COMMON_NETWORK_STR_LEN];
    le_result_t res;

    res = le_mrc_GetCurrentNetworkName(homeNetwork, sizeof(homeNetwork));

    if (res != LE_OK)
    {
        cm_cmn_FormatPrint("Current Network Operator", "");
        return res;
    }

    cm_cmn_FormatPrint("Current Network Operator", homeNetwork);

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to check if the radio is powered on.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetRadioPower
(
    void
)
{
    le_result_t res;
    le_onoff_t state;

    res = le_mrc_GetRadioPower(&state);

    if (res != LE_OK)
    {
        return res;
    }

    switch (state)
    {
        case LE_OFF:
            cm_cmn_FormatPrint("Power", "OFF");
            break;
        case LE_ON:
            cm_cmn_FormatPrint("Power", "ON");
            break;
        default:
            return LE_FAULT;
    }

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the network registration state.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetRegState
(
    void
)
{
    le_result_t res;
    le_mrc_NetRegState_t state;

    res = le_mrc_GetNetRegState(&state);

    if (res != LE_OK)
    {
        return res;
    }

    switch (state)
    {
        case LE_MRC_REG_NONE:
            cm_cmn_FormatPrint("Status", "Not registered and not currently searching for new operator (LE_MRC_REG_NONE)");
            break;
        case LE_MRC_REG_HOME:
            cm_cmn_FormatPrint("Status", "Registered, home network (LE_MRC_REG_HOME)");
            break;
        case LE_MRC_REG_SEARCHING:
            cm_cmn_FormatPrint("Status", "Not registered but currently searching for a new operator (LE_MRC_REG_SEARCHING)");
            break;
        case LE_MRC_REG_DENIED:
            cm_cmn_FormatPrint("Status", "Registration was denied, usually because of invalid access credentials (LE_MRC_REG_DENIED)");
            break;
        case LE_MRC_REG_ROAMING:
            cm_cmn_FormatPrint("Status", "Registered to a roaming network (LE_MRC_REG_ROAMING)");
            break;
        default:
            cm_cmn_FormatPrint("Status", "Unknown state (LE_MRC_REG_UNKNOWN)");
            break;
    }

    return res;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the signal quality.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetSignalQuality
(
    void
)
{
    le_result_t res;
    uint32_t signal;

    res = le_mrc_GetSignalQual(&signal);

    if (res != LE_OK)
    {
        return res;
    }

    switch (signal)
    {
        case 0:
            cm_cmn_FormatPrint("Signal", "No signal strength (0)");
            break;
        case 1:
            cm_cmn_FormatPrint("Signal", "Very weak signal strength (1)");
            break;
        case 2:
            cm_cmn_FormatPrint("Signal", "Weak signal strength (2)");
            break;
        case 3:
            cm_cmn_FormatPrint("Signal", "Good signal strength (3)");
            break;
        case 4:
            cm_cmn_FormatPrint("Signal", "Strong signal strength (4)");
            break;
        case 5:
            cm_cmn_FormatPrint("Signal", "Very strong signal strength (5)");
            break;
        default:
            cm_cmn_FormatPrint("Signal", "Unknown signal strength");
            break;
    }

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the radio access technology.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetCurrentRAT
(
    void
)
{
    le_result_t res;
    le_mrc_Rat_t rat;
    le_mrc_NetRegState_t state;

    res = le_mrc_GetNetRegState(&state);

    if ((LE_OK != res) ||
        ((LE_MRC_REG_HOME != state) && (LE_MRC_REG_ROAMING != state))
       )
    {
        cm_cmn_FormatPrint("Current RAT", "Module not registered on network, RAT not available");
        return LE_FAULT;
    }

    res = le_mrc_GetRadioAccessTechInUse(&rat);
    if (res != LE_OK)
    {
       cm_cmn_FormatPrint("Current RAT", "Unknown network (LE_MRC_RAT_UNKNOWN)");
       return res;
    }

    switch (rat)
    {
        case LE_MRC_RAT_GSM:
            cm_cmn_FormatPrint("Current RAT", "GSM network (LE_MRC_RAT_GSM)");
            break;
        case LE_MRC_RAT_UMTS:
            cm_cmn_FormatPrint("Current RAT", "UMTS network (LE_MRC_RAT_UMTS)");
            break;
        case LE_MRC_RAT_TDSCDMA:
            cm_cmn_FormatPrint("Current RAT", "TD-SCDMA network (LE_MRC_RAT_TDSCDMA)");
            break;
        case LE_MRC_RAT_LTE:
            cm_cmn_FormatPrint("Current RAT", "LTE network (LE_MRC_RAT_LTE)");
            break;
        case LE_MRC_RAT_CDMA:
            cm_cmn_FormatPrint("Current RAT", "CDMA network (LE_MRC_RAT_CDMA)");
            break;
        default:
            cm_cmn_FormatPrint("Current RAT", "Unknown network (LE_MRC_RAT_UNKNOWN)");
            break;
    }

    return res;
}



//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the Circuit and Packet Switched state.
 *
 * @return LE_OK if the call was successful.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetServicesState
(
    void
)
{
    le_result_t res;
    le_mrc_NetRegState_t serviceState;

    res = le_mrc_GetPacketSwitchedState(&serviceState);
    if (res != LE_OK)
    {
        return res;
    }

    switch (serviceState)
    {
        case LE_MRC_REG_NONE:
            cm_cmn_FormatPrint("PS", "Packet Switched Not registered (LE_MRC_REG_NONE)");
            break;
        case LE_MRC_REG_HOME:
            cm_cmn_FormatPrint("PS", "Packet Switched Registered, home network (LE_MRC_REG_HOME)");
            break;
        case LE_MRC_REG_ROAMING:
            cm_cmn_FormatPrint("PS", "Packet Switched Registered to a"\
                " roaming network (LE_MRC_REG_ROAMING)");
            break;
        default:
            cm_cmn_FormatPrint("PS", "Packet Switched Unknown state (LE_MRC_REG_UNKNOWN)");
            break;
    }

    return LE_OK;
}



//-------------------------------------------------------------------------------------------------
/**
 * This function sets the radio power.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_SetRadioPower
(
    le_onoff_t power    ///< [IN] Radio power switch
)
{
    le_result_t res;
    le_onoff_t currPower;

    res = le_mrc_GetRadioPower(&currPower);

    if (res != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Don't set if the radio power if it's the same
    if (currPower == power)
    {
        switch (currPower)
        {
            case LE_OFF:
                printf("Radio power is already set to OFF.\n");
                break;
            case LE_ON:
                printf("Radio power is already set to ON.\n");
                break;
            default:
                break;
        }
    }
    else
    {
        res = le_mrc_SetRadioPower(power);

        if (res != LE_OK)
        {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function returns modem status information to the user.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_GetModemStatus
(
    void
)
{
    le_result_t res;
    int exitStatus = EXIT_SUCCESS;

    res = GetRadioPower();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetCurrentNetworkName();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetCurrentRAT();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetRegState();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetSignalQuality();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetServicesState();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    printf("\n");
    return exitStatus;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function sets the radio access technology preferences.
 *
 * @return
 * - LE_OK    If the call was successful
 * - LE_FAULT Otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_SetRat
(
    le_mrc_RatBitMask_t rat ///< [IN] Radio access technology
)
{
    return le_mrc_SetRatPreferences(rat);
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the radio access technology preferences.
 *
 * @return
 * - LE_OK    If the call was successful
 * - LE_FAULT Otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_mrc_GetRat
(
    void
)
{
    le_mrc_RatBitMask_t rat;

    if (LE_OK != le_mrc_GetRatPreferences(&rat))
    {
        return LE_FAULT;
    }

    printf("Prefered RATs : ");
    if (rat & LE_MRC_BITMASK_RAT_GSM)
    {
        printf("GSM ");
    }

    if (rat & LE_MRC_BITMASK_RAT_UMTS)
    {
        printf("UMTS ");
    }

    if (rat & LE_MRC_BITMASK_RAT_TDSCDMA)
    {
        printf("TDSCDMA ");
    }

    if (rat & LE_MRC_BITMASK_RAT_LTE)
    {
        printf("LTE ");
    }

    if (rat & LE_MRC_BITMASK_RAT_CDMA)
    {
        printf("CDMA ");
    }

    if (LE_MRC_BITMASK_RAT_ALL == rat)
    {
        printf("AUTO ");
    }
    printf("\n");
    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for radio service.
 */
//--------------------------------------------------------------------------------------------------
void cm_mrc_ProcessRadioCommand
(
    const char * command,   ///< [IN] Radio command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    if (0 == strcmp(command, "help"))
    {
        cm_mrc_PrintRadioHelp();
        exit(EXIT_SUCCESS);
    }
    else if (0 == strcmp(command, "status"))
    {
        exit(cm_mrc_GetModemStatus());
    }
    else if (0 == strcmp(command, "on"))
    {
        exit(cm_mrc_SetRadioPower(LE_ON));
    }
    else if (0 == strcmp(command, "off"))
    {
        exit(cm_mrc_SetRadioPower(LE_OFF));
    }
    else if (0 == strcmp(command, "rat"))
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "RAT value missing. e.g. cm radio"
                        " rat <[CDMA] [GSM] [UMTS] [LTE] [TDSCDMA]> or <AUTO>"))
        {
            le_mrc_RatBitMask_t rat = 0;
            const char* ratStrPtr;
            int index;

            for (index = 2 ; index < numArgs ; index++)
            {
                ratStrPtr = le_arg_GetArg(index);
                LE_DEBUG("Args (%d) => '%s'",index, ratStrPtr);

                if (0 == strcmp(ratStrPtr, "AUTO"))
                {
                    if(cm_mrc_SetRat(LE_MRC_BITMASK_RAT_ALL) == LE_OK)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        LE_ERROR("Failed to set LE_MRC_BITMASK_RAT_ALL rat value");
                        printf("Failed to set LE_MRC_BITMASK_RAT_ALL rat value\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (0 == strcmp(ratStrPtr, "CDMA"))
                {
                    rat |= LE_MRC_BITMASK_RAT_CDMA;
                }
                else if (0 == strcmp(ratStrPtr, "GSM"))
                {
                    rat |= LE_MRC_BITMASK_RAT_GSM;
                }
                else if (0 == strcmp(ratStrPtr, "LTE"))
                {
                    rat |= LE_MRC_BITMASK_RAT_LTE;
                }
                else if (0 == strcmp(ratStrPtr, "UMTS"))
                {
                    rat |= LE_MRC_BITMASK_RAT_UMTS;
                }
                else if (0 == strcmp(ratStrPtr, "TDSCDMA"))
                {
                    rat |= LE_MRC_BITMASK_RAT_TDSCDMA;
                }
                else
                {
                    LE_ERROR("INVALID RAT option!!");
                    printf("INVALID RAT option!!\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (LE_OK == cm_mrc_SetRat(rat))
            {
                exit(EXIT_SUCCESS);
            }
            LE_ERROR("Failed to set rat value");
            printf("Failed to set rat value\n");
        }
        exit(EXIT_FAILURE);
    }
    else if (0 == strcmp(command, "getRAT"))
    {
        exit(cm_mrc_GetRat());
    }
    else
    {
        printf("Invalid command for radio service.\n");
        exit(EXIT_FAILURE);
    }
}
