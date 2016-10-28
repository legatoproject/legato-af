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

static int SockFd;
static int ConnFd;

static le_atServer_DeviceRef_t DevRef = NULL;

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
    },
    {
        .atCmdPtr = "AT+CLOSE",
        .cmdRef = NULL,
        .handlerPtr = CloseHandler,
    },
    {
        .atCmdPtr = "AT+ABCD",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
    },
    {
        .atCmdPtr = "AT",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
    },
    {
        .atCmdPtr = "ATA",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
    },
    {
        .atCmdPtr = "ATE",
        .cmdRef = NULL,
        .handlerPtr = AtCmdHandler,
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
            AtCmdCreation[i].cmdRef, AtCmdCreation[i].handlerPtr, NULL);

        i++;
    }
}
