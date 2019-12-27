/**
 * This module implements the integration tests for AT commands client API and ATI response
 *  le_atClient_Start() and le_atClient_Stop(). It is not applicable on all platform
 *
 *  le_atClient_Start()
 *  le_atClient_Create()
 *  le_atClient_Delete()
 *  le_atClient_Stop()
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 *  Not applicable on all platform
 * 'apps: ' section minimal to use in the .sdef file is:
 *
 * apps:
    {
        // Platform services.
        ...
        ... atService
        ... atClientRTOS
    }
 *
 */

#include "legato.h"
#include "interfaces.h"

#ifndef CONFIG_CUSTOM_OS
#include <termios.h>
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Default AT commande timeout in [ms]
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_AT_CMD_TIMEOUT          10000

//--------------------------------------------------------------------------------------------------
/**
 * Numebr of loop
 */
//--------------------------------------------------------------------------------------------------
#define NB_TEST_LOOP                    5

//--------------------------------------------------------------------------------------------------
/**
 * Device path used for sending AT commands on linux platform
 */
//--------------------------------------------------------------------------------------------------
#define AT_PORT_PATH                    "/dev/ttyS1"

static void UnsolHandler
(
    const char* unsolPtr,
    void* contextPtr
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * Main test application
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_atClient_DeviceRef_t     atDeviceRef = NULL;
    le_atClient_CmdRef_t        cmdRef      = NULL;
    le_result_t                 res         = LE_FAULT;
    char responseStr[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};
    int i;
    int fd = 0;

    LE_INFO("=== BEGIN TEST FOR AT_CLIENT_RTOS ===");
    LE_TEST_PLAN(NB_TEST_LOOP*5);

#ifndef CONFIG_CUSTOM_OS
    fd = open(AT_PORT_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
    {
        LE_ERROR("Open device failed, errno %d, %s", errno, LE_ERRNO_TXT(errno));
    }

    struct termios term;
    bzero(&term, sizeof(term));

    // Default config
    tcgetattr(fd, &term);
    cfmakeraw(&term);
    term.c_oflag &= ~OCRNL;
    term.c_oflag &= ~ONLCR;
    term.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &term);
    tcflush(fd, TCIOFLUSH);
#endif

    for (i=0; i<NB_TEST_LOOP; i++)
    {
        // bind atClient to device
        atDeviceRef = le_atClient_Start(fd);
        LE_TEST_OK(NULL != atDeviceRef, "le_atClient_Start Passed - AtDeviceRef: [%p]",  atDeviceRef);

        // create the at command reference and set the at command and send
        res = le_atClient_SetCommandAndSend(&cmdRef,
            atDeviceRef,
            "ati",
            "Manufacturer:|Model:|Revision:",
            "OK|ERROR|+CME ERROR:",
            DEFAULT_AT_CMD_TIMEOUT);

        LE_TEST_OK( LE_OK == res, "le_atClient_SetCommandAndSend() Passed");

        le_atClient_AddUnsolicitedResponseHandler("+UNDEFINED:", atDeviceRef, UnsolHandler, NULL, 1);

        // get Intermediate and Final responses
        if(res == LE_OK)
        {
            res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                responseStr, sizeof(responseStr));

            while(res == LE_OK)
            {
                LE_INFO("Intermediate response <%s>",responseStr);
                if(strcmp(responseStr,"") == 0)
                {
                    LE_INFO("le_atClient_GetNextIntermediateResponse returned LE_OK");
                    LE_INFO("intermediateResponsBuf should not be empty");
                }
                res = le_atClient_GetNextIntermediateResponse(cmdRef,
                    responseStr, sizeof(responseStr));
            }

            res = le_atClient_GetFinalResponse(cmdRef,
                responseStr, sizeof(responseStr));
            LE_TEST_OK( LE_OK == res,
                "le_atClient_GetFinalResponse() Passed, final Response: [%s]",
                responseStr);
        }

        // delete at command Reference
        res = le_atClient_Delete(cmdRef);
        LE_TEST_OK( LE_OK == res, "le_atClient_Delete() Passed");

        // get summary for tests
        LE_INFO("------------------------------------------------");
        LE_INFO("FILE: %s", __FILE__);
        LE_INFO("Summary: Total Tests: %d Failures: %d",
            _le_test_GetNumTests(),
            _le_test_GetNumFailures());

        // unBind the device
        res = le_atClient_Stop(atDeviceRef);
        LE_TEST_OK( LE_OK == res, "le_atClient_Stop() Passed");
    }

#ifndef CONFIG_CUSTOM_OS
    // Close the fd
    close(fd);
#endif

    LE_INFO("=== END TEST FOR AT_CLIENT_RTOS ===");
    LE_TEST_EXIT;
}
