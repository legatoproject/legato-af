/**
 * This module implements the integration tests for AT commands server API.
 *
 * This is using the uart serial port.
 * First, unbind the linux console on the uart:
 * 1. in /etc/inittab, comment line which starts getty
 * 2. relaunch init: kill -HUP 1
 * 3. kill getty
 * 4. On PC side, open a terminal on the plugged uart console
 * 5. Send AT commands. Accepted AT commands: AT, ATA, ATE, AT+ABCD
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"


#define PARAM_MAX 10


//--------------------------------------------------------------------------------------------------
/**
 * AT command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_INFO("commandRef %p", commandRef);

    char rsp[LE_ATSERVER_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATSERVER_COMMAND_MAX_BYTES];
    memset(atCommandName,0,LE_ATSERVER_COMMAND_MAX_BYTES);

    // Get command name
    LE_ASSERT(le_atServer_GetCommandName(commandRef,atCommandName,LE_ATSERVER_COMMAND_MAX_BYTES)
                                                                                          == LE_OK);
    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATSERVER_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATSERVER_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp+strlen(rsp), LE_ATSERVER_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp+strlen(rsp), LE_ATSERVER_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp+strlen(rsp), LE_ATSERVER_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp+strlen(rsp), LE_ATSERVER_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    // Send the command type into an intermediate response
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);


    char param[LE_ATSERVER_PARAMETER_MAX_BYTES];
    int i = 0;

    // Send parameters into an intermediate response
    for (; i < parametersNumber; i++)
    {
        memset(param,0,LE_ATSERVER_PARAMETER_MAX_BYTES);
        LE_ASSERT(le_atServer_GetParameter( commandRef,
                                            i,
                                            param,
                                            LE_ATSERVER_PARAMETER_MAX_BYTES) == LE_OK);



        memset(rsp,0,LE_ATSERVER_RESPONSE_MAX_BYTES);
        snprintf(rsp, LE_ATSERVER_RESPONSE_MAX_BYTES, "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

        LE_INFO("param %d %s", i, param);
    }

    // Send Final response
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_atServer_CmdRef_t atCmdRef = NULL;
    le_atServer_CmdRef_t atRef = NULL;
    le_atServer_CmdRef_t atARef = NULL;
    le_atServer_CmdRef_t atERef = NULL;

    LE_INFO("AT server test starts");

    le_atServer_DeviceRef_t devRef = le_atServer_Start("/dev/ttyHSL1");
    LE_ASSERT(devRef != NULL);

    struct
    {
        const char* atCmdPtr;
        le_atServer_CmdRef_t cmdRef;
    } atCmdCreation[] =     {
                                { "AT+ABCD",    atCmdRef    },
                                { "AT",         atRef       },
                                { "ATA",        atARef      },
                                { "ATE",        atERef      },
                                { NULL,         NULL        }
                            };

    int i=0;

    // AT commands subscriptions
    while ( atCmdCreation[i].atCmdPtr != NULL )
    {
        atCmdCreation[i].cmdRef = le_atServer_Create(atCmdCreation[i].atCmdPtr);
        LE_ASSERT(atCmdCreation[i].cmdRef != NULL);

        le_atServer_AddCommandHandler(atCmdCreation[i].cmdRef, AtCmdHandler, NULL);

        i++;
    }

    LE_INFO("AT server test started");
}
