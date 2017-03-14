/**
 * This module implements the integration tests for AT commands server API.
 *
 * Use a TCP socket to connect to the atServer:
 * Open a connection (with telnet for instance) on port 1234
 *
 * The AT server bridge can be activated thanks to the AT command AT+BRIDGE:
 * - AT+BRIDGE="OPEN" opens the bridge
 * - AT+BRIDGE="ADD" adds the current device to the bridge
 * - AT+BRIDGE="REMOVE" removes the current device from the bridge
 * - AT+BRIDGE="CLOSE" closes the current device to the bridge
 * note that the bridge is opened on the device /dev/ttyAT (directed to the modem) and may have to
 * be changed depending on the used target.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NB_CLIENT_MAX 4

#define STRING_OPEN "OPEN"
#define STRING_CLOSE "CLOSE"
#define STRING_ADD "ADD"
#define STRING_REMOVE "REMOVE"

//--------------------------------------------------------------------------------------------------
/**
 * Extern function
 */
//--------------------------------------------------------------------------------------------------

void automaticTest_Init
(
    void
);

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

//--------------------------------------------------------------------------------------------------
/**
 * Call termination response
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TERMINATED_NO_CARRIER_UNSOL,
    TERMINATED_NO_CARRIER_FINAL,
    TERMINATED_OK
}
CallTerminationResponse_t;

//--------------------------------------------------------------------------------------------------
/**
 * Dial context
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_DeviceRef_t     devRef;
    le_atServer_CmdRef_t        commandRef;
    le_mcc_CallRef_t            testCallRef;
    bool                        isCallRefCreated;
    CallTerminationResponse_t   terminatedRsp;
    le_timer_Ref_t              timerRef;
}
DialContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Device context
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_fdMonitor_Ref_t monitorRef;
    le_atServer_DeviceRef_t devRef;
    int fd;
}
DeviceCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Test context
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mem_PoolRef_t        devicesPool;
    le_thread_Ref_t         mainThreadRef;
    DialContext_t           dialContext;
    le_atServer_BridgeRef_t bridgeRef;
}
TestCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Test global context
 *
 */
//--------------------------------------------------------------------------------------------------
static TestCtx_t TestCtx;

//--------------------------------------------------------------------------------------------------
/**
 * AT Commands handlers declaration
 *
 */
//--------------------------------------------------------------------------------------------------
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

static void AtBridgeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

void automaticTest_AtTestHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * AT Commands definition
 *
 */
//--------------------------------------------------------------------------------------------------
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
        .contextPtr = &TestCtx.dialContext
    },
    {
        .atCmdPtr = "ATD",
        .cmdRef = NULL,
        .handlerPtr = AtdCmdHandler,
        .contextPtr = &TestCtx.dialContext
    },
    {
        .atCmdPtr = "ATH",
        .cmdRef = NULL,
        .handlerPtr = AthCmdHandler,
        .contextPtr = &TestCtx.dialContext

    },
    {
        .atCmdPtr = "AT+BRIDGE",
        .cmdRef = NULL,
        .handlerPtr = AtBridgeHandler,
        .contextPtr = &TestCtx
    },
    {
        .atCmdPtr = "AT+TEST",
        .cmdRef = NULL,
        .handlerPtr = automaticTest_AtTestHandler,
        .contextPtr = &TestCtx.bridgeRef
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
    for (i = 0; i < parametersNumber; i++)
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
    le_atServer_DeviceRef_t devRef = NULL;

    LE_INFO("Closing Server Session");

    LE_ASSERT(le_atServer_GetDevice(commandRef, &devRef) == LE_OK);

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

    LE_ASSERT(le_atServer_Close(devRef) == LE_OK);
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

        le_atServer_DeviceRef_t devRef = NULL;
        LE_ASSERT(le_atServer_GetDevice(commandRef, &devRef) == LE_OK);

        if (LE_ATSERVER_TYPE_PARA == type)
        {
            memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
            LE_ASSERT(le_atServer_GetParameter(commandRef,
                                                0,
                                                param,
                                                LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);
            if (0 == strncmp(param, "1", strlen("1")))
            {
                LE_ASSERT(le_atServer_EnableEcho(devRef) == LE_OK);
            }
            else if (0 == strncmp(param, "0", strlen("0")))
            {
                LE_ASSERT(le_atServer_DisableEcho(devRef) == LE_OK);
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
        if (((dialNumber[i] < '0') || (dialNumber[i] > '9')) && (dialNumber[i] != '+'))
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

    if (dialCtxPtr->testCallRef != NULL)
    {
        le_result_t res = le_mcc_Answer(dialCtxPtr->testCallRef);

        if (res == LE_OK)
        {
            LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                    LE_ATSERVER_OK,
                                                    false,
                                                    "") == LE_OK);

            return;
        }
    }

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                            LE_ATSERVER_ERROR,
                                            true,
                                            "NO CARRIER") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * ATH command handler
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
        le_atServer_DeviceRef_t devRef = NULL;
        LE_ASSERT(le_atServer_GetDevice(commandRef, &devRef) == LE_OK);

        StopTimer(dialCtxPtr);
        dialCtxPtr->terminatedRsp = TERMINATED_OK;
        dialCtxPtr->devRef = devRef;
        dialCtxPtr->commandRef = commandRef;
        LE_ASSERT(le_mcc_HangUp(dialCtxPtr->testCallRef) == LE_OK);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * AT+BRIDGE command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtBridgeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    TestCtx_t* testCtxPtr = contextPtr;

    if (!testCtxPtr)
    {
        LE_ERROR("Bad context");
        return;
    }

    le_atServer_DeviceRef_t deviceRef;
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];

    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    if (parametersNumber != 1)
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);
        return;
    }

    // Get the deviceRef where the AT command was issued
    LE_ASSERT(le_atServer_GetDevice(commandRef, &deviceRef) == LE_OK);

    LE_ASSERT(le_atServer_GetParameter(commandRef,
                                       0,
                                       param,
                                       LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

    if (strncmp(param, STRING_OPEN, strlen(STRING_OPEN)) == 0)
    {
        if (testCtxPtr->bridgeRef == NULL)
        {
            // Start AT command bridge
            int fdTtyAT = open("/dev/ttyAT", O_RDWR | O_NOCTTY | O_NONBLOCK);
            testCtxPtr->bridgeRef = le_atServer_OpenBridge(fdTtyAT);
            LE_ASSERT(testCtxPtr->bridgeRef != NULL);
        }
        else
        {
            goto error;
        }
    }
    else if (strncmp(param, STRING_ADD, strlen(STRING_ADD)) == 0)
    {
        if (!testCtxPtr->bridgeRef)
        {
            goto error;
        }

        LE_ASSERT(le_atServer_AddDeviceToBridge(deviceRef,testCtxPtr->bridgeRef) == LE_OK);
    }
    else if (strncmp(param, STRING_REMOVE, strlen(STRING_REMOVE)) == 0)
    {
        if (!testCtxPtr->bridgeRef)
        {
            goto error;
        }

        LE_ASSERT(le_atServer_RemoveDeviceFromBridge(deviceRef,testCtxPtr->bridgeRef) == LE_OK);
    }
    else if (strncmp(param, STRING_CLOSE, strlen(STRING_CLOSE)) == 0)
    {
        if (!testCtxPtr->bridgeRef)
        {
            goto error;
        }

        LE_ASSERT(le_atServer_CloseBridge(testCtxPtr->bridgeRef) == LE_OK);
        testCtxPtr->bridgeRef = NULL;
    }

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

    return;

error:
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);

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
 * Socket handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SocketHandler
(
    int   fd,
    short events
)
{
    LE_INFO("Host disconnected");

    DeviceCtx_t* deviceCtxPtr = le_fdMonitor_GetContextPtr();

    if (!deviceCtxPtr)
    {
        LE_ERROR("No device context");
        return;
    }

    if (deviceCtxPtr->fd != fd)
    {
        LE_ERROR("Bad fd: %d (expected: %d)", fd, deviceCtxPtr->fd);
        return;
    }

    if (POLLRDHUP == events)
    {
        le_atServer_Close(deviceCtxPtr->devRef);
        le_fdMonitor_Delete(deviceCtxPtr->monitorRef);
        close(deviceCtxPtr->fd);
        le_mem_Release(deviceCtxPtr);
    }
    else
    {
        LE_ERROR("Unknown error %d", events);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add socket monitoring.
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddMonitoring
(
    void* param1Ptr,
    void* param2Ptr
)
{
    DeviceCtx_t* deviceCtxPtr = param1Ptr;
    LE_ASSERT(deviceCtxPtr != NULL);

    char monitorName[30];
    snprintf(monitorName, sizeof(monitorName), "monitor-%d", deviceCtxPtr->fd);

    le_fdMonitor_Ref_t monitorRef = le_fdMonitor_Create(monitorName,
                                                        deviceCtxPtr->fd,
                                                        SocketHandler,
                                                        POLLRDHUP);


    deviceCtxPtr->monitorRef = monitorRef;

    le_fdMonitor_SetContextPtr(monitorRef, deviceCtxPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Socket thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* SocketThread
(
    void* contextPtr
)
{
    le_atServer_ConnectService();

    int ret, optVal = 1;
    struct sockaddr_in myAddress, clientAddress;
    le_atServer_DeviceRef_t devRef = NULL;
    int sockFd;
    TestCtx_t* testCtxPtr = contextPtr;

    // Create the socket
    sockFd = socket (AF_INET, SOCK_STREAM, 0);

    if (sockFd < 0)
    {
        LE_ERROR("creating socket failed: %m");
        return NULL;
    }

    // set socket option
    ret = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (ret)
    {
        LE_ERROR("error setting socket option %m");
        return NULL;
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
        return NULL;
    }

    // Listen on the socket
    listen(sockFd, NB_CLIENT_MAX);

    socklen_t addressLen = sizeof(clientAddress);

    while (1)
    {
        int connFd;

        connFd = accept(sockFd, (struct sockaddr *)&clientAddress, &addressLen);

        devRef = le_atServer_Open(dup(connFd));
        LE_ASSERT(devRef != NULL);

        // Create device context and monitor the socket
        DeviceCtx_t* deviceCtxPtr = le_mem_ForceAlloc(testCtxPtr->devicesPool);
        LE_ASSERT(deviceCtxPtr != NULL);

        deviceCtxPtr->devRef = devRef;
        deviceCtxPtr->fd = connFd;

        le_event_QueueFunctionToThread(testCtxPtr->mainThreadRef,
                                       AddMonitoring,
                                       deviceCtxPtr,
                                       NULL);

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("AT server test starts");
    int i = 0;

    memset(&TestCtx, 0, sizeof(TestCtx));

    // Device pool allocation
    TestCtx.devicesPool = le_mem_CreatePool("DevicesPool",sizeof(DeviceCtx_t));
    le_mem_ExpandPool(TestCtx.devicesPool, NB_CLIENT_MAX);

    TestCtx.mainThreadRef = le_thread_GetCurrent();

    // AT commands subscriptions
    while ( i < NUM_ARRAY_MEMBERS(AtCmdCreation) )
    {
        AtCmdCreation[i].cmdRef = le_atServer_Create(AtCmdCreation[i].atCmdPtr);
        LE_ASSERT(AtCmdCreation[i].cmdRef != NULL);

        le_atServer_AddCommandHandler(
            AtCmdCreation[i].cmdRef, AtCmdCreation[i].handlerPtr, AtCmdCreation[i].contextPtr);

        i++;
    }

    // Add a call handler
    le_mcc_AddCallEventHandler(MyCallEventHandler, &TestCtx.dialContext);

    le_thread_Start(le_thread_Create("SocketThread",
                                     SocketThread,
                                     &TestCtx));

    automaticTest_Init();
}

