/**
 * This module implements the integration tests for AT commands server API.
 *
 * Use a TCP socket to connect to the atServer:
 * Open a connection (with telnet for instance) on port 1234
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PARAM_MAX   10

//--------------------------------------------------------------------------------------------------
/**
 * AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                         atCmdPtr;
    le_atServer_CmdRef_t                cmdRef;
    le_atServer_CommandHandlerFunc_t    handlerPtr;
    void*                               contextPtr;
}
AtCmd_t;

//------------------------------------------------------------------------------
/**
 * Dial context
 *
 */
//------------------------------------------------------------------------------
typedef struct
{
    le_atServer_DeviceRef_t     devRef;
    le_atServer_CmdRef_t        commandRef;
    le_mcc_CallRef_t            testCallRef;
    bool                        isCallRefCreated;
    enum
    {
        TERMINATED_NO_CARRIER_UNSOL,
        TERMINATED_NO_CARRIER_FINAL,
        TERMINATED_OK
    }                           terminatedRsp;
    le_timer_Ref_t              timerRef;
}
DialContext_t;

static le_atServer_DeviceRef_t DevRef = NULL;
static DialContext_t DialContext;

static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void DelHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void CloseHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AteCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AtdCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AtaCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AthCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void CloseHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static AtCmd_t AtCmdCreation[] =
{
    {
        .atCmdPtr = "AT+DEL",
        .cmdRef = NULL,
        .handlerPtr = DelHandler,
        .contextPtr = NULL
    },
    {
        .atCmdPtr = "AT+CLOSE",
        .cmdRef = NULL,
        .handlerPtr = CloseHandler,
        .contextPtr = NULL
    },
    {
        .atCmdPtr = "AT+ABCD",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
        .contextPtr = NULL
    },
    {
        .atCmdPtr = "AT",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
        .contextPtr = NULL
    },
    {
        .atCmdPtr = "ATE",
        .cmdRef = NULL,
        .handlerPtr = AteCmdHandler,
        .contextPtr = NULL
    },
    {
        .atCmdPtr = "ATA",
        .cmdRef = NULL,
        .handlerPtr = AtaCmdHandler,
        .contextPtr = &DialContext
    },
    {
        .atCmdPtr = "ATD",
        .cmdRef = NULL,
        .handlerPtr = AtdCmdHandler,
        .contextPtr = &DialContext
    },
    {
        .atCmdPtr = "ATH",
        .cmdRef = NULL,
        .handlerPtr = AthCmdHandler,
        .contextPtr = &DialContext

    },
};


//--------------------------------------------------------------------------------------------------
/**
 * Prepare handler
 *
 */
//--------------------------------------------------------------------------------------------------
void PrepareHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    LE_INFO("commandRef %p", commandRef);

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);
    // Get command name
    LE_ASSERT(le_atServer_GetCommandName(commandRef,atCommandName,LE_ATDEFS_COMMAND_MAX_BYTES)
        == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    // Send the command type into an intermediate response
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

    // Send parameters into an intermediate response
    for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
        LE_ASSERT(le_atServer_GetParameter(commandRef,
                                           i,
                                           param,
                                           LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * generic AT command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    PrepareHandler(commandRef, type, parametersNumber, contextPtr);
    // Send Final response
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

}

//--------------------------------------------------------------------------------------------------
/**
 * AT+DEL command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void DelHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    int i = 0;

    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    // Delete all AT commands except +DEL and +CLOSE
    while ( i < NUM_ARRAY_MEMBERS(AtCmdCreation) )
    {
        if ((0 != strncmp(AtCmdCreation[i].atCmdPtr, "AT+DEL", strlen("AT+DEL"))) &&
            (0 != strncmp(AtCmdCreation[i].atCmdPtr, "AT+CLOSE", strlen("AT+CLOSE"))))
        {
            LE_ASSERT(le_atServer_Delete(AtCmdCreation[i].cmdRef) == LE_OK);
        }
        i++;
    }

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * AT+STOP command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    LE_INFO("Closing Server Session");

    LE_ASSERT(le_atServer_Close(DevRef) == LE_OK);

    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * ATE command handler (echo)
 *
 */
//--------------------------------------------------------------------------------------------------
static void AteCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    char        param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    le_result_t res = LE_OK;

    if (1 != parametersNumber)
    {
        res = LE_FAULT;
    }
    else
    {
        PrepareHandler(commandRef, type, parametersNumber, contextPtr);

        if (LE_ATSERVER_TYPE_PARA == type)
        {
            memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
            LE_ASSERT(le_atServer_GetParameter(commandRef,
                                                0,
                                                param,
                                                LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);
            if (0 == strncmp(param, "1", strlen("1")))
            {
                LE_ASSERT(le_atServer_EnableEcho(DevRef) == LE_OK);
            }
            else if (0 == strncmp(param, "0", strlen("0")))
            {
                LE_ASSERT(le_atServer_DisableEcho(DevRef) == LE_OK);
            }
            else
            {
                res = LE_FAULT;
            }
        }
    }

    // Send Final response
    if (LE_OK == res)
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
    }
    else
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop timer used for Dial command
 *
 */
//--------------------------------------------------------------------------------------------------
static void StopTimer
(
    DialContext_t* dialCtxPtr
)
{
    if(dialCtxPtr->timerRef)
    {
        le_timer_Stop(dialCtxPtr->timerRef);
        le_timer_Delete(dialCtxPtr->timerRef);
        dialCtxPtr->timerRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Incoming call timer handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void IncomingCallTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_ASSERT(le_atServer_SendUnsolicitedResponse("RING",
                                      LE_ATSERVER_ALL_DEVICES,
                                      NULL) == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * ATD command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtdCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_INFO("Dial command");
    le_result_t res;
    DialContext_t* dialCtxPtr = contextPtr;

    char dialNumber[LE_ATDEFS_PARAMETER_MAX_BYTES];

    memset(dialNumber, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);

    if (type != LE_ATSERVER_TYPE_PARA)
    {
        LE_ERROR("Bad type %d", type);
        goto error;
    }

    if (parametersNumber != 1)
    {
        LE_ERROR("Bad param number %d", parametersNumber);
        goto error;
    }

    LE_ASSERT(le_atServer_GetParameter(commandRef,
                                       0,
                                       dialNumber,
                                       LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

    LE_INFO("Dial %s", dialNumber);

    // Check if CSD call
    if ( dialNumber[strlen(dialNumber)-1] != ';' )
    {
        LE_ERROR("CSD call");
        goto error;
    }

    int i;
    for (i=0; i < strlen(dialNumber)-1; i++)
    {
        if (((dialNumber[i] < 0x30) || (dialNumber[i] > 0x39)) && (dialNumber[i] != '+'))
        {
            LE_ERROR("Invalid char %c", dialNumber[i]);
            goto error;
        }
    }

    dialCtxPtr->testCallRef = le_mcc_Create(dialNumber);

    if (dialCtxPtr->testCallRef == NULL)
    {
        goto error;
    }

    dialCtxPtr->isCallRefCreated = true;

    dialCtxPtr->terminatedRsp = TERMINATED_NO_CARRIER_UNSOL;

    res = le_mcc_Start(dialCtxPtr->testCallRef);

    if (res != LE_OK)
    {
        goto error;
    }

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

    return;

error:
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                            LE_ATSERVER_ERROR,
                                            true,
                                            "NO CARRIER") == LE_OK);

}

//--------------------------------------------------------------------------------------------------
/**
 * ATA command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtaCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    DialContext_t* dialCtxPtr = contextPtr;

    if (dialCtxPtr->testCallRef == NULL)
    {
        goto error;
    }
    else
    {
        le_result_t res = le_mcc_Answer(dialCtxPtr->testCallRef);

        if (res != LE_OK)
        {
            goto error;
        }

        LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                LE_ATSERVER_OK,
                                                false,
                                                "") == LE_OK);

        return;
    }

error:
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                            LE_ATSERVER_ERROR,
                                            true,
                                            "NO CARRIER") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * ATD command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AthCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    DialContext_t* dialCtxPtr = contextPtr;

    if (dialCtxPtr->testCallRef == NULL)
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                LE_ATSERVER_OK,
                                                false,
                                                "") == LE_OK);
    }
    else
    {
        StopTimer(dialCtxPtr);
        dialCtxPtr->terminatedRsp = TERMINATED_OK;
        dialCtxPtr->devRef = DevRef;
        dialCtxPtr->commandRef = commandRef;
        LE_ASSERT(le_mcc_HangUp(dialCtxPtr->testCallRef) == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent,
    void* contextPtr
)
{
    LE_INFO("callEvent %d", callEvent);
    DialContext_t* dialCtxPtr = contextPtr;

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_ASSERT(le_atServer_SendUnsolicitedResponse("DIAL: ALERTING",
                                                      LE_ATSERVER_ALL_DEVICES,
                                                      NULL) == LE_OK);

    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        StopTimer(dialCtxPtr);

        LE_ASSERT(le_atServer_SendUnsolicitedResponse("DIAL: CONNECTED",
                                                      LE_ATSERVER_ALL_DEVICES,
                                                      NULL) == LE_OK);
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        StopTimer(dialCtxPtr);

        switch (dialCtxPtr->terminatedRsp)
        {
            case TERMINATED_NO_CARRIER_UNSOL:
                LE_ASSERT(le_atServer_SendUnsolicitedResponse("NO CARRIER",
                                                  LE_ATSERVER_ALL_DEVICES,
                                                  NULL) == LE_OK);
            break;
            case TERMINATED_NO_CARRIER_FINAL:
                    LE_ASSERT(le_atServer_SendFinalResponse(dialCtxPtr->commandRef,
                                                LE_ATSERVER_OK,
                                                true,
                                                "NO CARRIER") == LE_OK);
            break;
            case TERMINATED_OK:
                    LE_ASSERT(le_atServer_SendFinalResponse(dialCtxPtr->commandRef,
                                    LE_ATSERVER_OK,
                                    false,
                                    "") == LE_OK);
            break;
        }

        if (dialCtxPtr->isCallRefCreated)
        {
            le_mcc_Delete(dialCtxPtr->testCallRef);
            dialCtxPtr->isCallRefCreated = false;
        }

        dialCtxPtr->testCallRef = NULL;
        le_mcc_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        dialCtxPtr->testCallRef = callRef;
        dialCtxPtr->isCallRefCreated = false;
        dialCtxPtr->terminatedRsp = TERMINATED_NO_CARRIER_UNSOL;
        dialCtxPtr->timerRef = le_timer_Create("IncomingCall");
        le_timer_SetHandler(dialCtxPtr->timerRef, IncomingCallTimerHandler);
        // Dispaly a RING every 3s
        le_timer_SetMsInterval(dialCtxPtr->timerRef, 3000);
        le_timer_SetRepeat(dialCtxPtr->timerRef, 0);
        le_timer_Start(dialCtxPtr->timerRef);

        LE_ASSERT(le_atServer_SendUnsolicitedResponse("RING",
                                              LE_ATSERVER_ALL_DEVICES,
                                              NULL) == LE_OK);
    }
    else if (callEvent == LE_MCC_EVENT_ORIGINATING)
    {
        LE_ASSERT(le_atServer_SendUnsolicitedResponse("DIAL: ORIGINATING",
                                                      LE_ATSERVER_ALL_DEVICES,
                                                      NULL) == LE_OK);
    }
    else if (callEvent == LE_MCC_EVENT_SETUP)
    {
        LE_ASSERT(le_atServer_SendUnsolicitedResponse("DIAL: SETUP",
                                                      LE_ATSERVER_ALL_DEVICES,
                                                      NULL) == LE_OK);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    le_atServer_Close(DevRef);
    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int sockFd;
    int connFd;
    int ret, optVal = 1, i = 0;
    struct sockaddr_in myAddress, clientAddress;

    LE_INFO("======== Start atServer implementation Test ========");

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    // Create the socket
    sockFd = socket (AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        LE_ERROR("creating socket failed: %m");
        return;
    }

    // set socket option
    ret = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (ret)
    {
        LE_ERROR("error setting socket option %m");
        return;
    }

    memset(&myAddress,0,sizeof(myAddress));

    myAddress.sin_port = htons(1235);
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind server - socket
    ret = bind(sockFd,(struct sockaddr_in *)&myAddress,sizeof(myAddress));
    if (ret)
    {
        LE_ERROR("%m");
        return;
    }

    // Listen on the socket
    listen(sockFd,1);

    socklen_t addressLen = sizeof(clientAddress);

    connFd = accept(sockFd, (struct sockaddr *)&clientAddress, &addressLen);

    DevRef = le_atServer_Open(connFd);
    LE_ASSERT(DevRef != NULL);

    // AT commands subscriptions
    while ( i < NUM_ARRAY_MEMBERS(AtCmdCreation) )
    {
        AtCmdCreation[i].cmdRef = le_atServer_Create(AtCmdCreation[i].atCmdPtr);
        LE_ASSERT(AtCmdCreation[i].cmdRef != NULL);

        le_atServer_AddCommandHandler(
            AtCmdCreation[i].cmdRef, AtCmdCreation[i].handlerPtr, AtCmdCreation[i].contextPtr);

        i++;
    }

    memset(&DialContext,0,sizeof(DialContext));

    // Add a call handler
    le_mcc_AddCallEventHandler(MyCallEventHandler, &DialContext);
}
