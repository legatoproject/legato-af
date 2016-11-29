/**
 * This module implements the integration tests for AT commands server API.
 *
 * 2 ways to connect to a bearer:
 *
 * - Using UART:
 * 1. Define __UART__ flag
 * 2. First, unbind the linux console on the uart:
 *      a. in /etc/inittab, comment line which starts getty
 *      b. relaunch init: kill -HUP 1
 *      c. kill getty
 *      d. On PC side, open a terminal on the plugged uart console
 *      e. Send AT commands. Accepted AT commands: AT, ATA, ATE, AT+ABCD
 *
 * - Using TCP socket:
 * Open a connection (with telnet for instance) on port 1234
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Define __UART__ flag if AT commands server is bound on the UART.
// Undefine it for TCP socket binding.
//#define __UART__

#define ARRAY_SIZE(X)   (sizeof(X)/sizeof(X[0]))

#define PARAM_MAX   10

//------------------------------------------------------------------------------
/**
 * struct AtCmd defintion
 *
 */
//------------------------------------------------------------------------------
struct AtCmd
{
    const char* atCmdPtr;
    le_atServer_CmdRef_t cmdRef;
    le_atServer_CommandHandlerFunc_t handlerPtr;
    void* contextPtr;
};

//------------------------------------------------------------------------------
/**
 * struct AtCmdRetvals contains all possible return values from the common
 * function
 *
 * @atCmdParams: array containing the command parameters
 *
 */
//------------------------------------------------------------------------------
struct AtCmdRetVals
{
    char atCmdParams[PARAM_MAX][LE_ATDEFS_COMMAND_MAX_BYTES];
};

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

static int SockFd;
static int ConnFd;

static le_atServer_DeviceRef_t DevRef = NULL;
static DialContext_t DialContext;

static void AtCmdHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void DelHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AtdCmdHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AtaCmdHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void AthCmdHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static void CloseHandler(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
);

static struct AtCmd AtCmdCreation[] =
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
        .atCmdPtr = "ATA",
        .cmdRef = NULL,
        .handlerPtr = AtaCmdHandler,
        .contextPtr = &DialContext
    },
    {
        .atCmdPtr = "ATE",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
        .contextPtr = NULL
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

//------------------------------------------------------------------------------
/**
 * Uppercase a string
 */
//------------------------------------------------------------------------------
static char* Uppercase
(
    char* strPtr
)
{
    int i = 0;
    while (strPtr[i] != '\0')
    {
        strPtr[i] = toupper(strPtr[i]);
        i++;
    }

    return strPtr;
}

//------------------------------------------------------------------------------
/**
 * get command name's refrence
 */
//------------------------------------------------------------------------------
static le_atServer_CmdRef_t GetRef
(
    const char* cmdName
)
{
    int i = 0;

    for (i=0; i<ARRAY_SIZE(AtCmdCreation); i++)
    {
        if ( ! (strcmp(AtCmdCreation[i].atCmdPtr, cmdName)) )
        {
            return AtCmdCreation[i].cmdRef;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------
/**
 * Prepare handler
 *
 */
//------------------------------------------------------------------------------
static struct AtCmdRetVals PrepareHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    struct AtCmdRetVals atCmdRetVals;
    int i = 0;

    LE_INFO("commandRef %p", commandRef);

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    for (i=0; i<PARAM_MAX; i++)
    {
        memset(atCmdRetVals.atCmdParams[i],
            0, LE_ATDEFS_PARAMETER_MAX_BYTES);
    }

    // Get command name
    LE_ASSERT(le_atServer_GetCommandName(
        commandRef,atCommandName,LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
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
        LE_ASSERT(
            le_atServer_GetParameter(commandRef,
                                    i,
                                    param,
                                    LE_ATDEFS_PARAMETER_MAX_BYTES)
            == LE_OK);


        memset(rsp,0,LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp,LE_ATDEFS_RESPONSE_MAX_BYTES,
            "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(
            le_atServer_SendIntermediateResponse(commandRef, rsp)
            == LE_OK);

        strncpy(atCmdRetVals.atCmdParams[i], param, sizeof(param));
        LE_INFO("param %d \"%s\"", i, atCmdRetVals.atCmdParams[i]);
    }

    return atCmdRetVals;
}

//------------------------------------------------------------------------------
/**
 * generic AT command handler
 *
 */
//------------------------------------------------------------------------------
static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    PrepareHandler(commandRef, type, parametersNumber, contextPtr);
    // Send Final response
    LE_ASSERT(
        le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "")
        == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * AT+DEL command handler
 *
 */
//------------------------------------------------------------------------------
static void DelHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    le_result_t result = LE_OK;
    le_atServer_CmdRef_t atCmdRef = NULL;
    struct AtCmdRetVals atCmdRetVals;
    char* usrAtCmd;
    int i = 0;
    le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_OK;

    atCmdRetVals =
        PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    for (i=0; i<parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        usrAtCmd = Uppercase(atCmdRetVals.atCmdParams[i]);
        LE_INFO("deleting command %s", usrAtCmd);
        atCmdRef = GetRef(usrAtCmd);
        if (!atCmdRef)
        {
            LE_ERROR("command %s not registred ", usrAtCmd);
        }

        result = le_atServer_Delete(atCmdRef);
        if (result)
        {
            LE_ERROR("deleting command %s failed with error %d",
                usrAtCmd, result);
            finalRsp = LE_ATSERVER_ERROR;
        }
        else
        {
            LE_INFO("command %s deleted", usrAtCmd);
        }
    }

    LE_ASSERT(
        le_atServer_SendFinalResponse(
            commandRef, finalRsp, false, "")
        == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * AT+STOP command handler
 *
 */
//------------------------------------------------------------------------------
static void CloseHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_INFO("Closing Server Session");

    LE_ASSERT(le_atServer_Close(DevRef) == LE_OK);

    exit(0);
}

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

//------------------------------------------------------------------------------
/**
 * Incoming call timer handler
 *
 */
//------------------------------------------------------------------------------
static void IncomingCallTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_ASSERT(le_atServer_SendUnsolicitedResponse("RING",
                                      LE_ATSERVER_ALL_DEVICES,
                                      NULL) == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * ATD command handler
 *
 */
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/**
 * ATA command handler
 *
 */
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/**
 * ATD command handler
 *
 */
//------------------------------------------------------------------------------
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


//------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    le_atServer_Close(DevRef);
    exit(0);
}

//------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//------------------------------------------------------------------------------
COMPONENT_INIT
{
    int ret, optVal = 1, i = 0;
    struct sockaddr_in myAddress, clientAddress;

    LE_INFO("AT server test starts");

    /*
     * Register a signal event handler
     * for SIGINT when user interrupts/terminates process
     */
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

#ifdef __UART__
    // UART connection
    ConnFd = open("/dev/ttyHSL1", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        LE_ERROR("Error in opening the device errno = %d", errno);
        return;
    }
#else
    // Create the socket
    SockFd = socket (AF_INET, SOCK_STREAM, 0);

    if (SockFd < 0)
    {
        LE_ERROR("creating socket failed: %m");
        return;
    }

    // set socket option
    ret = setsockopt(SockFd, SOL_SOCKET, \
            SO_REUSEADDR, &optVal, sizeof(optVal));
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
    ret = bind(SockFd,(struct sockaddr_in *)&myAddress,sizeof(myAddress));
    if (ret)
    {
        LE_ERROR("%m");
        return;
    }

    // Listen on the socket
    listen(SockFd,1);

    socklen_t addressLen = sizeof(clientAddress);

    ConnFd =
        accept(SockFd, (struct sockaddr *)&clientAddress, &addressLen);
#endif // __UART__

    DevRef = le_atServer_Open(ConnFd);
    LE_ASSERT(DevRef != NULL);

    // AT commands subscriptions
    while ( i < ARRAY_SIZE(AtCmdCreation) )
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
