/**
 * @file trigger.c
 *
 * This module implements an utility to trig the eCall application. It is given as an example, it
 * shows how the eCall app must be trigged.
 *
 * You can call the utility by issuing the command:
 * @verbatim
   /opt/legato/apps/eCall/bin/trig <number of passengers>
   @endverbatim
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of the 'number of passengers' argument.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PAX_COUNT_MAX_LEN (3+1)

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the 'trig' tool is:",
            "   trig <number of passengers>",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    char        paxCountString[PAX_COUNT_MAX_LEN] = { 0 };
    uint32_t    paxCount;

    if (le_arg_NumArgs() == 1)
    {
        le_arg_GetArg(0, paxCountString, PAX_COUNT_MAX_LEN);
        paxCount = atoi(paxCountString);

        LE_INFO("trig with.%d", paxCount);
        ecallApp_StartSession(paxCount);
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT trig");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}