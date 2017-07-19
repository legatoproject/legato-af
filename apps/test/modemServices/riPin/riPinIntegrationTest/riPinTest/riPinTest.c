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
            "   app runProc riPinTest --exe=riPinTest -- <take/release/pulse> [pulse duration in ms]"};

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
        LE_INFO("   Test case.%s", testCase);

        res = le_riPin_AmIOwnerOfRingSignal(&amIOwner);
        if (res == LE_OK)
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

        LE_ERROR_IF(res == LE_FAULT, "Failed to know the owner of the Ring Indicator signal");

        if (strncmp(testCase, "take", strlen("take")) == 0)
        {
            res = le_riPin_TakeRingSignal();
            LE_INFO_IF(res == LE_OK, "Legato is the owner of the Ring Indicator signal");
            LE_WARN_IF(res == LE_UNSUPPORTED, "Platform doesn't support this request");
        }
        else if (strncmp(testCase, "release", strlen("release")) == 0)
        {
            res = le_riPin_ReleaseRingSignal();
            LE_INFO_IF(res == LE_OK, "Legato is no more the owner of the Ring Indicator signal");
            LE_WARN_IF(res == LE_UNSUPPORTED, "Platform doesn't support this request");
        }
        else if (strncmp(testCase, "pulse", strlen("pulse")) == 0)
        {
            const char* durPtr = le_arg_GetArg(1);
            if (NULL == durPtr)
            {
                LE_ERROR("durPtr is NULL");
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

