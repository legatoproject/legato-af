 /**
  * This module implements the le_port's tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Byte length to read from fd.
 */
//--------------------------------------------------------------------------------------------------
#define READ_BYTES 100

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
 * AT Commands definition
 */
//--------------------------------------------------------------------------------------------------
static AtCmd_t AtCmdCreation;

//--------------------------------------------------------------------------------------------------
/**
 * Thread and semaphore reference.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t Semaphore;
static le_thread_Ref_t AppThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * Device reference.
 */
//--------------------------------------------------------------------------------------------------
static le_port_DeviceRef_t devRef;

//--------------------------------------------------------------------------------------------------
/**
 * Prepare handler.
 */
//--------------------------------------------------------------------------------------------------
static void PrepareHandler
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
    LE_ASSERT_OK(le_atServer_GetCommandName(commandRef, atCommandName,
                 LE_ATDEFS_COMMAND_MAX_BYTES));

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName + 2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            exit(EXIT_FAILURE);
        break;
    }

    // Send the command type into an intermediate response
    LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, rsp));

    // Send parameters into an intermediate response
    for (i = 0; i < parametersNumber; i++)
    {
        memset(param, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);
        LE_ASSERT_OK(le_atServer_GetParameter(commandRef,
                                           i,
                                           param,
                                           LE_ATDEFS_PARAMETER_MAX_BYTES));

        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s PARAM %d: %s", atCommandName + 2, i, param);

        LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, rsp));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * AT command Mode handler
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdModeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    // Send Final response
    LE_ASSERT_OK(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, ""));

    le_sem_Post(Semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Data Mode handler
 */
//--------------------------------------------------------------------------------------------------
static void DataModeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    int fd;
    le_result_t result;
    char buffer[100];
    ssize_t len;
    size_t count;

    le_port_ConnectService();

    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    // Send Final response
    LE_ASSERT_OK(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, ""));

    result = le_port_SetDataMode(devRef, &fd);
    if (LE_OK == result)
    {
        LE_INFO("fd from port service is %d. le_port_SetDataMode() API success...", fd);
    }
    else
    {
        LE_ERROR("le_port_SetDataMode() API fails !");
        exit(EXIT_FAILURE);
    }

    le_utf8_Copy(buffer, "Test data mode", sizeof(buffer), NULL);

    count = strlen(buffer);

    len = write(fd, buffer, count);
    if (len == -1)
    {
        LE_ERROR("Failed to write to fd %m");
        le_tty_Close(fd);
        le_port_Release(devRef);
        exit(EXIT_FAILURE);
    }
    else if (len == count)
    {
        LE_INFO("Data is successfully written into device");
    }
    else
    {
        LE_ERROR("Failed to write data");
        le_tty_Close(fd);
        le_port_Release(devRef);
        exit(EXIT_FAILURE);
    }

    do
    {
        len = read(fd, buffer, READ_BYTES);
        if (len == -1)
        {
            LE_ERROR("Failed to read from fd %m");
            le_tty_Close(fd);
            le_port_Release(devRef);
            exit(EXIT_FAILURE);
        }
        if (strstr(buffer, "+++") != NULL)
        {
            LE_INFO("Device will switch into AT command mode");
            break;
        }
    } while (1);

    le_sem_Post(Semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add AtServer handler
 */
//--------------------------------------------------------------------------------------------------
static void* ATServerAddHandler
(
    void* contexPtr
)
{
    le_atServer_ConnectService();

    le_atServer_AddCommandHandler(AtCmdCreation.cmdRef,
                                  AtCmdCreation.handlerPtr,
                                  AtCmdCreation.contextPtr);
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test. This test code has been written to test the port service stub code APIs.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t result;
    le_atServer_DeviceRef_t atServerDevRef;

    Semaphore = le_sem_Create("HandlerSem",0);

    AtCmdCreation.atCmdPtr = "AT+TESTCMDMODE";

    AtCmdCreation.cmdRef = le_atServer_Create(AtCmdCreation.atCmdPtr);
    LE_ASSERT(AtCmdCreation.cmdRef != NULL);

    AtCmdCreation.handlerPtr = AtCmdModeHandler;
    AtCmdCreation.contextPtr = NULL;

    AppThreadRef = le_thread_Create("ATServerHandler", ATServerAddHandler, NULL);

    le_thread_Start(AppThreadRef);

    devRef = le_port_Request("uart");
    if (NULL != devRef)
    {
        LE_INFO("le_port_Request() API success...");
    }
    else
    {
        LE_ERROR("Device reference is NULL ! le_port_Request() API fails !");
        exit(EXIT_FAILURE);
    }

    // Wait untill AT command not received on uart.
    le_sem_Wait(Semaphore);
    le_thread_Cancel(AppThreadRef);

    AtCmdCreation.atCmdPtr = "AT+TESTDATAMODE";

    AtCmdCreation.cmdRef = le_atServer_Create(AtCmdCreation.atCmdPtr);
    LE_ASSERT(AtCmdCreation.cmdRef != NULL);

    AtCmdCreation.handlerPtr = DataModeHandler;
    AtCmdCreation.contextPtr = NULL;

    AppThreadRef = le_thread_Create("ATServerHandler", ATServerAddHandler, NULL);

    le_thread_Start(AppThreadRef);

    // Wait untill data mode testing completes.
    le_sem_Wait(Semaphore);
    le_thread_Cancel(AppThreadRef);

    result = le_port_SetCommandMode(devRef, &atServerDevRef);
    if (LE_OK == result)
    {
        LE_INFO("le_port_SetCommandMode() API success...");
    }
    else
    {
        LE_ERROR("le_port_SetCommandMode() API fails !");
        exit(EXIT_FAILURE);
    }

    result = le_port_Release(devRef);
    if (LE_OK == result)
    {
        LE_INFO("le_port_Release() API success...");
    }
    else
    {
        LE_ERROR("le_port_Release() API fails !");
        exit(EXIT_FAILURE);
    }

    LE_INFO("======= Port service Integration Test completes =======");

    exit(EXIT_SUCCESS);
}
