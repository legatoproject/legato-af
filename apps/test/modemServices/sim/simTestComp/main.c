 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <semaphore.h>

#include "main.h"

#define TEST_STRING_LEN 50

//--------------------------------------------------------------------------------------------------
/**
 * Print function.
 *
 */
//--------------------------------------------------------------------------------------------------
void Print(char* string)
{
    bool sandboxed = (getuid() != 0);

    if(sandboxed)
    {
        LE_INFO("%s", string);
    }
    else
    {
        fprintf(stderr, "%s\n", string);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    const char * usagePtr[] = {
            "Usage of the 'simTest' application is:",
            "SIM allocation test: simTest create <sim_select> <pin>",
            "SIM state test: simTest state <sim_select> <pin>",
            "SIM authentification test: simTest auth <sim_select> <pin> <puk>",
            "No SIM test: simTest nosim <sim_select>",
            "SIM select: simTest select",
            "SIM lock test: simTest lock <sim_select> <pin>",
            "",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print((char*) usagePtr[idx]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start simTest app.");
    const char* cardNum;
    const char* testString;

    // Get the test identifier and SIM select
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);
    }

    if (le_arg_NumArgs() > 1)
    {
        cardNum = le_arg_GetArg(1);
    }

    // Test: state
    if (strcmp(testString, "state") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_State( atoi(cardNum), pin);
    }
    // Test: create
    else if (strcmp(testString, "create") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
        }
        else
        {
            Print("error");
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Create(atoi(cardNum), pin);
    }
    // Test: authentification
    else if (strcmp(testString, "auth") == 0)
    {
        const char* pin;
        const char* puk;



        // Get the pin and puk codes
        if (le_arg_NumArgs() == 4)
        {
            pin = le_arg_GetArg(2);
            puk = le_arg_GetArg(3);
        }
        else
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Authentication(atoi(cardNum),pin,puk);
    }
    // Test: no sim
    else if (strcmp(testString, "nosim") == 0)
    {
        // Call the test function
        simTest_SimAbsent(atoi(cardNum));
    }
    // Test: SIM selection
    else if (strcmp(testString, "select") == 0)
    {
        // Call the test function
        simTest_SimSelect();
    }
    // Test: lock
    else if (strcmp(testString, "lock") == 0)
    {
        const char* pin;

        // Get the pin code
        if (le_arg_NumArgs() == 3)
        {
            pin = le_arg_GetArg(2);
        }
        else
        {
            Print("error");
            PrintUsage();
            exit(EXIT_FAILURE);
        }

        // Call the test function
        simTest_Lock(atoi(cardNum),pin);
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
