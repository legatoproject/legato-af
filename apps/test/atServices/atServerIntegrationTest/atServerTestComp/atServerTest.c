/**
 * This module implements the integration tests for AT commands server API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"

/* TO BE COMPLETED...*/

#if 0
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

    char atCommandName[LE_ATSERVER_COMMAND_MAX_BYTES];
    memset(atCommandName,0,LE_ATSERVER_COMMAND_MAX_BYTES);
    LE_ASSERT(le_atServer_GetCommandName(commandRef,atCommandName,LE_ATSERVER_COMMAND_MAX_BYTES)
                                                                                          == LE_OK);
    LE_INFO("AT command name %s", atCommandName);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
#if 0

    le_atServer_CmdRef_t atCmdRef = NULL;
    le_atServer_CmdRef_t atRef = NULL;
    le_atServer_CmdRef_t atARef = NULL;
    le_atServer_CmdRef_t atERef = NULL;

    le_atServer_DeviceRef_t devRef = le_atServer_Start("/dev/ttyS0");
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
#endif
}
