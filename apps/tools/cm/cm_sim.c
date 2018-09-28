//-------------------------------------------------------------------------------------------------
/**
 * @file cm_sim.c
 *
 * Handle sim related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_sim.h"
#include "cm_common.h"

static le_sim_Id_t SimId;

//-------------------------------------------------------------------------------------------------
/**
 * Print the help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_sim_PrintSimHelp
(
    void
)
{
    printf("SIM usage\n"
            "=========\n\n"
            "To get sim status:\n"
            "\tcm sim\n"
            "\tcm sim status\n\n"
            "To get sim information:\n"
            "\tcm sim info\n\n"
            "To get the SIM IMSI (International Mobile Subscriber Identity):\n"
            "\tcm sim imsi\n\n"
            "To get the SIM ICCID (integrated circuit card identifier):\n"
            "\tcm sim iccid\n\n"
            "To get the SIM EID (identifier for the embedded Universal Integrated Circuit Card):\n"
            "\tcm sim eid\n\n"
            "To get the sim phone number:\n"
            "\tcm sim number\n\n"
            "To enter pin code:\n"
            "\tcm sim enterpin <pin>\n\n"
            "To change pin code:\n"
            "\tcm sim changepin <oldpin> <newpin>\n\n"
            "To lock sim:\n"
            "\tcm sim lock <pin>\n\n"
            "To unlock sim:\n"
            "\tcm sim unlock <pin>\n\n"
            "To unblock sim:\n"
            "\tcm sim unblock <puk> <newpin>\n\n"
            "To store pin:\n"
            "\tcm sim storepin <pin>\n\n"
            "To select SIM:\n"
            "\tcm sim select <EMBEDDED | EXTERNAL_SLOT_1 | EXTERNAL_SLOT_2 | REMOTE>\n\n"
            "To use auto SIM selection:\n"
            "\tcm sim mode <AUTO | MANUAL> \n\n"
            "Enter PIN: Enters the PIN code that is required before any Mobile equipment "
            "functionality can be used.\n"
            "Change PIN: Change the PIN code of the SIM card.\n"
            "Lock: Enable security of the SIM card, it will request for a PIN code upon insertion."
            "\n"
            "Unlock: Disable security of the SIM card, it won't request a PIN code upon insertion "
            "(unsafe).\n"
            "Unblock: Unblocks the SIM card. The SIM card is blocked after X unsuccessful attempts "
            "to enter the PIN.\n\n"
            "Whether security is enabled or not, the SIM card has a PIN code that must be entered "
            "for every operations.\n"
            "Only ways to change this PIN code are through 'changepin' and 'unblock' operations."
            "\n\n"
            );
}

//-------------------------------------------------------------------------------------------------
/**
 * Structure to hold an enum <=> string relation for @ref le_sim_Id_t.
 */
//-------------------------------------------------------------------------------------------------
typedef struct {
    le_sim_Id_t simId;
    const char * strPtr;
}
SimIdStringAssoc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Array containing all enum <=> string relations for @ref le_sim_Id_t.
 */
//-------------------------------------------------------------------------------------------------
static const SimIdStringAssoc_t SimIdStringAssocs[] = {
    { LE_SIM_EMBEDDED,          "EMBEDDED" },
    { LE_SIM_EXTERNAL_SLOT_1,   "EXTERNAL_SLOT_1" },
    { LE_SIM_EXTERNAL_SLOT_2,   "EXTERNAL_SLOT_2" },
    { LE_SIM_REMOTE,            "REMOTE" },
};

//-------------------------------------------------------------------------------------------------
/**
 * This function converts a le_sim_Id_t to a string.
 *
 * @return Type as a string
 */
//-------------------------------------------------------------------------------------------------
static const char * SimIdToString
(
    le_sim_Id_t x
)
{
    int i;

    for(i = 0; i < NUM_ARRAY_MEMBERS(SimIdStringAssocs); i++)
    {
        if (x == SimIdStringAssocs[i].simId)
        {
            return SimIdStringAssocs[i].strPtr;
        }
    }

    LE_FATAL("Unknown value for enum le_sim_Id_t: %d", (int)x);

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function converts a string to a le_sim_Id_t.
 *
 * @return Type as an enum
 */
//-------------------------------------------------------------------------------------------------
static le_sim_Id_t SimIdFromString
(
    const char * strPtr
)
{
    int i;

    for(i = 0; i < NUM_ARRAY_MEMBERS(SimIdStringAssocs); i++)
    {
        if (0 == strcmp(SimIdStringAssocs[i].strPtr, strPtr))
        {
            return SimIdStringAssocs[i].simId;
        }
    }

    LE_WARN("Unable to convert '%s' to a le_sim_Id_t", strPtr);

    return LE_SIM_ID_MAX;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM state.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimStatus()
{
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;

    state = le_sim_GetState(SimId);

    switch (state)
    {
        case LE_SIM_INSERTED:
            printf("SIM card is inserted and locked (LE_SIM_INSERTED).\n");
            break;
        case LE_SIM_ABSENT:
            printf("SIM card is absent (LE_SIM_ABSENT).\n");
            break;
        case LE_SIM_READY:
            printf("SIM card is inserted and unlocked (LE_SIM_READY).\n");
            break;
        case LE_SIM_BLOCKED:
            printf("SIM card is blocked (LE_SIM_BLOCKED).\n");
            break;
        case LE_SIM_BUSY:
            printf("SIM card is busy (LE_SIM_BUSY).\n");
            break;
        case LE_SIM_POWER_DOWN:
            printf("SIM card is powered down (LE_SIM_POWER_DOWN).\n");
            break;
        default:
            printf("Unknown SIM state.\n");
            break;
    }

    printf("\n");

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the home network name.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetNetworkOperator
(
    void
)
{
    char homeNetwork[100];
    le_result_t res;
    int ret = EXIT_SUCCESS;

    res = le_sim_GetHomeNetworkOperator(SimId, homeNetwork, sizeof(homeNetwork));

    if (res != LE_OK)
    {
        homeNetwork[0] = '\0';
        ret = EXIT_FAILURE;
    }

    cm_cmn_FormatPrint("Home Network Operator", homeNetwork);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM IMSI.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimImsi
(
    void
)
{
    char imsi[LE_SIM_IMSI_BYTES];
    le_result_t res;
    int ret = EXIT_SUCCESS;

    res = le_sim_GetIMSI(SimId, imsi, sizeof(imsi));

    if (res != LE_OK)
    {
        imsi[0] = '\0';
        ret = EXIT_FAILURE;
    }

    cm_cmn_FormatPrint("IMSI", imsi);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM ICCID.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimIccid
(
    void
)
{
    char iccid[LE_SIM_ICCID_BYTES];
    le_result_t res;
    int ret = EXIT_SUCCESS;

    res = le_sim_GetICCID(SimId, iccid, sizeof(iccid));

    if (res != LE_OK)
    {
        iccid[0] = '\0';
        ret = EXIT_FAILURE;
    }

    cm_cmn_FormatPrint("ICCID", iccid);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM EID.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimEid
(
    void
)
{
    char eid[LE_SIM_EID_BYTES];
    le_result_t res;
    int ret = EXIT_SUCCESS;

    res = le_sim_GetEID(SimId, eid, sizeof(eid));

    if (LE_OK != res)
    {
        eid[0] = '\0';
        ret = EXIT_FAILURE;
    }

    cm_cmn_FormatPrint("EID", eid);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM phone number.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimPhoneNumber
(
    void
)
{
    char number[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    le_result_t res;
    int ret = EXIT_SUCCESS;

    res = le_sim_GetSubscriberPhoneNumber(SimId, number, sizeof(number));

    if (res != LE_OK)
    {
        number[0] = '\0';
        ret = EXIT_FAILURE;
    }

    cm_cmn_FormatPrint("Phone Number", number);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM card type.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetCardType
(
    void
)
{
    cm_cmn_FormatPrint("Type", SimIdToString(SimId));
    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the SIM info (Home PLMN,...).
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetSimInfo
(
    void
)
{
    int ret = EXIT_SUCCESS;

    cm_sim_GetCardType();

    if (EXIT_SUCCESS != cm_sim_GetSimIccid())
    {
        ret = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS != cm_sim_GetNetworkOperator())
    {
        ret = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS != cm_sim_GetSimEid())
    {
        ret = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS != cm_sim_GetSimImsi())
    {
        ret = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS != cm_sim_GetSimPhoneNumber())
    {
        ret = EXIT_FAILURE;
    }

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will enter the pin code for the sim.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_EnterPin
(
    const char * pin    ///< [IN] PIN code
)
{
    le_result_t     res = LE_OK;

    res = le_sim_EnterPIN(SimId, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            return EXIT_SUCCESS;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
            break;
        case LE_OVERFLOW:
            printf("The PIN code is too long (max 8 digits).\n");
            break;
        case LE_UNDERFLOW:
            printf("The PIN code is not long enough (min 4 digits).\n");
            break;
        default:
            printf("Error: %s\n", LE_RESULT_TXT(res));
            printf("Remaining PIN tries: %d\n", le_sim_GetRemainingPINTries(SimId));
            break;
    }

    return EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will change the pin code for the sim.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_ChangePin
(
    const char * oldPinPtr,     ///< [IN] Old PIN code
    const char * newPinPtr      ///< [IN] New PIN code
)
{
    le_result_t     res = LE_OK;

    res = le_sim_ChangePIN(SimId, oldPinPtr, newPinPtr);

    switch(res)
    {
        case LE_OK:
            printf("Success.\n");
            return EXIT_SUCCESS;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
            break;
        case LE_OVERFLOW:
            printf("The PIN code is too long (max 8 digits).\n");
            break;
        case LE_UNDERFLOW:
            printf("The PIN code is not long enough (min 4 digits).\n");
            break;
        default:
            printf("Error: %s\n", LE_RESULT_TXT(res));
            break;
    }

    return EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will lock the sim.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_LockSim
(
    const char * pin    ///< [IN] PIN code
)
{
    le_result_t     res = LE_OK;

    res = le_sim_Lock(SimId, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            return EXIT_SUCCESS;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
            break;
        case LE_OVERFLOW:
            printf("The PIN code is too long (max 8 digits).\n");
            break;
        case LE_UNDERFLOW:
            printf("The PIN code is not long enough (min 4 digits).\n");
            break;
        default:
            printf("Error: %s\n", LE_RESULT_TXT(res));
            break;
    }

    return EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will unlock the sim.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_UnlockSim
(
    const char * pin    ///< [IN] PIN code
)
{
    le_result_t     res = LE_OK;

    res = le_sim_Unlock(SimId, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            return EXIT_SUCCESS;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
            break;
        case LE_OVERFLOW:
            printf("The PIN code is too long (max 8 digits).\n");
            break;
        case LE_UNDERFLOW:
            printf("The PIN code is not long enough (min 4 digits).\n");
            break;
        default:
            printf("Error: %s\n", LE_RESULT_TXT(res));
            printf("Remaining PIN tries: %d\n", le_sim_GetRemainingPINTries(SimId));
            break;
    }

    return EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will unblock the sim.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_UnblockSim
(
    const char * pukPtr,       ///< [IN] PUK code
    const char * newPinPtr     ///< [IN] New PIN code
)
{
    le_result_t     res = LE_OK;

    res = le_sim_Unblock(SimId, pukPtr, newPinPtr);
    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            return EXIT_SUCCESS;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Invalid SIM Identifier.\n");
            break;
        case LE_OVERFLOW:
            printf("The PIN code is too long (max 8 digits).\n");
            break;
        case LE_UNDERFLOW:
            printf("The PIN code is not long enough (min 4 digits).\n");
            break;
        case LE_OUT_OF_RANGE:
            printf("The PUK code length is not correct (8 digits).\n");
            break;
        default:
        {
            uint32_t remainingPukTries = 0;

            printf("Error: %s\n", LE_RESULT_TXT(res));
            res = le_sim_GetRemainingPUKTries(SimId, &remainingPukTries);

            if (LE_OK == res)
            {
                printf("Remaining PUK tries: %d\n", remainingPukTries);
            }
            else
            {
                printf("Failed to get the remaining PUK tries: error = %d\n", res);
            }
            break;
        }
    }

    return EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function will store the pin in the secure storage.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_StorePin
(
    const char * pin    ///< [IN] PIN code
)
{
    le_result_t result = le_cellnet_SetSimPinCode(SimId, pin);

    if (LE_OK == result)
    {
        printf("PIN code successfully stored!\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf("Unable to store PIN code, error %s\n", LE_RESULT_TXT(result));
        return EXIT_FAILURE;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will select the SIM.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_Select
(
    const char * typeStr  ///< [IN] SIM type as a string
)
{
    le_sim_Id_t simId;
    le_result_t res;

    simId = SimIdFromString(typeStr);

    if (simId >= LE_SIM_ID_MAX)
    {
        printf("'%s' is not a valid SIM type.\n", typeStr);
        return EXIT_FAILURE;
    }

    res = le_sim_SelectCard(simId);
    if (res != LE_OK)
    {
        printf("Unable to select '%s'.\n", typeStr);
        return EXIT_FAILURE;
    }
    SimId = simId;

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will change the SIM selection mode.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_SetMode
(
    const char * modeStr  ///< [IN] SIM mode as a string
)
{
    bool enable = false;

    if (!modeStr)
    {
        LE_ERROR("modeStr is NULL");
        return EXIT_FAILURE;
    }

    if (0 == strcmp(modeStr, "AUTO"))
    {
        enable = true;

    }
    else if (0 == strcmp(modeStr, "MANUAL"))
    {
        enable = false;
    }
    else
    {
        printf("Wrong <type> argument\n");
        return EXIT_FAILURE;
    }

    if (le_sim_SetAutomaticSelection(enable) != LE_OK)
    {
        printf("Unable to set automatic selection mode to %d\n", enable);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will return the current SIM selection mode.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_sim_GetMode
(
    void
)
{
    bool enable = false;

    if (le_sim_GetAutomaticSelection(&enable) != LE_OK)
    {
        printf("Unable to get automatic selection mode\n");
        return EXIT_FAILURE;
    }

    printf("SIM selection mode: %s\n", enable ? "AUTO" : "MANUAL");

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for sim service.
 */
//--------------------------------------------------------------------------------------------------
void cm_sim_ProcessSimCommand
(
    const char * command,   ///< [IN] Sim commands
    size_t numArgs          ///< [IN] Number of arguments
)
{
    SimId = le_sim_GetSelectedCard();
    const char* pinPtr = le_arg_GetArg(2);
    if ((numArgs > 2) && (NULL == pinPtr))
    {
        LE_ERROR("pinPtr is NULL");
        exit(EXIT_FAILURE);
    }

    if (strcmp(command, "help") == 0)
    {
        cm_sim_PrintSimHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "status") == 0)
    {
        exit(cm_sim_GetSimStatus());
    }
    else if (strcmp(command, "enterpin") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "PIN code missing. e.g. cm sim enterpin <pin>"))
        {
            exit(cm_sim_EnterPin(pinPtr));
        }
    }
    else if (strcmp(command, "changepin") == 0)
    {
        if (cm_cmn_CheckEnoughParams(2, numArgs, "PIN code missing. e.g. cm sim changepin <pin>"))
        {
            const char* newPinPtr = le_arg_GetArg(3);
            if (NULL == newPinPtr)
            {
                LE_ERROR("newPinPtr is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_sim_ChangePin(pinPtr, newPinPtr));
        }
    }
    else if (strcmp(command, "lock") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "PIN code missing. e.g. cm sim lock <pin>"))
        {
            exit(cm_sim_LockSim(pinPtr));
        }
    }
    else if (strcmp(command, "unlock") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "PIN code missing. e.g. cm sim unlock <pin>"))
        {
            exit(cm_sim_UnlockSim(pinPtr));
        }
    }
    else if (strcmp(command, "unblock") == 0)
    {
        if (cm_cmn_CheckEnoughParams(2,
                                     numArgs,
                                     "PUK/PIN code missing. e.g. cm sim unblock <puk> <newpin>"))
        {
            const char* newPinPtr = le_arg_GetArg(3);
            if (NULL == newPinPtr)
            {
                LE_ERROR("newPinPtr is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_sim_UnblockSim(pinPtr, newPinPtr));
        }
    }
    else if (strcmp(command, "storepin") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "PIN code missing. e.g. cm sim storepin <pin>"))
        {
            exit(cm_sim_StorePin(pinPtr));
        }
    }
    else if (strcmp(command, "info") == 0)
    {
        exit(cm_sim_GetSimInfo());
    }
    else if (strcmp(command, "iccid") == 0)
    {
        exit(cm_sim_GetSimIccid());
    }
    else if (strcmp(command, "eid") == 0)
    {
        exit(cm_sim_GetSimEid());
    }
    else if (strcmp(command, "imsi") == 0)
    {
        exit(cm_sim_GetSimImsi());
    }
    else if (strcmp(command, "number") == 0)
    {
        exit(cm_sim_GetSimPhoneNumber());
    }
    else if (strcmp(command, "select") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "SIM type missing. e.g. cm sim select <type>"))
        {
            exit(cm_sim_Select(pinPtr));
        }
    }
    else if (strcmp(command, "mode") == 0)
    {
        if (numArgs == 3)
        {
            exit(cm_sim_SetMode(pinPtr));
        }
        else
        {
            exit(cm_sim_GetMode());
        }
    }
    else
    {
        printf("Invalid command for SIM service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

