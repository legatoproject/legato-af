
// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_radio.c
 *
 *  Handle radio control related functionality
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_mrc.h"
#include "cm_common.h"

#define CFG_MODEMSERVICE_MRC_PATH "/modemServices/radio"
#define CFG_NODE_PREF_RAT "preferences/rat"

// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the current network name.
*
*  @return LE_OK if the call was successful.
*/
// -------------------------------------------------------------------------------------------------
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

// -------------------------------------------------------------------------------------------------
/**
*  Print the radio help text to stdout.
*/
// -------------------------------------------------------------------------------------------------
void cm_mrc_PrintRadioHelp
(
    void
)
{
    printf("Radio usage\n"
            "===========\n\n"
            "To get modem status:\n"
            "\tcm radio\n\n"
            "To enable/disable radio:\n"
            "\tcm radio <on/off>\n\n"
            "To set radio access technology:\n"
            "\tcm radio rat <CDMA/GSM/UMTS/LTE> ...\n\n"
            "After setting the radio access technology, you will need to do a 'legato restart' for it take into effect.\n\n"
            "\tcm radio rat MANUAL\n\n"
            "To resume auto RAT selection.\n\n"
            );
}


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the network registration state.
*
*  @return LE_OK if the call was successful.
*/
// -------------------------------------------------------------------------------------------------
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


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the signal quality.
*
*  @return LE_OK if the call was successful.
*/
// -------------------------------------------------------------------------------------------------
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


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the radio access technology.
*
*  @return LE_OK if the call was successful.
*/
// -------------------------------------------------------------------------------------------------
static le_result_t GetRAT
(
    void
)
{
    le_result_t res;
    le_mrc_Rat_t rat;

    res = le_mrc_GetRadioAccessTechInUse(&rat);

    if (res != LE_OK)
    {
        return res;
    }

    switch (rat)
    {
        case LE_MRC_RAT_GSM:
            cm_cmn_FormatPrint("RAT", "GSM network (LE_MRC_RAT_GSM)");
            break;
        case LE_MRC_RAT_UMTS:
            cm_cmn_FormatPrint("RAT", "UMTS network (LE_MRC_RAT_UMTS)");
            break;
        case LE_MRC_RAT_LTE:
            cm_cmn_FormatPrint("RAT", "LTE network (LE_MRC_RAT_LTE)");
            break;
        case LE_MRC_RAT_CDMA:
            cm_cmn_FormatPrint("RAT", "CDMA network (LE_MRC_RAT_CDMA)");
            break;
        default:
            cm_cmn_FormatPrint("RAT", "Unknown network (LE_MRC_RAT_UNKNOWN)");
            break;
    }

    return res;
}





// -------------------------------------------------------------------------------------------------
/**
*  This function sets the radio power.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
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
            case 0:
                printf("Radio power is already set to OFF.\n");
                break;
            case 1:
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


// -------------------------------------------------------------------------------------------------
/**
*  This function returns modem status information to the user.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_mrc_GetModemStatus
(
    void
)
{
    le_result_t res;
    int exitStatus = EXIT_SUCCESS;

    res = GetCurrentNetworkName();

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }


    res = GetRAT();

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

    printf("\n");
    return exitStatus;
}


// -------------------------------------------------------------------------------------------------
/**
*   This function sets the radio access technology to use.
*
*   @todo Current tool will only support adding one RAT (0) to simplify scope.
*
*   @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_mrc_SetRat
(
    int8_t     index,      ///< [IN] RAT Index in config tree
    const char *ratPtr     ///< [IN] Radio access technology
)
{
    char ratToUpper[CMODEM_COMMON_RAT_STR_LEN];
    cm_cmn_toUpper(ratPtr, ratToUpper, sizeof(ratToUpper));

    char configPath[512];
    int pos = 0;

    pos = snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_MRC_PATH, CFG_NODE_PREF_RAT);

    if(index == -1)
    {
        le_cfg_QuickDeleteNode(configPath);
    }
    else
    {
        snprintf(configPath + pos, sizeof(configPath) - pos, "/%u", index);
        le_cfg_QuickSetString(configPath, ratToUpper);
    }


    return EXIT_SUCCESS;
}

