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

#define PARAM_MAX 10

le_atServer_DeviceRef_t DevRef = NULL;

// Define __UART__ flag if AT commands server is bound on the UART.
// Undefine it for TCP socket binding.
//#define __UART__

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
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    le_atServer_Stop(DevRef);
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

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

#ifdef __UART__
    // UART connection
    int fd = open("/dev/ttyHSL1", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        LE_ERROR("Error in opening the device errno = %d", errno);
        return;
    }
#else
    // Create the socket
    int socketFd = socket (AF_INET, SOCK_STREAM, 0);
    int fd;

    LE_ASSERT(socketFd > 0);

    struct sockaddr_in myAddress, clientAddress;

    memset(&myAddress,0,sizeof(myAddress));

    myAddress.sin_port = htons(1234);
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind server - socket
    LE_ASSERT( bind(socketFd,(struct sockaddr_in *)&myAddress,sizeof(myAddress)) == 0 );

    // Listen on the socket
    listen(socketFd,1);

    socklen_t addressLen = sizeof(clientAddress);

    fd = accept(socketFd,
                          (struct sockaddr *)&clientAddress,
                          &addressLen);
#endif // __UART__

    DevRef = le_atServer_Start(fd);
    LE_ASSERT(DevRef != NULL);

    struct
    {
        const char* atCmdPtr;
        le_atServer_CmdRef_t cmdRef;
    } atCmdCreation[] = {
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
}
