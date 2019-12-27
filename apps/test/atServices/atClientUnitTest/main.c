/**
 * This module implements the unit tests for AT Client API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "defs.h"

//--------------------------------------------------------------------------------------------------
/**
 * Client timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define CLIENT_TIMEOUT 10

//--------------------------------------------------------------------------------------------------
/**
 * Shared data between threads
 */
//--------------------------------------------------------------------------------------------------
static SharedData_t SharedData;


//--------------------------------------------------------------------------------------------------
/**
 * Test the atClient set text false cases.
 */
//--------------------------------------------------------------------------------------------------
void Testle_atClientSetTextFalseTest
(
    void
)
{
    le_atClient_CmdRef_t cmdRef;
    const char* textPtr = "run";

    cmdRef = le_atClient_Create();

    // Set text test
    LE_ASSERT(LE_BAD_PARAMETER == le_atClient_SetText(NULL, textPtr));
    textPtr = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
               aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    LE_ASSERT(LE_FAULT == le_atClient_SetText(cmdRef, textPtr));
    LE_ASSERT_OK(le_atClient_Delete(cmdRef));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the atClient send false cases.
 */
//--------------------------------------------------------------------------------------------------
void Testle_atClientSendFalseTest
(
    void
)
{
    int fd = 0;
    le_atClient_CmdRef_t cmdRef = NULL;
    le_atClient_DeviceRef_t devRef = NULL;

    cmdRef = le_atClient_Create();
    LE_ASSERT(LE_BAD_PARAMETER == le_atClient_Send(NULL));
    LE_ASSERT(LE_FAULT == le_atClient_Send(cmdRef));

    LE_ASSERT(NULL == le_atClient_Start(-1));

    devRef = le_atClient_Start(fd);
    LE_ASSERT_OK(le_atClient_SetDevice(cmdRef, devRef));
    LE_ASSERT_OK(le_atClient_SetFinalResponse(cmdRef, "OK|ERROR|+CME ERROR"));
    LE_ASSERT_OK(le_atClient_SetTimeout(cmdRef, 1));
    LE_ASSERT(LE_TIMEOUT == le_atClient_Send(cmdRef));

    LE_ASSERT(LE_TIMEOUT == le_atClient_SetCommandAndSend(&cmdRef, devRef, "AT",
                                                          "OK|ERROR|+CME ERROR",
                                                          "OK|ERROR|+CME ERROR", 1));
}

//--------------------------------------------------------------------------------------------------
/**
 * Client thread function
 */
//--------------------------------------------------------------------------------------------------
static void* AtClient
(
    void* contextPtr
)
{
    SharedData_t* sharedDataPtr;
    struct sockaddr_un addr;
    int socketFd;
    char buffer[LE_ATDEFS_RESPONSE_MAX_BYTES];
    sharedDataPtr = (SharedData_t*)contextPtr;
    le_atClient_DeviceRef_t devRef;
    le_atClient_CmdRef_t cmdRef;

    LE_INFO("AtClient Thread Started!");

    le_clk_Time_t timeToWait = {CLIENT_TIMEOUT, 0};
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(sharedDataPtr->semRef, timeToWait));

    socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    LE_ASSERT(socketFd != -1);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sharedDataPtr->devPathPtr, sizeof(addr.sun_path)-1);
    LE_ASSERT(connect(socketFd, (struct sockaddr *)&addr, sizeof(addr)) != -1);

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(sharedDataPtr->semRef, timeToWait));

    // Pass the socket fd to start the client
    devRef = le_atClient_Start(socketFd);
    LE_ASSERT(devRef != NULL);

    cmdRef = le_atClient_Create();
    LE_ASSERT_OK(le_atClient_SetDevice(cmdRef, devRef));
    LE_ASSERT_OK(le_atClient_SetCommand(cmdRef, "AT+CREG?"));
    LE_ASSERT_OK(le_atClient_SetFinalResponse(cmdRef, "OK|ERROR|+CME ERROR"));
    LE_ASSERT_OK(le_atClient_SetIntermediateResponse(cmdRef, "+CREG:"));
    LE_ASSERT_OK(le_atClient_Send(cmdRef));

    LE_ASSERT_OK(le_atClient_GetFinalResponse(cmdRef, buffer, LE_ATDEFS_RESPONSE_MAX_BYTES));
    LE_INFO("final rsp: %s", buffer);
    LE_ASSERT(strcmp(buffer, "OK") == 0);

    memset(buffer, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    LE_ASSERT_OK(le_atClient_GetFirstIntermediateResponse(cmdRef, buffer,
                                                          LE_ATDEFS_RESPONSE_MAX_BYTES));
    LE_INFO("inter rsp: %s", buffer);
    LE_ASSERT(strcmp(buffer, "+CREG: 0,1") == 0);
    LE_ASSERT(le_atClient_GetNextIntermediateResponse(cmdRef, buffer, LE_ATDEFS_RESPONSE_MAX_BYTES)
              == LE_NOT_FOUND);
    LE_ASSERT_OK(le_atClient_Delete(cmdRef));

    LE_ASSERT(le_atClient_SetCommandAndSend(&cmdRef, devRef, "AT+CGSN", "", "OK|ERROR|+CME ERROR",
                                            LE_ATDEFS_COMMAND_DEFAULT_TIMEOUT) == LE_OK);
    LE_ASSERT(le_atClient_GetFinalResponse(cmdRef, buffer, LE_ATDEFS_RESPONSE_MAX_BYTES) == LE_OK);
    LE_INFO("final rsp: %s", buffer);
    LE_ASSERT(strcmp(buffer, "OK") == 0);

    memset(buffer, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    LE_ASSERT_OK(le_atClient_GetFirstIntermediateResponse(cmdRef, buffer,
                                                          LE_ATDEFS_RESPONSE_MAX_BYTES));
    LE_INFO("inter rsp: %s", buffer);
    LE_ASSERT(strcmp(buffer, "359377060033064") == 0);

    LE_ASSERT(le_atClient_GetNextIntermediateResponse(cmdRef, buffer, LE_ATDEFS_RESPONSE_MAX_BYTES)
              == LE_NOT_FOUND);
    LE_ASSERT(le_atClient_Delete(cmdRef) == LE_OK);

    // Try to stop the device
    LE_ASSERT_OK(le_atClient_Stop(devRef));
    LE_ASSERT(le_atClient_Stop(devRef) == LE_FAULT);

    Testle_atClientSetTextFalseTest();
    Testle_atClientSendFalseTest();

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(sharedDataPtr->semRef, timeToWait));

    LE_INFO("====== ATClient unit test PASSED ======");
    exit(EXIT_SUCCESS);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t atClientThread;

    LE_INFO("====== ATClient unit test Start ======");

    SharedData.devPathPtr = "\0at-dev";
    SharedData.semRef = le_sem_Create("AtUnitTestSem", 0);
    SharedData.atClientThread = le_thread_GetCurrent();

    atClientThread = le_thread_Create("atClientThread", AtClient, (void *)&SharedData);
    le_thread_Start(atClientThread);

    AtClientServer(&SharedData);
}
