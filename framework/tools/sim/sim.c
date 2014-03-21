// -------------------------------------------------------------------------------------------------
/**
 *  Utility to work with sim from the command line.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_sim.h"
#include "le_cfg_interface.h"

#define CFG_MODEMSERVICE_SIM_PATH "/modemServices/sim"
#define CFG_NODE_PIN              "pin"

static uint32_t SimSlot = 1;

// -------------------------------------------------------------------------------------------------
/**
 *  Print the help text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void HelpText
(
    void
)
{
    printf("Usage:\n\n"
            "To get sim status:\n"
            "\tsim\n\n"
            "To enter pin code:\n"
            "\tsim enterpin <pin>\n\n"
            "To change pin code:\n"
            "\tsim changepin <oldpin> <newpin>\n\n"
            "To lock sim:\n"
            "\tsim lock <pin>\n\n"
            "To unlock sim:\n"
            "\tsim unlock <pin>\n\n"
            "To unblock sim:\n"
            "\tsim unblock <puk> <newpin>\n\n"
            "To store pin:\n"
            "\tsim storepin <pin>\n\n"
            "Enter PIN: Enters the PIN code that is required before any Mobile equipment functionality can be used.\n"
            "Change PIN: Change the PIN code of the SIM card.\n"
            "Lock: Enable security of the SIM card, it will request for a PIN code upon insertion.\n"
            "Unlock: Disable security of the SIM card, it won't request a PIN code upon insertion (unsafe).\n"
            "Unblock: Unblocks the SIM card. The SIM card is blocked after X unsuccessful attempts to enter the PIN.\n\n"
            "Whether security is enabled or not, the SIM card has a PIN code that must be entered for every operations.\n"
            "Only ways to change this PIN code are through 'changepin' and 'unblock' operations.\n\n"
            );
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function returns a SIM reference, and exit application on failure.
 */
// -------------------------------------------------------------------------------------------------
static le_sim_Ref_t GetSimRef
(
    uint32_t simSlot
)
{
    le_sim_Ref_t simRef = NULL;

    if(SimSlot != 1)
    {
        printf("SIM slot: %d\n", SimSlot);
    }

    simRef = le_sim_Create(SimSlot);

    if(!simRef)
    {
        fprintf(stderr, "Invalid Slot (%u)\n", simSlot);
        exit(EXIT_FAILURE);
    }

    return simRef;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to get the SIM state.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int GetSimStatus()
{
    le_sim_Ref_t simRef = NULL;
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;
    int exitValue = EXIT_SUCCESS;

    simRef = GetSimRef(SimSlot);

    state = le_sim_GetState(simRef);

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
        default:
            printf("Unknown SIM state.\n");
            break;
    }

    le_sim_Delete(simRef);

    return exitValue;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will enter the pin code for the sim.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int EnterPin
(
    const char * pin
)
{
    le_sim_Ref_t simRef = NULL;
    le_result_t res = LE_OK;

    int exitValue = EXIT_FAILURE;

    simRef = GetSimRef(SimSlot);

    res = le_sim_EnterPIN(simRef, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            exitValue = EXIT_SUCCESS;
            break;
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
            printf("Remaining PIN tries: %d\n", le_sim_GetRemainingPINTries(simRef));
            break;
    }

    le_sim_Delete(simRef);

    return exitValue;
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will change the pin code for the sim.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int ChangePin
(
    const char * oldPinPtr,
    const char * newPinPtr
)
{
    le_sim_Ref_t simRef = NULL;
    le_result_t res = LE_OK;

    int exitValue = EXIT_FAILURE;

    simRef = GetSimRef(SimSlot);

    res = le_sim_ChangePIN(simRef, oldPinPtr, newPinPtr);

    switch(res)
    {
        case LE_OK:
            printf("Success.\n");
            exitValue = EXIT_SUCCESS;
            break;
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

    return exitValue;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will lock the sim.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int LockSim
(
    const char * pin
)
{
    le_sim_Ref_t simRef = NULL;
    le_result_t res = LE_OK;

    int exitValue = EXIT_FAILURE;

    simRef = GetSimRef(SimSlot);

    res = le_sim_Lock(simRef, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            exitValue = EXIT_SUCCESS;
            break;
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

    le_sim_Delete(simRef);

    return exitValue;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will unlock the sim.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int UnlockSim
(
    const char * pin
)
{
    le_sim_Ref_t simRef = NULL;
    le_result_t res = LE_OK;

    int exitValue = EXIT_FAILURE;

    simRef = GetSimRef(SimSlot);

    res = le_sim_Unlock(simRef, pin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            exitValue = EXIT_SUCCESS;
            break;
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
            printf("Remaining PIN tries: %d\n", le_sim_GetRemainingPINTries(simRef));
            break;
    }

    le_sim_Delete(simRef);

    return exitValue;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will unblock the sim.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int UnblockSim
(
    const char * puk,
    const char * newpin
)
{
    le_sim_Ref_t simRef = NULL;
    le_result_t res = LE_OK;

    int exitValue = EXIT_FAILURE;

    simRef = GetSimRef(SimSlot);

    res = le_sim_Unblock(simRef, puk, newpin);

    switch (res)
    {
        case LE_OK:
            printf("Success.\n");
            exitValue = EXIT_SUCCESS;
            break;
        case LE_NOT_FOUND:
            printf("Failed to select the SIM card for this operation.\n");
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
            printf("Error: %s\n", LE_RESULT_TXT(res));
            break;
    }

    le_sim_Delete(simRef);

    return exitValue;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will store the pin in the configdb.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int StorePin
(
    const char * pin
)
{
    int exitValue = EXIT_SUCCESS;
    char configPath[512];
    le_result_t res;

    snprintf(configPath, sizeof(configPath), "%s/%d", CFG_MODEMSERVICE_SIM_PATH, SimSlot);

    le_cfg_iteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(configPath);
    le_cfg_SetString(iteratorRef, CFG_NODE_PIN, pin);
    res = le_cfg_CommitWrite(iteratorRef);

    if (res != LE_OK)
    {
        exitValue = EXIT_FAILURE;
        printf("Failed to store pin. Error %s\n", LE_RESULT_TXT(res));
    }

    le_cfg_DeleteIterator(iteratorRef);

    return exitValue;
}


// -------------------------------------------------------------------------------------------------
/**
 *  Program init
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Make sure that our connection to the config tree is initialized.
    le_cfg_Initialize();

    int i = 0;
    char command[256] = "";

    // No arguments
    if (le_arg_NumArgs() == 0)
    {
        exit(GetSimStatus());
    }

    /* Options */
    for (i = 0; i < le_arg_NumArgs(); i++)
    {
        le_arg_GetArg(i, command, sizeof(command));

        if( command[0] != '-' )
        {
            break;
        }

        /* Slot slot */
        if (strcmp(command, "-s") == 0)
        {
            if ( (i+2) <= le_arg_NumArgs())
            {
                le_arg_GetArg(i+1, command, sizeof(command));
                SimSlot = atoi(command);

                if ((SimSlot == 0) || (SimSlot > le_sim_CountSlots()))
                {
                    printf("Invalid SIM slots, please try again.\n");
                    exit(EXIT_FAILURE);
                }

                printf("SIM slot: %d\n", SimSlot);

                // if its just sim -s [slot]
                if (le_arg_NumArgs() == 2)
                {
                    exit(GetSimStatus());
                }

                break;
            }
        }
    }

    /* Commands */
    for (; i < le_arg_NumArgs(); i++)
    {
        le_arg_GetArg(i, command, sizeof(command));

        if (strcmp(command, "help") == 0)
        {
            HelpText();
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(command, "enterpin") == 0)
        {
            if (i + 2 <= le_arg_NumArgs())
            {
                char pin[100] = "";
                le_arg_GetArg(i+1, pin, sizeof(pin));
                exit(EnterPin(pin));
            }
            else
            {
                printf("PIN code missing. e.g. sim enterpin <pin>\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(command, "changepin") == 0)
        {
            if (i + 3 <= le_arg_NumArgs())
            {
                char oldPin[10] = "";
                char newPin[10] = "";

                le_arg_GetArg(i+1, oldPin, sizeof(oldPin));
                le_arg_GetArg(i+2, newPin, sizeof(newPin));

                exit(ChangePin(oldPin, newPin));
            }
            else
            {
                printf("PIN code missing. e.g. sim enterpin <pin>\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(command, "lock") == 0)
        {
            if (i + 2 <= le_arg_NumArgs())
            {
                char pin[100] = "";
                le_arg_GetArg(i+1, pin, sizeof(pin));
                exit(LockSim(pin));
            }
            else
            {
                printf("PIN code missing. e.g. sim lock <pin>\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(command, "unlock") == 0)
        {
            if (i + 2 <= le_arg_NumArgs())
            {
                char pin[100] = "";
                le_arg_GetArg(i+1, pin, sizeof(pin));
                exit(UnlockSim(pin));
            }
            else
            {
                printf("PIN code missing. e.g. sim unlock <pin>\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(command, "unblock") == 0)
        {
            if (i + 3 <= le_arg_NumArgs())
            {
                char puk[100] = "";
                char newpin[100] = "";
                le_arg_GetArg(i+1, puk, sizeof(puk));
                le_arg_GetArg(i+2, newpin, sizeof(newpin));
                exit(UnblockSim(puk, newpin));
            }
            else
            {
                printf("PUK/PIN code missing. e.g. sim unblock <puk> <newpin>\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(command, "storepin") == 0)
        {
            if (i + 2 <= le_arg_NumArgs())
            {
                char pin[100] = "";
                le_arg_GetArg(i+1, pin, sizeof(pin));
                exit(StorePin(pin));
            }
            else
            {
                printf("PIN code missing. e.g. sim storepin <pin>\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // if none of the conditions have been met, it means they entered an invalid command
    printf("Invalid command. Please try again.\n");
    HelpText();
    exit(EXIT_FAILURE);
}
