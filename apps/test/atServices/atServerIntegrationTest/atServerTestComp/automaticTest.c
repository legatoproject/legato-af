/**
 * This module implements the automatic test executed by the AT command AT+TEST.
 * The syntax is:
 * AT+TEST="<AT command to execute>".
 * The AT command given in argument is sent in loop to the AT server.
 * The command creates first a new client for the AT server. This client sends in loop the
 * AT command given in argument of AT+TEST.
 * All commands and responses are sent in unsolicited to the console used to issue the command
 * AT+TEST.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BRIDGE_OPEN_AND_ADD "AT+BRIDGE=\"OPEN\";+BRIDGE=\"ADD\"\r"
#define BRIDGE_ADD          "AT+BRIDGE=\"ADD\"\r"

//--------------------------------------------------------------------------------------------------
/**
 * Automatic test context
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char atCmd[LE_ATDEFS_PARAMETER_MAX_BYTES+1];
    char unsol[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int socketFd;
    le_fdMonitor_Ref_t fdMonitor;
    int nbExpectedCharReceived;
    le_atServer_DeviceRef_t devRef;
    le_atServer_BridgeRef_t* bridgeRefPtr;
}
AutoTestContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for AutoTestContext_t
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    AutoTestPool;


//--------------------------------------------------------------------------------------------------
/**
 * Read incoming data
 *
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< File descriptor to read on
    short events ///< Event reported on fd (expect only POLLIN)
)
{
    AutoTestContext_t* autoTestCtxPtr = le_fdMonitor_GetContextPtr();
    const char expectedRsp[] = "\r\nOK\r\n";

    if ((unsigned short) events & POLLRDHUP)
    {
        le_fdMonitor_Delete(autoTestCtxPtr->fdMonitor);
    }
    else if ((unsigned short) events & POLLIN)
    {
        char rsp[100];
        ssize_t count;
        int i=0;

        count = read(fd, rsp, sizeof(rsp));

        while (i < count)
        {
            LE_DEBUG("%c, %c", rsp[i], expectedRsp[autoTestCtxPtr->nbExpectedCharReceived]);

            if ( rsp[i] == expectedRsp[autoTestCtxPtr->nbExpectedCharReceived] )
            {
                autoTestCtxPtr->nbExpectedCharReceived++;
            }
            else
            {
                autoTestCtxPtr->nbExpectedCharReceived = 0;
                if (rsp[i] == expectedRsp[0])
                {
                    autoTestCtxPtr->nbExpectedCharReceived++;
                }
            }

            switch (rsp[i])
            {
                case '\r':
                break;
                case '\n':
                    if (strlen(autoTestCtxPtr->unsol) != 0)
                    {
                        LE_ASSERT_OK(le_atServer_SendUnsolicitedResponse(autoTestCtxPtr->unsol,
                                                                        LE_ATSERVER_SPECIFIC_DEVICE,
                                                                        autoTestCtxPtr->devRef));
                        memset(autoTestCtxPtr->unsol,0,sizeof(autoTestCtxPtr->unsol));
                    }
                break;
                default:
                    autoTestCtxPtr->unsol[strlen(autoTestCtxPtr->unsol)] = rsp[i];
                break;
            }

            i++;
        }

        if (autoTestCtxPtr->nbExpectedCharReceived == strlen(expectedRsp))
        {
            write (autoTestCtxPtr->socketFd,
                   autoTestCtxPtr->atCmd,
                   strlen(autoTestCtxPtr->atCmd));

            LE_ASSERT_OK(le_atServer_SendUnsolicitedResponse(autoTestCtxPtr->atCmd,
                                                             LE_ATSERVER_SPECIFIC_DEVICE,
                                                             autoTestCtxPtr->devRef));
        }
    }
    else
    {
        LE_ERROR("Unexpected events %d", events);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread function
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread
(
    void* contextPtr
)
{
    struct sockaddr_in myAddress;
    AutoTestContext_t* autoTestCtxPtr = contextPtr;
    char monitorName[64];

    le_atServer_ConnectService();

    autoTestCtxPtr->socketFd = socket (AF_INET, SOCK_STREAM, 0);
    LE_ASSERT(autoTestCtxPtr->socketFd != -1);

    memset(&myAddress,0,sizeof(myAddress));

    myAddress.sin_port = htons(1235);
    myAddress.sin_family = AF_INET;

    // Connect to the atServerTest socket (created into SocketThread)
    LE_ASSERT(connect( autoTestCtxPtr->socketFd,
                       (struct sockaddr *)&myAddress,
                       sizeof(myAddress)) != -1);

    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", autoTestCtxPtr->socketFd);

    // Monitor the fd
    autoTestCtxPtr->fdMonitor = le_fdMonitor_Create(monitorName,
                                                    autoTestCtxPtr->socketFd,
                                                    RxNewData,
                                                    POLLIN | POLLRDHUP);

    le_fdMonitor_SetContextPtr(autoTestCtxPtr->fdMonitor, autoTestCtxPtr);

    if (NULL == *autoTestCtxPtr->bridgeRefPtr)
    {
        write (autoTestCtxPtr->socketFd, BRIDGE_OPEN_AND_ADD, sizeof(BRIDGE_OPEN_AND_ADD));
    }
    else
    {
        write (autoTestCtxPtr->socketFd, BRIDGE_ADD, sizeof(BRIDGE_ADD));
    }

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT+TEST command handler
 *
 */
//--------------------------------------------------------------------------------------------------
void automaticTest_AtTestHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    static int testThread=0;
    char atCmd[LE_ATDEFS_PARAMETER_MAX_BYTES];

    if (parametersNumber != 1)
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);
        return;
    }

    AutoTestContext_t* autoTestCtxPtr = le_mem_ForceAlloc(AutoTestPool);

    memset(autoTestCtxPtr, 0, sizeof(AutoTestContext_t));

    autoTestCtxPtr->bridgeRefPtr = contextPtr;

    LE_ASSERT(le_atServer_GetParameter(commandRef,
                                   0,
                                   atCmd,
                                   LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

    snprintf(autoTestCtxPtr->atCmd, sizeof(autoTestCtxPtr->atCmd), "%s\r", atCmd);

    LE_ASSERT(le_atServer_GetDevice(commandRef, &autoTestCtxPtr->devRef) == LE_OK);

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);

    char name[30];
    snprintf(name, sizeof(name), "TestThread-%d", testThread);
    testThread++;


    // start the thread to simulate a host
    le_thread_Start(le_thread_Create(name, TestThread, autoTestCtxPtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Automatic test initialization
 *
 */
//--------------------------------------------------------------------------------------------------
void automaticTest_Init
(
    void
)
{
    AutoTestPool = le_mem_CreatePool("AutoTestPool",sizeof(AutoTestContext_t));
}
