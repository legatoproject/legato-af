 /**
  * This module implements the le_riPin's test.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"


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
        "Usage of the riPinTest app is:",
        "\tapp runProc riPinTest --exe=riPinTest -- <take/release/pulse> [pulse duration in ms]"
    };

    for (idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if (sandboxed)
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
 * Initialize the test component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    bool amIOwner;
    le_result_t res;

    LE_INFO("Init");

    if (le_arg_NumArgs() >= 1)
    {
        LE_INFO("======== Ring Indicator signal Test ========");

        const char* testCase = le_arg_GetArg(0);
        if (NULL == testCase)
        {
            LE_INFO("testCase is NULL");
            exit(EXIT_FAILURE);
        }
        LE_INFO("\tTest case: '%s'", testCase);

        res = le_riPin_AmIOwnerOfRingSignal(&amIOwner);
        if (LE_OK == res)
        {
            if (true == amIOwner)
            {
                LE_INFO("Legato is the owner of the Ring Indicator signal");
            }
            else
            {
                LE_INFO("Legato is NOT the owner of the Ring Indicator signal");
            }
        }
        LE_ERROR_IF(res == LE_FAULT, "Failed to retrieve the owner of the Ring Indicator signal");

        if (0 == strncmp(testCase, "take", strlen("take")))
        {
            res = le_riPin_TakeRingSignal();
            LE_INFO_IF(LE_OK == res, "Legato is the owner of the Ring Indicator signal");
            LE_WARN_IF(LE_UNSUPPORTED == res, "Platform doesn't support this request");
            LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));
        }
        else if (0 == strncmp(testCase, "release", strlen("release")))
        {
            res = le_riPin_ReleaseRingSignal();
            LE_INFO_IF(LE_OK == res, "Legato is no more the owner of the Ring Indicator signal");
            LE_WARN_IF(LE_UNSUPPORTED == res, "Platform doesn't support this request");
            LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));
        }
        else if (0 == strncmp(testCase, "pulse", strlen("pulse")))
        {
            const char* durPtr = le_arg_GetArg(1);
            if (!durPtr)
            {
                LE_ERROR("No pulse duration provided");
                exit(EXIT_FAILURE);
            }
            uint32_t duration = atoi(durPtr);
            le_riPin_PulseRingSignal(duration);
            LE_INFO("Check the Ring indicator signal");
        }
        else
        {
            PrintUsage();
            LE_INFO("EXIT riPinTest");
            exit(EXIT_FAILURE);
        }

        LE_INFO("======== Ring Indicator signal test started successfully ========");
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT riPinTest");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

